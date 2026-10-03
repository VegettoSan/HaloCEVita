#!/usr/bin/env python3
"""Validate the clean restart's inputs, without trusting earlier HaloCEVita code."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CODE_SUFFIXES = {'.c', '.h', '.cc', '.cpp', '.cxx', '.S', '.s', '.asm', '.py'}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(reference=None):
    manifest = json.loads((ROOT / 'tools/origins.json').read_text())
    assert manifest['schema'] == 1
    assert manifest['sources']['decomp']['repository'] == 'https://github.com/cybersecurity/halo-ce-universal'
    assert manifest['sources']['vita']['repository'] == 'https://github.com/BirchWoodGod/halo-ce-vita'
    records = manifest['files']
    project = manifest.get('project_files', {})
    for relative, item in records.items():
        assert item['origin'] in manifest['sources'], relative
        assert item['source_commit'] == manifest['sources'][item['origin']]['commit'], relative
        assert digest(ROOT / relative) == item['sha256'], 'Imported file changed without updating provenance: ' + relative
        if reference and item['origin'] == 'decomp':
            expected = item.get('source_sha256', item['sha256'])
            assert digest(reference / item['source_path']) == expected, 'Original source differs: ' + relative
    for relative, item in project.items():
        assert item['reason'], relative
        assert digest(ROOT / relative) == item['sha256'], 'Project file changed without updating provenance: ' + relative
    registered = set(records) | set(project)
    for directory in ('source', 'port', 'tools'):
        for path in (ROOT / directory).rglob('*'):
            if path.is_file() and path.suffix in CODE_SUFFIXES and '__pycache__' not in path.parts:
                relative = path.relative_to(ROOT).as_posix()
                assert relative in registered, 'Unregistered code in the new base: ' + relative
    for relative in ('port/vita/src/main.c', 'port/vita/src/halo_ui_original.c',
                     'port/vita/src/vita_gl_compat.c', 'tools/vita_verify.py'):
        assert not (ROOT / relative).exists(), 'Discarded implementation returned: ' + relative
    print(f'PASS origins: {len(records)} source imports, {len(project)} new project files; no unregistered code')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--reference', type=Path, help='Direct checkout of the pinned original decomp')
    args = parser.parse_args()
    verify(args.reference)
