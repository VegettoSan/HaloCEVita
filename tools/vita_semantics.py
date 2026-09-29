#!/usr/bin/env python3
"""Use upstream MSVC tag/COMDAT scanning with GCC's weak-linkage rules."""
from pathlib import Path
import re
import sys
from linux_msvc_semantics import read, render, scan_inline_functions, scan_tags, source_files

files = source_files([Path('source'), Path('port/include/xdk')])
inlines = scan_inline_functions(files, True)
# GCC errors on #pragma weak followed by an explicitly static definition.
static_inline = re.compile(r'\bstatic\s+(?:__inline|_inline|__forceinline)\b'
                           r'[^;{}()]*?\b([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{')
private = set()
for path in files:
    private.update(static_inline.findall(read(path)))
output = Path(sys.argv[1])
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(render(scan_tags(files), inlines - private))
