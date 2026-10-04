"""Fresh restart's ARM object graph, adapted from the pinned Vita donor.

This phase deliberately has no ELF/VPK rule: all engine units are compiled,
but the complete host/renderer integration must pass a real link first.
"""
import argparse
import io
import json
import os
from pathlib import Path

from .ninja_syntax import Writer
from .linux_build import GAME_FLAGS, PLATFORM_FLAGS, game_sources, game_defines_and_includes
from .android_build import VARIADIC_PROTOTYPE_FILES

# Direct donor engine ABI, independent of the SDK-facing host ABI.
VITA_ABI_FLAGS = [
    '--target=armv7a-none-eabihf', '-mcpu=cortex-a9', '-mfpu=neon',
    '-mfloat-abi=hard', '-mthumb', '-fno-math-errno', '-D__vita__',
    '-DHALO_VITA=1', '-DHALO_RELOCATABLE_TAG_CACHE=1', '-fms-extensions',
    '-fshort-wchar', '-fno-short-enums', '-fsigned-char', '-fmax-type-align=1',
    '-femulated-tls', '-fcommon', '-fno-strict-aliasing', '-fwrapv',
    '-fno-delete-null-pointer-checks', '-fno-omit-frame-pointer',
    '-ffp-contract=off', '-ffunction-sections', '-fdata-sections', '-O2', '-g',
    *(f'-fno-builtin-{name}' for name in (
        'wcslen', 'wcsnlen', 'wcschr', 'wcsrchr', 'wcscmp', 'wcsncmp', 'wcscpy',
        'wcsncpy', 'wcscat', 'wcsncat', 'wmemchr', 'wmemcmp', 'wmemcpy',
        'wmemmove', 'wmemset')),
]
HOST_FLAGS = ['-mcpu=cortex-a9', '-mfpu=neon', '-mfloat-abi=hard', '-mthumb',
              '-O2', '-g', '-Wall', '-ffunction-sections', '-fdata-sections']


def quote(path):
    return '"' + str(path).replace('"', '\\"') + '"'


