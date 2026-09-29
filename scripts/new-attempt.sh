#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -lt 2 ]; then
  echo "usage: $0 A### \"Short title\"" >&2
  exit 2
fi
id="$1"
title="$2"
date="$(date +%F)"
cat >> docs/ATTEMPTS.md <<EOT

## ${date} — ${id} — ${title}

**Goal:**

**Hypothesis:**

**Environment:**

**Commit:**

**Commands:**

\`\`\`bash

\`\`\`

**Changes:**

**Result:** PENDING

**Evidence:**

\`\`\`text

\`\`\`

**Conclusion:**

**Do not repeat:**

**Next:**
EOT
printf 'Added %s to docs/ATTEMPTS.md\n' "$id"
