#!/usr/bin/env python3
"""Actual C menu relocation: transactional corruption tests and optional real maps.

Reads supplied caches in place; prints metadata only and never writes tag data.
Host target addresses are deliberately different from Xbox and host pointers.
"""
import ctypes as C
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BASE, NATIVE, NONE = 0x803A6000, 0x85FA6000, 0xFFFFFFFF


class Stats(C.Structure):
    _fields_ = [(name, C.c_uint32) for name in (
        'widgets', 'fonts', 'string_lists', 'bitmap_groups', 'pointers',
        'blocks', 'references', 'bitmap_count', 'volume_count', 'menu_index', 'menu_widgets',
        'block_addresses', 'data_addresses', 'reference_names')]


def fixture():
    tags = bytearray(8192)
    count, cursor = 5, 36 + 5 * 32
    locations = {}

    def alloc(name, size):
        nonlocal cursor
        cursor = (cursor + 3) & ~3
        locations[name], cursor = cursor, cursor + size
        return locations[name]

    def u32(offset, *values):
        struct.pack_into('<' + 'I' * len(values), tags, offset, *values)

    def reference(offset, index=NONE, group=NONE):
        u32(offset, group, 0, 0, index)

    names = [b'levels\\ui\\ui', b'ui\\shell\\main_menu\\main_menu',
             b'probe_bitmap', b'probe_font', b'probe_strings']
    groups = [0x73636E72, 0x44654C61, 0x6269746D, 0x666F6E74, 0x75737472]
    roots = []
    for i, (name, group, size) in enumerate(zip(names, groups, [1456, 1004, 108, 156, 12])):
        n = alloc('name' + str(i), len(name) + 1)
        tags[n:n + len(name)] = name
        root = alloc('root' + str(i), size)
        roots.append(root)
        u32(36 + 32 * i, group, NONE, NONE, 0xE1740000 + i, BASE + n, BASE + root, 0, 0)
    u32(0, BASE + 36, 0xE1740000, 0, count, 0, 0, 0, 0, 0x74616773)
    struct.pack_into('<h', tags, roots[0] + 60, 2)
    for off in [56, 236, 252, 340, 356, 420]:
        reference(roots[1] + off)
    for off in [60, 76, 92, 108]:
        reference(roots[3] + off)
    # Actual menu bitmap reference has a retained name with cache length=0.
    reference(roots[1] + 56, 0xE1740002, groups[2])
    u32(roots[1] + 60, BASE + locations['name2'])
    sequence, bitmap = alloc('sequence', 64), alloc('bitmap', 48)
    u32(roots[2] + 84, 1, BASE + sequence, 0)
    u32(roots[2] + 96, 1, BASE + bitmap, 0)
    struct.pack_into('<hh', tags, sequence + 32, 0, 1)
    u32(bitmap, 0x6269746D)
    struct.pack_into('<hhh', tags, bitmap + 4, 4, 4, 1)
    entry, text = alloc('string_entry', 20), alloc('text', 4)
    u32(roots[4], 1, BASE + entry, 0)
    u32(entry, 4, 0, 0, BASE + text, 0)
    tags[text:text + 4] = b'A\0\0\0'
    table, indices, character, pixels = (alloc('table', 12), alloc('indices', 512),
                                         alloc('character', 20), alloc('pixels', 4))
    u32(roots[3] + 48, 1, BASE + table, 0)
    u32(table, 256, BASE + indices, 0)
    tags[indices:indices + 512] = b'\xff' * 512
    struct.pack_into('<h', tags, indices + 2 * 65, 0)
    u32(roots[3] + 124, 1, BASE + character, 0)
    u32(roots[3] + 136, 4, 0, 0, BASE + pixels, 0)
    return tags[:cursor], locations


