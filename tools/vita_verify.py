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
build = Path(os.environ.get('HALO_VITA_BUILD_DIR', 'build/vita'))
(build / 'logs').mkdir(parents=True, exist_ok=True)
app_version = re.search(r'set\(VITA_APP_VERSION "([^"]+)"\)',
                        Path('port/vita/CMakeLists.txt').read_text()).group(1)
sdk = Path(os.environ['VITASDK'])
def run(tool, *args):
    return subprocess.run([str(sdk / 'bin' / ('arm-vita-eabi-' + tool)), *map(str, args)],
                          text=True, capture_output=True, check=True).stdout
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def git_metadata(*args):
    # Container checkout and Python may run as different owners. Trust only
    # this exact checkout for these read-only provenance queries.
    return subprocess.run(['git', '-c', f'safe.directory={root}', *args],
                          stdout=subprocess.PIPE, text=True, check=True).stdout.strip()
original_runtime = 'HALO_VITA_ORIGINAL_RUNTIME:BOOL=ON' in (build / 'CMakeCache.txt').read_text()
elf = build / 'HaloCE.elf'
self_file = build / 'eboot.bin'
assert self_file.read_bytes()[:4] == b'SCE\0', 'Not a Sony SELF'
abi = run('readelf', '-h', '-A', elf)
assert 'ELF32' in abi and 'Machine:                           ARM' in abi and 'hard-float ABI' in abi
assert 'VFP registers' in abi
symbols = run('nm', elf)
required = ['glGetActiveUniform', 'halo_vita_texture_transfer_finish', 'glMapBufferRange', 'glUnmapBuffer', 'glFinish', 'cseries_initialize', 'debug_memory_manager_initialize', 'debug_malloc', 'profile_initialize',
            'crc_checksum_buffer',
            'physical_memory_allocate', 'physical_memory_verify', 'XPhysicalAlloc', 'XQueryMemoryProtect',
            'game_state_allocate_buffer', 'game_state_free_buffer',
            'tag_iterator_next', 'tag_get', 'halo_vita_cache_mount_menu',
            'halo_vita_cache_validate_menu', 'halo_vita_cache_unmount_menu', 'vita_cache_read',
            'vita_cache_resource_bind', 'vita_cache_resource_read', 'vita_cache_resource_error', 'lruv_delete',
            'halo_vita_ui_runtime_initialize', 'halo_vita_ui_runtime_dispose',
            'halo_vita_menu_root_checkpoint', 'halo_vita_menu_root_load',
            'ui_widget_load_by_name_or_tag', 'ui_widget_event_handler_function_invoke',
            'player_ui_initialize',
            'halo_vita_ui_widgets_dispose_checkpoint',
            'file_reference_create_from_path', 'file_exists', 'xbox_demos_available',
            'GetFileAttributesA', 'GetLastError', 'SetLastError',
            'halo_vita_menu_tags_probe', 'vita_cache_relocate_menu', 'bitmap_group_get_bitmap_from_sequence',
            'unicode_string_list_get_string', 'font_get_character_by_ascii_code',
            'halo_vita_renderer_initialize', 'halo_vita_renderer_render_menu_frame',
            'halo_vita_renderer_dispose_before_root', '_rasterizer_dispose', 'texture_cache_close',
            'halo_vita_menu_bitmap_resources_activate', 'render_ui_widgets',
            '_rasterizer_psuedo_dynamic_screen_quad_draw', 'D3DDevice_End',
            'D3DDevice_Present', 'players_initialize', 'players_initialize_for_new_map',
            'player_control_initialize', 'cinematic_initialize',
            'cinematic_initialize_for_new_map', 'game_time_initialize', 'halo_vita_main_render_time_update',
            'halo_vita_ui_render_clock_update', 'halo_vita_wait_monotonic_deadline',
            'sound_initialize', 'sound_render', 'sound_refresh_looping',
            'DirectSoundCreate', 'DirectSoundDoWork', 'unspatialized_impulse_sound_new',
            'ui_play_audio_feedback_sound', 'halo_vita_menu_audio_initialize',
            'halo_vita_menu_audio_frame', 'halo_vita_menu_audio_start',
            'halo_vita_menu_audio_dispose', 'halo_vita_audio_mixer_shutdown',
            'halo_vita_sound_menu_refresh', '__wrap_sound_render_time', '__wrap_sound_idle']
required += ['halo_vita_ui_process_menu_action', 'halo_vita_ui_activate_main_menu_state',
             'xgpu_texture_get', 'xgpu_gl_state_invalidate',
             'glReadPixels', 'glGetIntegerv', 'glGetFloatv', 'glIsEnabled']
