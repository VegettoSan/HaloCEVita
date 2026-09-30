#!/usr/bin/env python3
"""Read Vita ELF32/gzip cores and symbolize relocated app addresses locally.

Vita note layout reference: https://github.com/xyzz/vita-parse-core.
Uses only stdlib and SDK addr2line. The user must select the matching ELF;
its digest is recorded. Stack words are candidates, not an unwound backtrace.
Only module/thread/register metadata is emitted; memory and system-info notes
are not exported. Keep dump/JSON under ignored build/, not in git.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess


def u32(data, at):
    return struct.unpack_from('<I', data, at)[0]


def cstr(data):
    return data.split(b'\0', 1)[0].decode('utf-8', errors='replace')


def headers(data):
    if data[:7] != b'\x7fELF\x01\x01\x01' or len(data) < 52:
        raise ValueError('Require ELF32 little-endian')
    if struct.unpack_from('<H', data, 18)[0] != 40:
        raise ValueError('Require ARM ELF')
    offset = u32(data, 28)
    size, count = struct.unpack_from('<HH', data, 42)
    if size < 32 or offset + size * count > len(data):
        raise ValueError('Invalid program-header table')
    result = []
    for i in range(count):
        kind, start, address, _, filesz, memsz, flags, _ = struct.unpack_from('<8I', data, offset + size * i)
        if start + filesz > len(data):
            raise ValueError('Truncated segment')
        result.append({'kind': kind, 'address': address, 'size': memsz, 'flags': flags,
                       'data': data[start:start + filesz]})
    return result


def notes(segments):
    result = {}
    for segment in segments:
        if segment['kind'] != 4:
            continue
        data, offset = segment['data'], 0
        while offset + 12 <= len(data):
            namesz, descsz, _ = struct.unpack_from('<3I', data, offset)
            start = offset + 12
            desc = start + ((namesz + 3) & ~3)
            end = desc + ((descsz + 3) & ~3)
            if end > len(data):
                raise ValueError('Truncated note')
            result[cstr(data[start:start + namesz])] = data[desc:desc + descsz]
            offset = end
    return result


def entries(data):
    offset = 8
    for _ in range(u32(data, 4)):
        size = u32(data, offset)
        if size < 8 or offset + size > len(data):
            raise ValueError('Invalid Vita note entry')
        yield data[offset:offset + size]
        offset += size


def modules(data):
    result, offset = [], 8
    for _ in range(u32(data, 4)):
        count = u32(data, offset + 0x4c)
        end = offset + 0x50 + count * 0x14 + 0x10
        if end > len(data):
            raise ValueError('Truncated module entry')
        name = cstr(data[offset + 0x24:offset + 0x4c])
        segments = []
        for i in range(count):
            at = offset + 0x50 + 0x14 * i
            segments.append({'address': u32(data, at + 8), 'size': u32(data, at + 12)})
        result.append({'name': name, 'segments': segments})
        offset = end
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dump', type=Path)
    parser.add_argument('--elf', required=True, type=Path, help='Exact ELF that produced the installed eboot')
    parser.add_argument('--module', default='HaloCE.elf')
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    packed = args.dump.read_bytes()
    data = gzip.decompress(packed) if packed[:2] == b'\x1f\x8b' else packed
    segments = headers(data)
    info = notes(segments)
    loaded = modules(info['MODULE_INFO'])
    elf_data = args.elf.read_bytes()
    elf_segments = [s for s in headers(elf_data) if s['kind'] == 1]
    sdk = Path(os.environ['VITASDK'])

    def address(value):
        plain = value & ~1
        found = {'address': f'0x{value:08x}'}
        for module in loaded:
            for i, seg in enumerate(module['segments']):
                if seg['address'] <= plain < seg['address'] + seg['size']:
                    offset = plain - seg['address']
                    found.update(module=module['name'], segment=i, offset=f'0x{offset:x}')
                    if module['name'] == args.module and i < len(elf_segments):
                        original = elf_segments[i]['address'] + offset
                        found['elf_address'] = f'0x{original:08x}'
                        found['symbol'] = subprocess.run([str(sdk / 'bin/arm-vita-eabi-addr2line'),
                            '-e', str(args.elf), '-f', '-C', hex(original)], check=True,
                            capture_output=True, text=True).stdout.strip().splitlines()
                    return found
        return found

    def memory(value, size):
        for segment in segments:
            offset = value - segment['address']
            if segment['kind'] == 1 and 0 <= offset and offset + size <= len(segment['data']):
                return segment['data'][offset:offset + size]
        return b''

    registers = {u32(e, 4): struct.unpack_from('<16I', e, 8) for e in entries(info['THREAD_REG_INFO'])}
    threads = []
    for entry in entries(info['THREAD_INFO']):
        uid, reason = u32(entry, 4), u32(entry, 0x74)
        thread = {'uid': f'0x{uid:08x}', 'name': cstr(entry[8:0x28]), 'stop_reason': f'0x{reason:x}'}
        if reason == 0x30004:
            thread['stop_label'] = 'data abort'
        if reason and uid in registers:
            regs = registers[uid]
            thread['registers'] = {('sp' if i == 13 else 'lr' if i == 14 else 'pc' if i == 15 else f'r{i}'):
                                   address(value) for i, value in enumerate(regs)}
            candidates = []
            for i in range(128):
                word = memory(regs[13] + 4 * i, 4)
                if not word:
                    break
                value = u32(word, 0)
                # Filter before symbolizing: data/random words are not call sites.
                app = next((m for m in loaded if m['name'] == args.module), None)
                if app and app['segments']:
                    code = app['segments'][0]
                    if code['address'] <= (value & ~1) < code['address'] + code['size']:
                        candidates.append({'stack_address': hex(regs[13] + 4 * i), **address(value)})
            thread['stack_code_candidates_not_backtrace'] = candidates
        threads.append(thread)
    report = {'dump_sha256': hashlib.sha256(packed).hexdigest(),
              'elf_sha256': hashlib.sha256(elf_data).hexdigest(), 'matching_elf': str(args.elf),
              'symbolization_assumes_matching_elf': True, 'modules': loaded, 'threads': threads}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    for thread in threads:
        if 'registers' in thread:
            print(thread['name'], thread['stop_reason'], thread.get('stop_label', ''))
            for register in ('pc', 'lr'):
                print(register, json.dumps(thread['registers'][register]))
            for candidate in thread['stack_code_candidates_not_backtrace']:
                print('stack candidate (not backtrace)', json.dumps(candidate))
    print('Report:', args.output)


if __name__ == '__main__':
    main()
