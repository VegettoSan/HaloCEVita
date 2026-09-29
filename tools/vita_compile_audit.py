#!/usr/bin/env python3
"""Compile every configured upstream game C unit, retaining individual errors.

This is a compile audit, not a claim that the resulting game can link/run.
No generated source replacements or undefined-symbol stubs are used.
"""
import argparse
import concurrent.futures
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--cc', default='arm-vita-eabi-gcc')
    parser.add_argument('--only', nargs='*')
    parser.add_argument('--output', default='build/vita/audit')
    parser.add_argument('--retry-failed', action='store_true',
                        help='Retry only failed units from this output (use only if shared headers/flags are unchanged)')
    parser.add_argument('--platform', action='store_true')
    args = parser.parse_args()
    os.chdir(ROOT)
    build = Path(args.output)
    build.mkdir(parents=True, exist_ok=True)
    subprocess.run([sys.executable, 'tools/vita_semantics.py',
                    'build/vita/halo_msvc_semantics.h'], check=True)
    project = next(p for p in json.loads(Path('config/config.json').read_text())['projects']
                   if p['name'] == 'halobetacache')
    flags = ['-std=gnu89', '-D__STRICT_ANSI__', '-DDEBUG', '-Dxbox',
             '-fms-extensions', '-fshort-wchar', '-fno-short-enums', '-fsigned-char', '-fcommon', '-fno-strict-aliasing',
             '-fwrapv', '-fno-delete-null-pointer-checks', '-ffp-contract=off',
             '-fno-omit-frame-pointer', '-ffunction-sections', '-fdata-sections', '-O2', '-g',
             '-Wno-multichar', '-Wno-unknown-pragmas', '-Wno-attributes',
             '-Wno-error=implicit-function-declaration', '-Wno-error=implicit-int',
             '-Wno-error=int-conversion', '-Wno-error=incompatible-pointer-types',
             '-include', 'port/vita/include/halo_vita_prefix.h',
             '-include', 'build/vita/halo_msvc_semantics.h',
             '-Iport/vita/include', '-Iport/linux/include']
    flags += ['-I' + d for d in project['options']['include_dirs'] if d != 'xbox/include']
    flags += ['-idirafter', 'port/include/xdk']
    if args.platform:
        flags += ['-std=gnu11', '-DHALO_LINUX_PLATFORM_LAYER', '-Iport/linux/src',
                  '-Iport/third_party/tomlc17', '-Iport/third_party/kcp']
    previous = None
    if args.retry_failed:
        previous = json.loads((build / 'report.json').read_text())
        sources = [r['source'] for r in previous['results'] if r['exit']]
    else:
        sources = args.only or [o['name'] for o in project['objects']
                               if o['status'] != 'MISSING' and o['name'].endswith('.c')]
    def compile_one(source):
        obj = build / Path(source).with_suffix('.o')
        obj.parent.mkdir(parents=True, exist_ok=True)
        command = [args.cc, *flags, '-c', source, '-o', str(obj)]
        completed = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        log = obj.with_suffix('.log')
        if args.retry_failed and log.exists():
            log.with_suffix('.failed.log').write_text(log.read_text())
        log.write_text(shlex.join(command) + '\n' + completed.stdout)
        return {'source': source, 'exit': completed.returncode, 'log': str(log)}
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        results = list(executor.map(compile_one, sources))
    if previous:
        retried = {r['source']: r for r in results}
        results = [retried.get(r['source'], r) for r in previous['results']]
    report = {'compiler': args.cc, 'total': len(results),
              'compiled': sum(r['exit'] == 0 for r in results), 'results': results}
    (build / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f"Native compile audit: {report['compiled']}/{report['total']} COMPILES")
    for result in results:
        if result['exit']:
            print('FAILED', result['source'], '->', result['log'])
    return int(report['compiled'] != report['total'])

if __name__ == '__main__':
    sys.exit(main())
