#!/usr/bin/env python3
"""Aggregate every compiled game object; report missing platform imports.

Relocatable aggregation is not a final Vita executable link.
"""
import json
import os
from pathlib import Path
import subprocess
import sys

directory = Path(sys.argv[1] if len(sys.argv) > 1 else 'build/vita/audit-abi')
sdk = Path(os.environ['VITASDK'])
report = json.loads((directory / 'report.json').read_text())
if any(r['exit'] for r in report['results']):
    sys.exit('Compile failures remain; do not aggregate an incomplete game.')
objects = [directory / Path(r['source']).with_suffix('.o') for r in report['results']]
rsp = directory / 'game-objects.rsp'
rsp.write_text('\n'.join('"' + str(p) + '"' for p in objects) + '\n')
output = directory / 'HaloGame.audit.o'
command = [str(sdk / 'bin/arm-vita-eabi-ld'), '-r', '-o', str(output), '@' + str(rsp)]
result = subprocess.run(command, capture_output=True, text=True)
(directory / 'aggregate-link.log').write_text(' '.join(command) + '\n' + result.stdout + result.stderr)
if result.returncode:
    print(result.stderr)
    sys.exit(result.returncode)
undefined = subprocess.run([str(sdk / 'bin/arm-vita-eabi-nm'), '-u', str(output)],
                           text=True, capture_output=True, check=True).stdout
(directory / 'game-imports.txt').write_text(undefined)
print(f'Aggregated {len(objects)} ARM32 game units; {len(undefined.splitlines())} unresolved CRT/platform imports.')
print('This is COMPILES evidence only, not LINKS for the complete game.')