def generate(sdk, cc, vitagl, vitashark):
    sdk, vitagl, vitashark = sdk.resolve(), vitagl.resolve(), vitashark.resolve()
    if not (sdk / 'bin/arm-vita-eabi-gcc').is_file():
        raise SystemExit('VitaSDK GCC missing at ' + str(sdk))
    if not (vitagl / 'source/vitaGL.h').is_file():
        raise SystemExit('Provide a direct official vitaGL checkout with --vitagl')
    if not (vitashark / 'source/vitashark.h').is_file():
        raise SystemExit('Provide a direct official vitaShaRK checkout with --vitashark')
    config = json.loads(Path('port/linux/port.json').read_text())
    output = io.StringIO()
    n = Writer(output)
    n.variable('python', quote(os.sys.executable))
    n.variable('engine_cc', quote(cc))
    n.variable('host_cc', quote(sdk / 'bin/arm-vita-eabi-gcc'))
    n.rule('cc_engine', '$engine_cc -MMD -MF $out.d $flags -c $in -o $out',
           description='ARM ENGINE $in', depfile='$out.d', deps='gcc')
    n.rule('cc_host', '$host_cc -MMD -MF $out.d $flags -c $in -o $out',
           description='ARM HOST $in', depfile='$out.d', deps='gcc')
    n.rule('msvc', '$python tools/linux_msvc_semantics.py --output $out $scan', restat=True)
    semantics = Path('build/vita/halo_msvc_semantics.h')
    platform_semantics = Path('build/vita/platform_msvc_semantics.h')
    n.build(semantics, 'msvc', implicit=[Path('tools/linux_msvc_semantics.py'),
            *sorted(Path('source').rglob('*.h')), *sorted(Path('source').rglob('*.c'))],
            variables={'scan': '--all-inlines --tags source --inlines source --inlines port/include/xdk'})
    n.build(platform_semantics, 'msvc', implicit=[Path('tools/linux_msvc_semantics.py'),
            *sorted(Path('port/include/xdk').glob('*.h'))],
            variables={'scan': '--inlines port/include/xdk'})
    abi = ' '.join(VITA_ABI_FLAGS + ['--sysroot=' + quote(sdk / 'arm-vita-eabi'),
                    '-isystem ' + quote(sdk / 'arm-vita-eabi/include')])
    common = '-Iport/vita/include -Iport/linux/include -idirafter port/include/xdk'
    engine = ' '.join([abi, *GAME_FLAGS, common,
                      '-include port/linux/include/halo_linux_prefix.h',
                      '-include ' + str(semantics), game_defines_and_includes(config)])
    platform = ' '.join([abi, *PLATFORM_FLAGS, common,
                        '-include port/linux/include/halo_linux_prefix.h',
                        '-include ' + str(platform_semantics), '-Iport/linux/src -Isource -Isource/cseries'])
    host = ' '.join(HOST_FLAGS + ['-Iport/vita/include', '-Iport/linux/src',
                                 '-I' + quote(vitagl / 'source'), '-I' + quote(vitashark / 'source')])
    groups = {'engine': [], 'platform': [], 'host': [], 'abi': []}
    def add(source, rule, flags, group, implicit=()):
        obj = Path('build/vita/obj') / source.with_suffix('.o')
        obj.parent.mkdir(parents=True, exist_ok=True)
        n.build(obj, rule, inputs=source, implicit=list(implicit), variables={'flags': flags})
        groups[group].append(obj)
    for source in game_sources(config):
        flags = engine
        if source.as_posix() == 'source/shell/shell_xbox.c':
            flags += ' -Dmain=halo_main'
        # Hard-float ARM cannot infer floats in old C variadic declarations.
        if source.as_posix() in VARIADIC_PROTOTYPE_FILES:
            flags += ' -include port/android/include/halo_android_variadic_prototypes.h'
        add(source, 'cc_engine', flags, 'engine', [semantics])
    for relative in ('port/linux/src/xbox_memory.c', 'port/linux/src/xbox_kernel.c',
                     'port/linux/src/tag_relocate.c', 'port/vita/platform/vita_memory_watch.c',
                     'port/vita/platform/bink_vita.c', 'port/vita/platform/vita_pad.c'):
        add(Path(relative), 'cc_engine', platform, 'platform', [platform_semantics])
    # Main/settings stay outside this compile gate while their renderer-facing
    # dependencies are being adapted. This is not a complete host or a link.
    for name in ('vita_cpu.c', 'vita_input.c', 'vita_net.c', 'vita_movie.c', 'vita_gl_host.c', 'vita_stubs.c'):
        add(Path('port/vita/host') / name, 'cc_host', host, 'host')
    add(Path('port/vita/tests/abi_engine.c'), 'cc_engine', abi + ' -Iport/vita/include', 'abi')
    add(Path('port/vita/tests/abi_host.c'), 'cc_host', host, 'abi')
    for group, inputs in groups.items():
        n.build('vita-' + group + '-objects', 'phony', inputs=inputs)
    n.build('vita-objects', 'phony', inputs=[p for g in groups.values() for p in g])
    Path('build/vita').mkdir(parents=True, exist_ok=True)
    Path('build/vita/build.ninja').write_text(output.getvalue())
    print('Generated compile-only graph: ' + ', '.join(f'{g}={len(v)}' for g, v in groups.items()))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, default=Path(os.environ.get('VITASDK', '~/vitasdk')).expanduser())
    parser.add_argument('--cc', default='clang')
    parser.add_argument('--vitagl', type=Path, required=True)
    parser.add_argument('--vitashark', type=Path, required=True)
    args = parser.parse_args()
    generate(args.sdk, args.cc, args.vitagl, args.vitashark)
