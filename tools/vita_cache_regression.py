#!/usr/bin/env python3
"""Exercise the actual portable Vita cache reader on synthetic/error cases.

Optional map paths are read in place; report metadata only, never save tag
payloads or modified maps. This does not execute ARM/game code or the GPU.
"""
import argparse
import ctypes as C
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
BASE = 0x803A6000
CAPACITY = 22 * 1024 * 1024


class Info(C.Structure):
    _fields_ = [(name, C.c_uint32) for name in (
        'logical_size', 'tag_offset', 'tag_size', 'tag_count', 'scenario_index',
        'vertices', 'indices', 'tag_crc')] + [('compressed', C.c_int)]


def fixture(compressed=False):
    tags = bytearray(96 + 1456)
    struct.pack_into('<9I', tags, 0, BASE + 36, 0xe1740000, 0, 1, 0, 0, 0, 0, 0x74616773)
    struct.pack_into('<8I', tags, 36, 0x73636e72, 0xffffffff, 0xffffffff,
                     0xe1740000, BASE + 68, BASE + 96, 0, 0)
    tags[68:81] = b'levels\\ui\\ui\0'
    struct.pack_into('<h', tags, 96 + 60, 2)
    header = bytearray(2048)
    struct.pack_into('<6I', header, 0, 0x68656164, 5, 8192 + len(tags), 0, 8192, len(tags))
    header[32:38] = b'probe\0'
    header[64:78] = b'01.10.12.2276\0'
    struct.pack_into('<I', header, 2044, 0x666f6f74)
    payload = bytes(8192 - 2048) + tags
    return bytes(header) + (zlib.compress(payload) if compressed else payload), bytes(tags)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('maps', nargs='*', type=Path)
    parser.add_argument('--cc', default='cc')
    args = parser.parse_args()
    os.chdir(ROOT)
    build = ROOT / 'build/vita/tests/cache-a017'
    build.mkdir(parents=True, exist_ok=True)
    sdk = Path(os.environ.get('VITASDK', '/usr/local/vitasdk-hardfp'))
    # Prefer host development headers/library. On this machine only the
    # host shared library is installed; zlib's portable SDK headers suffice.
    command = [args.cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC', '-O2',
               '-Iport/vita/include', '-idirafter', str(sdk / 'arm-vita-eabi/include'),
               'port/vita/src/vita_cache_read.c', '-l:libz.so.1', '-o', str(build / 'cache.so')]
    subprocess.run(command, check=True)
    reader, libc = C.CDLL(str(build / 'cache.so')), C.CDLL(None)
    libc.fopen.argtypes, libc.fopen.restype = [C.c_char_p, C.c_char_p], C.c_void_p
    libc.fclose.argtypes = [C.c_void_p]
    callback_type = C.CFUNCTYPE(C.c_int, C.c_uint32, C.c_void_p)
    reader.vita_cache_read.argtypes = [C.c_void_p, C.c_void_p, C.c_size_t, C.POINTER(Info),
                                      callback_type, C.c_void_p, C.c_char_p, C.c_size_t]

    def read(path, cancelled=False, capacity=CAPACITY):
        buffer = C.create_string_buffer(CAPACITY)
        error, info = C.create_string_buffer(160), Info()
        positions = []

        def progress(position, _context):
            positions.append(position)
            return not cancelled

        callback = callback_type(progress)
        file = libc.fopen(os.fsencode(path), b'rb')
        if not file:
            raise OSError(f'cannot open {path}')
        try:
            result = reader.vita_cache_read(file, buffer, capacity, C.byref(info), callback,
                                            None, error, len(error))
        finally:
            libc.fclose(file)
        assert positions == sorted(positions), 'progress went backwards'
        return result, error.value.decode(), info, buffer

    count = 0
    with tempfile.TemporaryDirectory(dir=build) as temporary:
        path = Path(temporary) / 'probe.map'
        for compressed in (False, True):
            data, expected = fixture(compressed)
            path.write_bytes(data)
            result, error, info, buffer = read(path)
            assert result, error
            assert info.tag_count == 1 and info.scenario_index == 0xe1740000
            assert info.compressed == compressed
            assert info.tag_crc == zlib.crc32(expected)
            assert buffer.raw[:len(expected)] == expected, 'reader changed tag data'
            count += 1
            result, error, _, _ = read(path, cancelled=True)
            assert not result and error == 'cancelled'
            count += 1
            result, error, _, _ = read(path, capacity=100)
            assert not result and 'bounds' in error
            count += 1

        plain, _ = fixture()
        mutations = [
            ('version', 4, 7), ('logical-limit', 8, 0xffffffff),
            ('tag-offset', 16, 0xfffffff0), ('tag-size', 20, CAPACITY + 1),
            ('footer', 2044, 0), ('tag-magic', 8192 + 32, 0),
            ('table', 8192, BASE - 1), ('count', 8192 + 12, 0xffffffff),
            ('unaligned-table', 8192, BASE + 37),
            ('datum', 8192 + 36 + 12, 0xe1740001),
            ('name', 8192 + 36 + 16, BASE + 100000),
            ('root', 8192 + 36 + 20, 0xffffffff),
            ('unaligned-root', 8192 + 36 + 20, BASE + 97),
            ('scenario-group', 8192 + 36, 0x6269746d),
            ('vertices', 8192 + 16, 0xffffffff),
            ('indices', 8192 + 24, 0xffffffff),
        ]
        for name, offset, value in mutations:
            changed = bytearray(plain)
            struct.pack_into('<I', changed, offset, value)
            path.write_bytes(changed)
            result, error, _, _ = read(path)
            assert not result, f'accepted invalid {name}'
            count += 1
        for data in (plain[:100], fixture(True)[0][:-2], fixture(True)[0][:-1] + b'!'):
            path.write_bytes(data)
            result, error, _, _ = read(path)
            assert not result, 'accepted truncated/corrupt stream'
            count += 1
    print(f'PASS: actual C reader {count} synthetic success/cancel/bounds/corruption cases')
    for path in args.maps:
        result, error, info, _ = read(path)
        report = {'file': path.name, 'PASS': bool(result), 'error': error,
                  **{name: getattr(info, name) for name, _ in Info._fields_}}
        print(json.dumps(report, sort_keys=True))
        if not result:
            raise RuntimeError(f'{path.name}: {error}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
