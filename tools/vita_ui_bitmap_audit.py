#!/usr/bin/env python3
"""Audit authored Xbox UI masks without modifying/extracting retail assets.

Layouts come from cache_files.c, ui_widget_tags.h and bitmap_group.h.
Only the main-menu root's direct child graph is walked. Output is metadata
and DXT alpha statistics, never pixels or tag payloads.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import zlib

BASE = 0x803A6000


def dxt_alpha(payload, width, height, fmt):
    block_size = 8 if fmt == 14 else 16
    if fmt not in (14, 15, 16) or width <= 0 or height <= 0:
        raise ValueError('unsupported alpha audit format/dimensions')
    bx, by = (width + 3) // 4, (height + 3) // 4
    if len(payload) < bx * by * block_size:
        raise ValueError('short DXT level')
    counts = Counter()
    for y in range(by):
        for x in range(bx):
            block = payload[(y * bx + x) * block_size:][:block_size]
            if fmt == 14:
                c0, c1, bits = struct.unpack('<HHI', block)
                alphas = [0 if c0 <= c1 and (bits >> (2 * i)) & 3 == 3 else 255
                          for i in range(16)]
            elif fmt == 15:
                bits = int.from_bytes(block[:8], 'little')
                alphas = [((bits >> (4 * i)) & 15) * 17 for i in range(16)]
            else:
                a0, a1 = block[:2]
                values = [a0, a1]
                values += ([( (8-i)*a0 + (i-1)*a1) // 7 for i in range(2, 8)]
                           if a0 > a1 else
                           [((6-i)*a0 + (i-1)*a1) // 5 for i in range(2, 6)] + [0, 255])
                bits = int.from_bytes(block[2:8], 'little')
                alphas = [values[(bits >> (3 * i)) & 7] for i in range(16)]
            for i, a in enumerate(alphas):
                if x * 4 + i % 4 < width and y * 4 + i // 4 < height:
                    counts[a] += 1
    return dict(min=min(counts), max=max(counts), zero=counts[0],
                opaque=counts[255], pixels=sum(counts.values()))


def inspect(path):
    disk = path.read_bytes()
    if len(disk) < 2048:
        raise ValueError('short map header')
    magic, version, size, _, offset, length = struct.unpack_from('<6I', disk)
    if magic != 0x68656164 or version != 5 or struct.unpack_from('<I', disk, 2044)[0] != 0x666F6F74:
        raise ValueError('not Xbox v5')
    if not 2048 <= size <= 128 * 1024 * 1024 or len(disk) > size:
        raise ValueError('map exceeds audit bounds')
    if len(disk) < size:
        inflater = zlib.decompressobj()
        data = disk[:2048] + inflater.decompress(disk[2048:], size - 2048 + 1)
        if not inflater.eof or inflater.unconsumed_tail:
            raise ValueError('invalid/truncated/oversize compressed map')
    else:
        data = disk
    if len(data) != size or offset < 2048 or offset + length > size:
        raise ValueError('logical map/tag range mismatch')
    tags = data[offset:offset+length]

    def read(pointer, count):
        start = pointer - BASE
        if start < 0 or count < 0 or start + count > length:
            raise ValueError('tag pointer out of range')
        return tags[start:start+count]

    table, _, _, count = struct.unpack_from('<4I', tags)
    if count > length // 32 or struct.unpack_from('<I', tags, 32)[0] != 0x74616773:
        raise ValueError('invalid tag table')
    records = {}
    for i in range(count):
        group, _, _, idx, label, pointer, _, _ = struct.unpack('<8I', read(table + i*32, 32))
        name = read(label, 1) + read(label+1, min(255, length-(label+1-BASE)))
        if b'\0' not in name:
            raise ValueError('unterminated name')
        records[idx] = group, name.split(b'\0')[0].decode('ascii'), pointer
    root = next(idx for idx, (_, name, _) in records.items()
                if name == r'ui\shell\main_menu\main_menu')
    seen, widgets, masks = set(), [], []

    def walk(idx):
        if idx in seen:
            return
        seen.add(idx)
        group, name, pointer = records[idx]
        if group != 0x44654C61:
            raise ValueError('child is not DeLa')
        raw = read(pointer, 1004)
        bg = struct.unpack_from('<I', raw, 68)[0]
        widgets.append(dict(datum=f'{idx:08x}', name=name,
                            text_argb=struct.unpack_from('<4f', raw, 268),
                            background=f'{bg:08x}'))
        if bg != 0xFFFFFFFF:
            bitmap_group, bitmap_name, bp = records[bg]
            if bitmap_group != 0x6269746D:
                raise ValueError('background is not bitm')
            n, addr, _ = struct.unpack_from('<3I', read(bp, 108), 96)
            if n > length // 48:
                raise ValueError('bitmap block count out of bounds')
            for j in range(n):
                bitmap = read(addr + j*48, 48)
                w, h, depth, kind, fmt = struct.unpack_from('<5h', bitmap, 4)
                po, ps = struct.unpack_from('<2I', bitmap, 24)
                if po < 2048 or po + ps > offset or kind != 0 or depth != 1:
                    raise ValueError('UI bitmap resource range/type invalid')
                masks.append(dict(name=bitmap_name, frame=j, format=fmt, size=[w, h],
                                  alpha=dxt_alpha(data[po:po+ps], w, h, fmt)))
        n, addr, _ = struct.unpack_from('<3I', raw, 992)
        if n > length // 80:
            raise ValueError('child block count out of bounds')
        for j in range(n):
            walk(struct.unpack_from('<I', read(addr+j*80, 80), 12)[0])

    walk(root)
    return dict(sha256=hashlib.sha256(disk).hexdigest(), logical_bytes=size,
                tag_count=count, widgets=widgets, masks=masks)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('map', type=Path)
    print(json.dumps(inspect(parser.parse_args().map), indent=2))
