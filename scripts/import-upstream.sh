#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "usage: $0 /path/to/halo-ce-universal" >&2
  exit 2
fi

src="$(cd "$1" && pwd)"
dst="$(pwd)"

if [ ! -d "$src/.git" ]; then
  echo "error: $src is not a Git checkout" >&2
  exit 1
fi
if [ ! -f "$src/configure.py" ] || [ ! -d "$src/source" ] || [ ! -d "$src/port/linux" ]; then
  echo "error: source does not look like halo-ce-universal" >&2
  exit 1
fi

sha="$(git -C "$src" rev-parse HEAD)"
date_iso="$(date +%F)"

printf 'Importing upstream %s\n' "$sha"

# Copy the upstream working tree but protect Vita-project-owned control files
# and directories. No --delete is used: upstream import/update must never
# remove port/vita or the persistent engineering history.
rsync -a \
  --exclude='.git/' \
  --exclude='README.md' \
  --exclude='AGENTS.md' \
  --exclude='.gitignore' \
  --exclude='.gitattributes' \
  --exclude='LICENSE' \
  --exclude='docs/' \
  --exclude='prompts/' \
  --exclude='templates/' \
  --exclude='scripts/' \
  --exclude='port/vita/' \
  "$src/" "$dst/"

# Keep upstream licensing separately without replacing HaloCEVita's own root LICENSE.
if [ -f "$src/LICENSE.md" ]; then
  cp "$src/LICENSE.md" "$dst/LICENSE.md"
fi

mkdir -p docs/upstream
if [ -f "$src/README.md" ]; then
  cp "$src/README.md" docs/upstream/README.md
fi
if [ -d "$src/docs" ]; then
  rsync -a "$src/docs/" docs/upstream/
fi

python3 - "$sha" "$date_iso" <<'PY'
from pathlib import Path
import re, sys
sha, date_iso = sys.argv[1:]
p = Path('docs/UPSTREAM.md')
text = p.read_text(encoding='utf-8')
text = re.sub(r'UPSTREAM_COMMIT=.*', f'UPSTREAM_COMMIT={sha}', text)
text = re.sub(r'UPSTREAM_IMPORT_DATE=.*', f'UPSTREAM_IMPORT_DATE={date_iso}', text)
p.write_text(text, encoding='utf-8')
PY

printf '%s\n' "$sha" > .upstream-base-sha

echo "Upstream import complete. Review 'git status' before committing."
echo "Vita-owned paths were preserved."