if original_runtime:
    # main_menu_load now owns the entire new-map lifetime. The staged mount,
    # root construction and partial cleanup functions are correctly discarded
    # by --gc-sections; requiring them would reject the original implementation.
    recovery_only = {
        'game_state_free_buffer', 'lruv_delete', '_rasterizer_dispose',
        'sound_refresh_looping', 'halo_vita_audio_mixer_shutdown',
        'vita_cache_relocate_menu', 'halo_vita_cache_mount_menu',
        'halo_vita_cache_validate_menu', 'halo_vita_cache_unmount_menu',
        'vita_cache_read', 'vita_cache_resource_bind', 'vita_cache_resource_read', 'vita_cache_resource_error',
        'halo_vita_ui_runtime_initialize', 'halo_vita_ui_runtime_dispose',
        'halo_vita_menu_root_checkpoint', 'halo_vita_menu_root_load',
        'halo_vita_ui_widgets_dispose_checkpoint', 'halo_vita_menu_tags_probe',
        'halo_vita_renderer_initialize', 'halo_vita_renderer_render_menu_frame',
        'halo_vita_renderer_dispose_before_root', 'halo_vita_menu_bitmap_resources_activate',
        'halo_vita_menu_audio_initialize', 'halo_vita_menu_audio_frame',
        'halo_vita_menu_audio_start', 'halo_vita_menu_audio_dispose',
        'halo_vita_sound_menu_refresh', 'halo_vita_ui_process_menu_action',
        'halo_vita_ui_activate_main_menu_state', 'halo_vita_ui_render_clock_update',
        '__wrap_sound_render_time', '__wrap_sound_idle',
    }
    required = [name for name in required if name not in recovery_only]
    required += ['game_initialize', 'process_ui_widgets', 'main_screen_shell_load',
                 'event_manager_update', 'input_initialize', 'input_update',
                 'input_abstraction_update', 'main_pregame_render', 'render_frame_pregame',
                 'render_frame_present', 'global_scenario_get', 'scenario_get_game_globals',
                 'halo_vita_original_game_initialize', 'halo_vita_ui_process_shell_frame',
                 'halo_vita_original_main_menu_load', 'halo_vita_original_render_menu_frame',
                 'main_menu_load', 'main_load_ui_scenario',
                 'game_load', 'game_initialize_for_new_map', 'scenario_tags_load',
                 'cache_files_initialize', 'cache_file_open', 'tag_files_open',
                 'cache_file_read', 'cache_copy_begin', 'cache_copy_initialize', 'cache_copy_get_status',
                 'WriteFileEx', 'ReadFileEx', 'halo_vita_cache_activate_original_tags',
                 'halo_vita_cache_activate_original_bsp',
                 'XGetDeviceChanges', 'XInputDebugGetKeystroke']
    forbidden = ['halo_vita_menu_deferred_', 'halo_vita_move_menu_focus',
                 'halo_vita_dispatch_focused_button', '__wrap_compute_sound_obstruction',
                 '__wrap_observer_get_camera']
    for name in forbidden:
        assert not re.search(r'\b[TtW]\s+' + re.escape(name), symbols), f'Recovery substitute in original target: {name}'
for name in required:
    assert re.search(r'\b[TW]\s+' + name + r'$', symbols, re.M), f'Missing real core symbol {name}'
if original_runtime:
    # This original owner is private to main.c, hence a local text symbol.
    assert re.search(r'\bt\s+main_new_map$', symbols, re.M), 'Missing original main_new_map owner'
# Retaining both a native reader and an Xbox reader in an archive is insufficient:
resource_disassembly = run('objdump', '-d', '--disassemble=cache_file_read', elf)
if original_runtime:
    # The native package must retain Halo's request worker, not the recovery reader.
    assert '<vita_cache_resource_read>' not in resource_disassembly, 'parallel Vita resource reader selected'
    assert re.search(r'\bblx?\b.*<(?:cache_file_windows_thread_wake|SetEvent)>', resource_disassembly), (
        'cache_file_read does not wake the original cache request worker')
    for obsolete in ('vita_cache_prepare_slot', 'vita_cache_resource_bind', 'vita_cache_resource_read',
                     'halo_vita_main_pump_deferred_map_change'):
        assert not re.search(r'\b[Tt]\s+' + obsolete + r'$', symbols, re.M), 'recovery owner linked: ' + obsolete
    # A126: verify the shipping ARM consumers cross the typed data boundary.
    # The software bitmap text path is covered by host execution but is GC'd
    # from this target; the hardware font cache is the retained renderer owner.
    for consumer in ('string_list_get_string', 'unicode_string_list_get_string',
                     'cache_hardware_format_character'):
        code = run('objdump', '-d', '--disassemble=' + consumer, elf)
        assert re.search(r'\bblx?\b.*<tag_data_get_pointer>', code), (
            consumer + ' bypasses the native typed tag_data accessor')
        print('PASS: ARM ' + consumer + ' calls original tag_data_get_pointer')
    code = run('objdump', '-d', '--disassemble=tag_data_get_pointer', elf)
    assert re.search(r'\bblx?\b.*<halo_vita_cache_resolve_compiled_pointer>', code), (
        'tag_data accessor bypasses the active compiled-image resolver')
    # A129: initialization may inline child recursion, so inspect the retained
    # original owner functions together. They contain the original bitmap
    # sequence lookup plus the corrected child and created-handler lookups.
    owners = ('ui_widget_load_children_recursive', 'widget_instance_initialize',
              'ui_widget_load_by_name_or_tag')
    names = re.findall(r'^[0-9a-f]+ [Tt] (\S+)$', symbols, re.M)
    owners = [name for name in names if any(name == owner or name.startswith(owner + '.')
                                          for owner in owners)]
    owner_code = '\n'.join(run('objdump', '-d', '--disassemble=' + owner, elf)
                           for owner in owners)
    assert len(re.findall(r'\bblx?\b.*<tag_block_get_element_with_size>', owner_code)) >= 3, (
        'original widget loading bypasses typed child/created-handler boundaries')
    for consumer in ('event_handler_dispatch', 'widget_instance_render_recursive',
                     'virtual_keyboard_get_character'):
        code = run('objdump', '-d', '--disassemble=' + consumer, elf)
        assert re.search(r'\bblx?\b.*<tag_block_get_element_with_size>', code), (
            consumer + ' bypasses compiled widget/keyboard block translation')
    print('PASS: ARM original widget load/created events/conditions/render inputs/keyboard use typed tag_block accessor')
