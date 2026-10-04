"""Audit the original renderer against a direct, pinned official vitaGL checkout.

Export presence is not a claim that the function implements its full desktop
semantics. Missing functions remain blockers; no success stubs are generated.
"""
import argparse
import json
import re
import subprocess
from pathlib import Path

EXPECTED_VITAGL = '1ae86f65718675b797d3cd8ffc021ad2527096cb'


def inspect(checkout):
    sha = subprocess.check_output(['git', '-C', str(checkout), 'rev-parse', 'HEAD'], text=True).strip()
    if sha != EXPECTED_VITAGL:
        raise SystemExit('vitaGL checkout differs from pinned source: ' + sha)
    loader = (checkout / 'source/lookup.c').read_text()
    if re.search(r'^\s*#\s*define\s+FAKE_UNRESOLVED_FUNCS', loader, re.M):
        raise SystemExit('Bogus unresolved-function mapping is forbidden')
    exports = set(re.findall(r'\{"(gl\w+)",\s*\(void \*\)', loader))
    header = Path('port/linux/src/gl.h').read_text()
    blocks = re.findall(r'#define GL_FUNCTIONS\(X\) \\\n(.*?)(?=\n[^\t])', header, re.S)
    assert len(blocks) == 2, 'Original GL function list structure changed'
    required = set(re.findall(r'X\((gl\w+)\)', blocks[1]))
    return {'vitagl_commit': sha, 'original_desktop_entry_count': len(required),
            'exported': sorted(required & exports), 'missing': sorted(required - exports),
            'status': 'BLOCKED' if required - exports else 'EXPORTS_PRESENT',
            'semantic_validation': 'PENDING; exports alone do not prove renderer correctness'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--vitagl', type=Path, required=True)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    report = inspect(args.vitagl)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(f"Renderer contract {report['status']}: {len(report['exported'])}/{report['original_desktop_entry_count']} entry points exported")
    print('Missing: ' + ', '.join(report['missing']))
