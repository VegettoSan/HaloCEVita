#!/usr/bin/env python3
"""Read-only authored UI navigation inventory; emits metadata, never assets."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import zlib

BASE, NONE = 0x803A6000, 0xFFFFFFFF


def inspect(path):
    disk = path.read_bytes()
    magic, version, size, _, offset, length = struct.unpack_from('<6I', disk)
    assert magic == 0x68656164 and version == 5
    assert 2048 <= size <= 128 * 1024 * 1024 and offset + length <= size
    data = disk if len(disk) == size else disk[:2048] + zlib.decompress(disk[2048:])
    assert len(data) == size
    tags = data[offset:offset + length]
    def read(address, count):
        start = address - BASE
        assert 0 <= start <= len(tags) and 0 <= count <= len(tags) - start
        return tags[start:start + count]
    table, _, _, count = struct.unpack_from('<4I', tags)
    records = {}
    for i in range(count):
        group, _, _, datum, label, address, _, _ = struct.unpack('<8I', read(table + i * 32, 32))
        name = read(label, min(256, len(tags) - (label - BASE))).split(b'\0', 1)[0].decode('ascii')
        if group == 0x44654C61:
            records[datum] = name, address
    def block(raw, off, width):
        n, addr, _ = struct.unpack_from('<3I', raw, off)
        assert n <= len(tags) // width
        return [read(addr + j * width, width) for j in range(n)]
    widgets = {}
    for datum, (name, address) in records.items():
        raw = read(address, 1004)
        events = []
        edges = set()
        for item in block(raw, 84, 72):
            flags, kind, function = struct.unpack_from('<Ihh', item)
            target = struct.unpack_from('<I', item, 20)[0]
            script = item[40:72].split(b'\0', 1)[0].decode('ascii')
            events.append(dict(flags=flags, event=kind, function=function, target=target, script=script))
            if target != NONE:
                assert target in records
                edges.add(target)
        for off in (724, 992):
            for item in block(raw, off, 80):
                target = struct.unpack_from('<I', item, 12)[0]
                if target != NONE:
                    assert target in records
                    edges.add(target)
        widgets[datum] = dict(name=name, type=struct.unpack_from('<h', raw)[0],
            flags=struct.unpack_from('<I', raw, 44)[0], events=events,
            data=[struct.unpack_from('<h', b)[0] for b in block(raw, 72, 36)], edges=sorted(edges))
    root = next(d for d, w in widgets.items() if w['name'] == r'ui\shell\main_menu\main_menu')
    reachable, pending = set(), [root]
    while pending:
        datum = pending.pop()
        if datum in reachable:
            continue
        reachable.add(datum)
        pending.extend(widgets[datum]['edges'])
    events = Counter(e['function'] for d in reachable for e in widgets[d]['events'] if e['flags'] & 128)
    data_functions = Counter(f for d in reachable for f in widgets[d]['data'])
    return dict(sha256=hashlib.sha256(disk).hexdigest(), widgets=len(widgets), reachable=len(reachable),
        event_functions=dict(sorted(events.items())), data_functions=dict(sorted(data_functions.items())),
        graph={f'{d:08x}': widgets[d] for d in sorted(reachable)})


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('map', type=Path)
    parser.add_argument('--summary', action='store_true')
    args = parser.parse_args()
    result = inspect(args.map)
    if args.summary:
        result.pop('graph')
    print(json.dumps(result, indent=2))