def verify_root_event_graph(tags):
    """Check the real ui.map contract used by Vita's selective root dispatch."""
    u32 = lambda p: struct.unpack_from('<I', tags, p)[0]
    i16 = lambda p: struct.unpack_from('<h', tags, p)[0]
    offset = lambda address: address - BASE
    directory = offset(u32(0))
    count = u32(12)
    entries = {u32(directory + 32 * i + 12): directory + 32 * i for i in range(count)}
    by_name = {
        tags[offset(u32(e + 16)):].split(b'\0', 1)[0]: index
        for index, e in entries.items()
    }
    root_index = by_name[b'ui\\shell\\main_menu\\main_menu']
    visited, handlers, creation = set(), set(), []

    def visit(index):
        assert index not in visited
        visited.add(index)
        definition = offset(u32(entries[index] + 20))
        assert not (u32(definition + 44) & 2), 'pause-game-time needs an unlinked contract'
        event_count, event_address = u32(definition + 84), u32(definition + 88)
        for j in range(event_count):
            event = offset(event_address) + j * 72
            kind, function, flags = i16(event + 4), i16(event + 6), u32(event)
            handlers.add(function)
            if kind == 24:
                creation.append((function, flags))
        child_count, child_address = u32(definition + 992), u32(definition + 996)
        for j in range(child_count):
            child = u32(offset(child_address) + j * 80 + 12)
            if child != NONE:
                visit(child)
        if i16(definition) == 3:
            description = u32(definition + 0x1A4 + 12)
            if description != NONE:
                visit(description)

    visit(root_index)
    assert len(visited) == 9, visited
    assert handlers == {0, 23, 86, 87, 101}, handlers
    assert sorted(creation) == [(23, 0x80), (86, 0x80)], creation
    return {'reachable_widgets': len(visited), 'event_indices': sorted(handlers),
            'creation_handlers': creation}


