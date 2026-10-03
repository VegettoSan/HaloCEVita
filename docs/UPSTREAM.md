# Upstream management

## Primary upstream

`https://github.com/cybersecurity/halo-ce-universal`

This project must remain a separate GitHub repository, not a GitHub fork.

## Initial reference observed during project setup

The upstream repository was actively changing on 2026-09-28. The Vita repository must record the **actual commit imported on the developer PC**, not rely permanently on a commit mentioned in chat or documentation.

After importing source, replace the placeholder below:

Verified during A001: this SHA is already imported; `.upstream-base-sha` matches, all required source/platform/build directories exist, and no reimport was performed. HaloCEVita's independent baseline HEAD was `080e0c5b2eca00e809f5b2c5b70fbbd004ca8f4a`. Import helper excluded platform README files; the preserved upstream root README is `docs/upstream/README.md`; Linux/Android source/build scripts were inspected directly.

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


## Focused native Vita platform reuse (2026-10-02, V5 reintegration)

Source reference BirchWoodGod/halo-ce-vita at5ceb8e89f2c1219046d070a3f306348a5da30c85 supplies the native SceNet socket adapter (host/vita_net.c, excluding diagnostic probes) and ARMv7 fenv/MSVC control-word adaptation. The socket unit now logs through our native logger. No donor frontend, executable, maps, GXM renderer or shaders are imported. These adapted portions retain GPL-3.0-only attribution; full source license is LICENSES/BirchWoodGod-GPL-3.0.txt. The combined executable containing them is distributed under GPL-3.0, with this public repository providing its corresponding modified source; existing compatible permissive notices remain intact.

## Focused original cache/decompression audit (2026-10-03)

This is a **reference audit, not a new blanket source import**. The imported baseline above remains `21714ac0860e9b9ca08fbdc8a1d620f1b8a03797` until a deliberate upstream synchronization is performed.

```text
CACHE_AUDIT_REPOSITORY=https://github.com/cybersecurity/halo-ce-universal
CACHE_AUDIT_COMMIT=80d30410c8db28f4008b92f4e012a1b046ece14e
CACHE_AUDIT_DATE=2026-10-03
```

Direct upstream files inspected at that exact commit:

- `source/cache/cache_files.c`
- `source/cache/cache_files_windows.c`
- `source/cache/cache_files_decompress_windows.c`
- `port/linux/port.json`
- `port/linux/src/xbox_files.c`

The audit confirms that original Halo separates source maps on `d:\maps` from six persistent writable `z:\cacheNNN.map` slots. Main Menu uses slot 2; solo uses slots 0–1; multiplayer uses slots 3–5. The decompressor invalidates the destination header first, writes/inflates the payload beginning at offset `0x800`, waits for successful payload completion, then commits the original source `0x800` header at offset zero last. `cache_file_open/read` consume a prepared logical cache rather than implicitly materializing a temporary file while opening the compressed source.

Current upstream Linux keeps the original cache/precache/decompression source closure and maps Xbox drive paths to host storage; it does not use HaloCEVita's historical `<map>.vita-logical.tmp` binding model.

HaloCEVita now follows that storage/lifetime contract through Vita-native synchronous file I/O and persistent `ux0:data/HaloCE/cache000.map` .. `cache005.map`. Exact findings, supplied `ui.map` read-only validation, intentional Vita deviations and host regression gates are recorded in `docs/CACHE_UPSTREAM_AUDIT.md`.