else:
    assert re.search(r'\bblx?\b.*<vita_cache_resource_read>', resource_disassembly), 'recovery cache reader missing'
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
    assert sfo['TITLE_ID'] == 'HCEV00001' and sfo['APP_VER'] == app_version
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
tested_package = digest(build / 'HaloCE.vpk') == '7f634c28e8ea851f7fded25503bd0f39c5d269a43087f4761c5fd20952120c88'
manifest = {'state': 'BOOTS' if tested_package else 'LINKS',
            'runtime_test': ('00.11 real Vita: original root and creation handlers86/23 PASS (A031)'
                             if tested_package else 'current package untested on Vita; prior00.30 partial UI with remaining rectangles (A089)'),
            'runtime_package_association': 'latest delivered package/banner; user did not independently supply package digest',
            'title_id': sfo['TITLE_ID'], 'project_max_demonstrated_state': 'partial HALO DRAW RENDERS',
            'rendering_scope': 'prior00.27/00.30 original logo/bitmap labels visible, white rectangles unresolved; current GPU alpha isolation requires console evidence (A087/A089/A091)',
            'audio_scope': 'prior00.26 continuous title1/Square feedback and reported29-30FPS (A083); current package audibility/performance untested',
            'navigation_scope': ('complete original UI update and callback tables, original input/events, native save I/O; console acceptance pending' if original_runtime else 'recovery staged widget/list/history bridge; console acceptance pending'),
            'runtime_route': 'original game_initialize/process_ui_widgets/main_pregame_render' if original_runtime else 'recovery staged UI',
            'world_scope': 'scenario/BSP world loading and campaign not yet accepted; compiled source closure is not runtime evidence',
            'alpha_probe_scope': 'up to2 first-frame copies of exact original bitmap shader draws; native storage/output readback, no authored alpha mutation (A091)',
            'prior_hardware_evidence': 'docs/ATTEMPTS.md A083/A087/A089: continuous audio, partial original bitmap UI; current source is not hardware-verified',
            'installation_test': ('00.11 installed and booted on user Vita (A031)' if tested_package else
                                  'current package installation/runtime pending; prior00.30 BOOTS/partial original UI (A089)'),
            'app_version': sfo['APP_VER'], 'livearea_images': images,
            'source_commit': git_metadata('rev-parse', 'HEAD'),
            'source_has_uncommitted_changes': bool(git_metadata(
                'status', '--porcelain', '--untracked-files=no')),
            'sdk': str(sdk), 'required_core_symbols': required,
            'optional_unresolved_sdk_hooks': undefined.splitlines(),
            'files': {name: {'sha256': digest(build / name), 'bytes': (build / name).stat().st_size} for name in files}}
(build / 'artifacts.json').write_text(json.dumps(manifest, indent=2) + '\n')
(build / 'SHA256SUMS').write_text('\n'.join(manifest['files'][name]['sha256'] + '  ' + name for name in files) + '\n')
(build / 'logs/elf-abi.txt').write_text(abi)
(build / 'logs/elf-unresolved.txt').write_text(undefined)
print('PASS: ELF32 ARM hard-float, real Halo core symbols, no unresolved game hooks, valid SELF/VPK/SFO, indexed PNG-8 LiveArea, no retail data/compiler module.')
print('HaloCE.vpk SHA-256:', manifest['files']['HaloCE.vpk']['sha256'])