def main():
    os.chdir(ROOT)
    build = ROOT / 'build/vita/tests/menu-a019'
    build.mkdir(parents=True, exist_ok=True)
    sdk = Path(os.environ.get('VITASDK', '/usr/local/vitasdk-hardfp'))
    command = ['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC', '-O2',
               '-Iport/vita/include', '-idirafter', str(sdk / 'arm-vita-eabi/include'),
               'port/vita/src/vita_menu_relocate.c', 'port/vita/src/vita_cache_read.c',
               '-l:libz.so.1', '-o', str(build / 'menu.so')]
    subprocess.run(command, check=True)
    lib = C.CDLL(str(build / 'menu.so'))
    lib.vita_cache_relocate_menu.argtypes = [C.c_void_p, C.c_size_t, C.c_uint32,
                                            C.POINTER(Stats), C.c_char_p, C.c_size_t]
    lib.vita_cache_relocate_menu.restype = C.c_void_p
    lib.vita_cache_restore_menu.argtypes = [C.c_void_p]

    def relocate(tags, base=NATIVE, expect=True):
        buffer = C.create_string_buffer(bytes(tags), len(tags))
        error, stats = C.create_string_buffer(256), Stats()
        plan = lib.vita_cache_relocate_menu(buffer, len(tags), base, C.byref(stats), error, 256)
        assert bool(plan) == expect, error.value.decode()
        changed = buffer.raw
        if plan:
            assert changed != bytes(tags), 'Successful relocation did not change a pointer'
            lib.vita_cache_restore_menu(plan)
        assert buffer.raw == bytes(tags), 'Rejected transaction or rollback changed source bytes'
        return changed, stats

    tags, loc = fixture()
    changed, stats = relocate(tags)
    assert struct.unpack_from('<I', changed, loc['root2'] + 88)[0] == NATIVE + loc['sequence']
    assert struct.unpack_from('<I', changed, loc['string_entry'] + 12)[0] == NATIVE + loc['text']
    assert changed[loc['bitmap']:loc['bitmap'] + 48] == tags[loc['bitmap']:loc['bitmap'] + 48]
    assert stats.menu_index == 0xE1740001 and stats.fonts == stats.widgets == stats.bitmap_count == 1
    # Keep the typed transaction active across a consumer access. A second
    # mount attempt must reject native pointers without changing the image.
    persistent = C.create_string_buffer(bytes(tags), len(tags))
    error, active_stats = C.create_string_buffer(256), Stats()
    plan = lib.vita_cache_relocate_menu(persistent, len(tags), NATIVE,
                                        C.byref(active_stats), error, 256)
    assert plan, error.value.decode()
    mounted = persistent.raw
    assert struct.unpack_from('<I', mounted, loc['root2'] + 88)[0] == NATIVE + loc['sequence']
    second = lib.vita_cache_relocate_menu(persistent, len(tags), NATIVE,
                                          C.byref(Stats()), error, 256)
    assert not second and persistent.raw == mounted
    lib.vita_cache_restore_menu(plan)
    assert persistent.raw == bytes(tags)
    cases = 2
    mutations = [
        ('late data out of bounds', loc['string_entry'] + 12, BASE + len(tags)),
        ('negative block count', loc['root2'] + 84, NONE),
        ('invalid definition pointer', loc['root2'] + 92, BASE + 100),
        ('misaligned block pointer', loc['root2'] + 88, BASE + loc['sequence'] + 1),
        ('reference salt mismatch', loc['root1'] + 68, 0x11110002),
        ('reference group mismatch', loc['root1'] + 56, 0x666F6E74),
        ('unterminated name length', loc['root1'] + 64, 255),
        ('odd unicode size', loc['string_entry'], 3),
        ('bad font table count', loc['table'], 255),
        ('oversized typed data', loc['string_entry'], NONE),
        ('bitmap signature', loc['bitmap'], 0),
        ('font character index', loc['indices'] + 2 * 65, 2),
        ('UTF16 terminator', loc['text'], 0x00010041),
    ]
    for name, offset, value in mutations:
        bad = bytearray(tags)
        struct.pack_into('<I', bad, offset, value)
        relocate(bad, expect=False)
        cases += 1
    cyclic = bytearray(tags) + bytearray(80)
    struct.pack_into('<III', cyclic, loc['root1'] + 992, 1, BASE + len(tags), 0)
    struct.pack_into('<4I', cyclic, len(tags), 0x44654C61, 0, 0, 0xE1740001)
    relocate(cyclic, expect=False)
    cases += 1
    relocate(tags, base=0xFFFFFFFC, expect=False)
    relocate(tags, base=NATIVE + 1, expect=False)
    cases += 2
    print(f'PASS: {cases} synthetic menu relocation/transaction cases')
    spec = importlib.util.spec_from_file_location('cache_regression', ROOT / 'tools/vita_cache_regression.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    lib.vita_cache_read.argtypes = [C.c_void_p, C.c_void_p, C.c_size_t, C.POINTER(module.Info),
                                   C.c_void_p, C.c_void_p, C.c_char_p, C.c_size_t]
    libc = C.CDLL(None)
    libc.fopen.argtypes, libc.fopen.restype = [C.c_char_p, C.c_char_p], C.c_void_p
    libc.fclose.argtypes = [C.c_void_p]
    for path in map(Path, sys.argv[1:]):
        with path.open('rb') as source:
            before_hash = hashlib.file_digest(source, 'sha256').hexdigest()
        buffer, info, error = C.create_string_buffer(module.CAPACITY), module.Info(), C.create_string_buffer(256)
        file = libc.fopen(os.fsencode(path), b'rb')
        assert file, path
        try:
            assert lib.vita_cache_read(file, buffer, module.CAPACITY, C.byref(info), None, None, error, 256), error.value
        finally:
            libc.fclose(file)
        has_menu = path.stem.casefold() == 'ui'
        _, stats = relocate(buffer.raw[:info.tag_size], expect=has_menu)
        graph = verify_root_event_graph(buffer.raw[:info.tag_size]) if has_menu else {}
        with path.open('rb') as source:
            assert hashlib.file_digest(source, 'sha256').hexdigest() == before_hash
        print(json.dumps({'map': path.name, 'status': 'PASS' if has_menu else 'SAFE REJECTION: no Main Menu tag', 'source_unchanged': True,
                          'tag_crc': f'{info.tag_crc:08x}',
                          **graph,
                          **{name: getattr(stats, name) for name, _ in Stats._fields_}}))


if __name__ == '__main__':
    main()
