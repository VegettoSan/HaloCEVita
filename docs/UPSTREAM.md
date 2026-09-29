# Upstream management

## Primary upstream

`https://github.com/cybersecurity/halo-ce-universal`

This project must remain a separate GitHub repository, not a GitHub fork.

## Initial reference observed during project setup

The upstream repository was actively changing on 2026-09-28. The Vita repository must record the **actual commit imported on the developer PC**, not rely permanently on a commit mentioned in chat or documentation.

After importing source, replace the placeholder below:

```text
UPSTREAM_REPOSITORY=https://github.com/cybersecurity/halo-ce-universal
UPSTREAM_COMMIT=21714ac0860e9b9ca08fbdc8a1d620f1b8a03797
UPSTREAM_IMPORT_DATE=2026-09-28
```

## Initial import procedure

Given sibling directories:

```text
workspace/
├── halo-ce-universal/   # normal upstream clone
└── HaloCEVita/          # this independent repository
```

From `HaloCEVita`, use the repository helper:

```bash
./scripts/import-upstream.sh ../halo-ce-universal
```

The helper intentionally **does not use `rsync --delete`**. It protects this project's `README.md`, `AGENTS.md`, engineering documentation, prompts, templates, scripts and `port/vita/`. It also records the imported upstream commit.

After import:

```bash
git status
git diff -- docs/UPSTREAM.md
git add .
git commit -m "Import halo-ce-universal baseline"
```

Do not copy the upstream `.git` directory. The GitHub repository must remain independent.

## Keeping an upstream remote without becoming a GitHub fork

A Git remote does not make the GitHub repository a fork. It is acceptable to add:

```bash
git remote add upstream https://github.com/cybersecurity/halo-ce-universal.git
git fetch upstream
```

Do not merge upstream blindly once Vita-specific changes exist. Prefer:

1. inspect upstream changes;
2. merge/rebase in a dedicated branch;
3. resolve Vita-specific conflicts intentionally;
4. run at least the current Vita compile/link test;
5. update this file with the new upstream SHA;
6. record regressions in `docs/ATTEMPTS.md`.

## Licensing

Preserve upstream `LICENSE.md` and all third-party licenses/notices when source is imported. Do not remove third-party attribution simply because the root project uses CC0.
