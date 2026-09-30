#!/usr/bin/env python3
"""Check package structure, native ABI, real-core symbols and unresolved hooks."""
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import zipfile
import zlib
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
os.chdir(root)
build = Path('build/vita')
sdk = Path(os.environ['VITASDK'])
def run(tool, *args):
    return subprocess.run([str(sdk / 'bin' / ('arm-vita-eabi-' + tool)), *map(str, args)],
                          text=True, capture_output=True, check=True).stdout
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
elf = build / 'HaloCE.elf'
self_file = build / 'eboot.bin'
assert self_file.read_bytes()[:4] == b'SCE\0', 'Not a Sony SELF'
abi = run('readelf', '-h', '-A', elf)
assert 'ELF32' in abi and 'Machine:                           ARM' in abi and 'hard-float ABI' in abi
assert 'VFP registers' in abi
symbols = run('nm', elf)
required = ['cseries_initialize', 'debug_memory_manager_initialize', 'debug_malloc', 'profile_initialize',
            'data_new', 'datum_new', 'data_iterator_next', 'memory_pool_compact', 'crc_checksum_buffer',
            'cache_file_header_verify', 'nv2a_vertex_shader_to_glsl', 'nv2a_pixel_shader_to_glsl', 'XInputGetState',
            'physical_memory_allocate', 'physical_memory_verify', 'XPhysicalAlloc', 'XQueryMemoryProtect',
            'game_state_allocate_buffer', 'game_state_free_buffer',
            'tag_iterator_next', 'tag_get', 'halo_vita_cache_mount_menu',
            'halo_vita_cache_validate_menu', 'halo_vita_cache_unmount_menu', 'vita_cache_read',
            'halo_vita_ui_runtime_initialize', 'halo_vita_ui_runtime_dispose',
            'halo_vita_ui_widgets_dispose_checkpoint',
            'halo_vita_menu_tags_probe', 'vita_cache_relocate_menu', 'bitmap_group_get_bitmap_from_sequence',
            'unicode_string_list_get_string', 'font_get_character_by_ascii_code']
for name in required:
    assert re.search(r'\b[TW]\s+' + name + r'$', symbols, re.M), f'Missing real core symbol {name}'
undefined = run('nm', '-u', elf)
optional = {'_ITM_deregisterTMCloneTable', '_ITM_registerTMCloneTable', '__deregister_frame_info',
            '__gnu_Unwind_Find_exidx', '__libc_fini', '__register_frame_info', '_pthread_stack_default_user', 'pthread_cancel'}
for line in undefined.splitlines():
    parts = line.split()
    assert len(parts) == 2 and parts[0] in {'w', 'v'} and parts[1] in optional, f'Unexpected unresolved import: {line}'
expected = {'eboot.bin', 'sce_sys/param.sfo', 'sce_sys/icon0.png',
            'sce_sys/livearea/contents/bg.png', 'sce_sys/livearea/contents/startup.png',
            'sce_sys/livearea/contents/template.xml'}
def sfo_values(data):
    magic, version, keys, values, count = struct.unpack_from('<5I', data)
    assert magic == 0x46535000
    result = {}
    for i in range(count):
        key, kind, length, capacity, offset = struct.unpack_from('<HHIII', data, 20 + 16*i)
        name = data[keys + key:].split(b'\0', 1)[0].decode()
        value = data[values + offset:values + offset + length]
        result[name] = value.rstrip(b'\0').decode() if kind == 0x204 else value.hex()
    return result

