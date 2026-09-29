# Attempt log — append only

Do not rewrite old failed attempts. Add a new entry when a later change supersedes one.

## 2026-09-28 — A000 — Project scaffold

**Goal:** create a persistent Vita-specific engineering workspace before code changes begin.

**Hypothesis:** a structured status/attempt/decision system will prevent repeated dead ends across Codex, ChatGPT and manual work.

**Changes:** created repository documentation, Vita directory placeholders, environment rules, roadmap and first Codex prompt.

**Result:** SUCCESS — organizational milestone only. No Vita source build attempted yet.

**Next:** import upstream source and record its exact SHA, then start A001 with a native ARM compile audit.

## 2026-09-28 — A000.1 — WSL script CRLF bootstrap failure

**Goal:** run `scripts/import-upstream.sh` under Ubuntu WSL.

**Observed error:** `/usr/bin/env: 'bash\r': No such file or directory` (rendered by WSL as `env: $'bash\r': No such file or directory`).

**Cause:** the local checkout converted shell-script line endings to Windows CRLF, so the shebang was parsed as `#!/usr/bin/env bash\r`.

**Fix:** enforce LF for shell/build scripts through `.gitattributes`; existing affected local checkouts can be repaired with `sed -i 's/\r$//' scripts/*.sh`.

**Result:** ROOT CAUSE IDENTIFIED / REPOSITORY POLICY FIXED. Runtime import should be retried after local normalization.

**Do not repeat:** do not debug Bash, `env`, PATH, or VitaSDK for this error until checking line endings first.

**Next:** normalize the current WSL checkout and rerun the upstream import.