def livearea_png(data, dimensions, name):
    """Verify our opaque PNG-8 contract, including actual palette/pixel data.

    A generic PNG signature/size check missed the original RGBA assets, which
    VitaShell rejected at promotion. Apply this check to every packaged image.
    """
    assert len(data) <= 420 * 1024, f'{name}: exceeds LiveArea image budget'
    assert data[:8] == b'\x89PNG\r\n\x1a\n', f'{name}: invalid PNG signature'
    chunks = []
    position = 8
    while position < len(data):
        assert position + 12 <= len(data), f'{name}: truncated PNG chunk'
        length = struct.unpack_from('>I', data, position)[0]
        end = position + 12 + length
        assert end <= len(data), f'{name}: truncated PNG payload'
        kind = data[position + 4:position + 8]
        payload = data[position + 8:position + 8 + length]
        crc = struct.unpack_from('>I', data, position + 8 + length)[0]
        assert crc == zlib.crc32(kind + payload), f'{name}: invalid PNG CRC'
        chunks.append((kind, payload))
        position = end
    assert chunks and chunks[0][0] == b'IHDR' and len(chunks[0][1]) == 13
    width, height, depth, color, compression, filtering, interlace = struct.unpack('>IIBBBBB', chunks[0][1])
    assert (width, height) == dimensions, f'{name}: incorrect dimensions'
    assert (depth, color, compression, filtering, interlace) == (8, 3, 0, 0, 0), (
        f'{name}: require indexed PNG-8/type 3; got depth={depth}, type={color} (0x8010113D risk)')
    assert [kind for kind, _ in chunks] == [b'IHDR', b'PLTE', b'IDAT', b'IEND'], (
        f'{name}: unexpected chunks/alpha; expected our opaque indexed assets')
    palette = chunks[1][1]
    assert len(palette) % 3 == 0 and 0 < len(palette) // 3 <= 128, f'{name}: invalid palette'
    assert chunks[-1][1] == b'', f'{name}: invalid IEND'
    inflater = zlib.decompressobj()
    expected_size = height * (width + 1)
    rows = inflater.decompress(chunks[2][1], expected_size + 1)
    assert len(rows) == expected_size and inflater.eof and not inflater.unused_data, f'{name}: invalid pixel stream'
    for y in range(height):
        row = rows[y * (width + 1):(y + 1) * (width + 1)]
        assert row[0] == 0 and max(row[1:]) < len(palette) // 3, f'{name}: invalid filter/palette index'
    return {'dimensions': [width, height], 'depth': depth, 'color_type': color,
            'palette_entries': len(palette) // 3, 'bytes': len(data)}

with zipfile.ZipFile(build / 'HaloCE.vpk') as package:
    assert package.testzip() is None
    assert set(package.namelist()) == expected, package.namelist()
    assert package.read('eboot.bin') == self_file.read_bytes()
    sfo = sfo_values(package.read('sce_sys/param.sfo'))
    assert sfo['TITLE_ID'] == 'HCEV00001' and sfo['APP_VER'] == '00.09'
    assert sfo['TITLE'] == 'Halo CE Vita'
    assert ('----- HaloCEVita native core bring-up ' + sfo['APP_VER'] + ' -----').encode() in elf.read_bytes(), (
        'Runtime log banner does not match packaged APP_VER')
    image_sizes = {'sce_sys/icon0.png': (128, 128),
                   'sce_sys/livearea/contents/bg.png': (840, 500),
                   'sce_sys/livearea/contents/startup.png': (280, 158)}
    images = {name: livearea_png(package.read(name), size, name) for name, size in image_sizes.items()}
    template = package.read('sce_sys/livearea/contents/template.xml')
    assert len(template) <= 32 * 1024
    livearea = ET.fromstring(template.decode('utf-8'))
    assert livearea.tag == 'livearea' and livearea.attrib['style'] == 'a1'
    assert livearea.findtext('livearea-background/image') == 'bg.png'
    assert livearea.findtext('gate/startup-image') == 'startup.png'
    assert b'libshacccg' not in '\n'.join(package.namelist()).encode()
files = ['HaloCE.vpk', 'eboot.bin', 'HaloCE.elf', 'HaloCE.elf.map']
# Hardware evidence belongs to the tested package, not every future rebuild
# with the same APP_VER. A different digest remains LINKS until tested.
tested_package = digest(build / 'HaloCE.vpk') == '88689067f668b5afb213e64508a512348360e9e168d6375ae31a1beb19750af5'
manifest = {'state': 'BOOTS' if tested_package else 'LINKS',
            'runtime_test': ('00.08 real Vita: persistent mount021/UI globals022, recheck, CRC rollback PASS (A022)'
                             if tested_package else 'rebuilt package untested;00.08 UI-mount BOOTS baseline (A022)'),
            'runtime_package_association': 'latest delivered package/banner; user did not independently supply package digest',
            'title_id': sfo['TITLE_ID'], 'project_max_demonstrated_state': 'DIAGNOSTIC RENDERS',
            'rendering_scope': 'diagnostic + upstream NV2A synthetic two-MOV triangle only; HALO DRAW REACHED/RENDERS unverified',
            'prior_hardware_evidence': 'docs/runtime/2026-09-29-00.08-debug.txt',
            'installation_test': ('00.08 installed and booted on user Vita (A022)' if tested_package else
                                  'prior00.08 installed and booted; rebuilt package hardware test pending'),
            'app_version': sfo['APP_VER'], 'livearea_images': images,
            'source_commit': subprocess.run(['git', 'rev-parse', 'HEAD'], capture_output=True, text=True, check=True).stdout.strip(),
            'source_has_uncommitted_changes': True, 'sdk': str(sdk), 'required_core_symbols': required,
            'optional_unresolved_sdk_hooks': undefined.splitlines(),
            'files': {name: {'sha256': digest(build / name), 'bytes': (build / name).stat().st_size} for name in files}}
(build / 'artifacts.json').write_text(json.dumps(manifest, indent=2) + '\n')
(build / 'SHA256SUMS').write_text('\n'.join(manifest['files'][name]['sha256'] + '  ' + name for name in files) + '\n')
(build / 'logs/elf-abi.txt').write_text(abi)
(build / 'logs/elf-unresolved.txt').write_text(undefined)
print('PASS: ELF32 ARM hard-float, real Halo core symbols, no unresolved game hooks, valid SELF/VPK/SFO, indexed PNG-8 LiveArea, no retail data/compiler module.')
print('HaloCE.vpk SHA-256:', manifest['files']['HaloCE.vpk']['sha256'])
