# HaloCEVita

Independent PlayStation Vita porting workspace for **Halo: Combat Evolved**, based on the source-port work from [`cybersecurity/halo-ce-universal`](https://github.com/cybersecurity/halo-ce-universal).

This repository is intentionally **not a GitHub fork**. It is a separate Vita-focused project with its own history, build system, documentation, experiments and releases. The original project remains the upstream technical reference.

## Project goal

Current milestone (2026-10-03): **00.39 original-renderer audit, CI Build277 verified LINKS; console acceptance pending**. Shipping selects original UI/input/events, original menu/new-map owners and pregame rendering. This audit restores upstream indexed-base semantics, preserves exact NV2A equations/register indices at the native shader boundary, and removes the independent Vita DXT1 decoder in favor of original DXT1/3/5 decoding. All26 required host contracts, exact-SDK native package verification and publication pass in [Vita Build277](https://github.com/VegettoSan/HaloCEVita/releases/tag/vita-build-277). Reported white panels/text placement and full gameplay remain unaccepted. See [function comparison and evidence](docs/RENDERER_AUDIT_00.39.md) and [exact CI package/test instructions](docs/BUILD.md).

Historical root package **00.11: BOOTS** (A031) creates the original nine-widget Main Menu root on real Vita after original handlers 86/23 complete. The vitaGL logo remains because original UI update/render and D3D8 frame submission are still pending; no Halo draw yet. The preserved **00.10: BOOTS** baseline validates the original Xbox `D:` file-reference/XDemos contract, UI mount/recheck/cleanup and Start exit. See [original Main Menu call graph](docs/MAIN_MENU_CALL_GRAPH.md).

Historical **00.21** exposed texture-transfer lifetime and native uniform-handle defects;00.22 corrected those contracts and subsequently showed a partial menu.00.24 restores the missing original presentation worker;00.25 also selects each texture stage before upload (A072), preserving original authored pixels/shaders. This binding defect is not proven to explain all white backgrounds or missing labels. See [status](docs/STATUS.md) and [attempt evidence](docs/ATTEMPTS.md).

Earlier **00.06: BOOTS** (A018) established the native96MiB Xbox-offset arena, original physical/game memory allocation and compressed ui/a10 tag reads. Hardware013/014 and both015/016 checkpoints passed: ui983/a10 3357 tags with matching CRCs. Its package/symbols remain in build/vita/attempts/a018-baseline-00.06/; newer00.07 hardware evidence is described above.

Package correction **00.02** (A012): 00.01 was rejected by VitaShell with `0x8010113D`. Opaque indexed PNG-8 repaired the packaging; subsequent user logs establish boot (A013). The unchanged older executable banner still said 00.01; 00.03 derives the banner and SFO from the same build version.

Build a native PlayStation Vita version of Halo CE from the decompiled/source-port code, targeting VitaSDK and the Vita's native 32-bit ARM environment.

The preferred architecture is:

- native ARMv7/32-bit Vita executable;
- VitaSDK standard/native toolchain;
- reuse the upstream Linux platform layer where practical;
- reuse Android ARM/ABI fixes where they solve non-x86 assumptions;
- graphics through vitaGL/VitaShaRK/libshacccg where feasible;
- SDL3 or direct Vita APIs for platform services depending on actual compatibility;
- no Android loader/host-guest architecture unless a concrete blocker proves it necessary.

## Legal / data policy

This repository must not contain proprietary Halo game data, Xbox disc images, extracted retail assets, keys or other copyrighted game content that is not already legitimately part of the upstream source repository.

The Vita build should load user-provided game data from a path such as:

```text
ux0:data/HaloCE/
├── maps/
├── save/
└── config.toml
```

Users are responsible for supplying data from a legally obtained copy of the game.

## Upstream

Primary upstream:

- https://github.com/cybersecurity/halo-ce-universal

Upstream ancestry/reference projects:

- https://github.com/bnunu/halo-1
- https://github.com/punpckhdq/halo

See [`docs/UPSTREAM.md`](docs/UPSTREAM.md) before importing or synchronizing code.

## Where to start

Humans and coding agents should read these files in order:

1. [`AGENTS.md`](AGENTS.md)
2. [`docs/STATUS.md`](docs/STATUS.md)
3. [`docs/ROADMAP.md`](docs/ROADMAP.md)
4. [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
5. [`docs/BUILD.md`](docs/BUILD.md)
6. [`docs/ATTEMPTS.md`](docs/ATTEMPTS.md)
7. [`docs/KNOWN_ISSUES.md`](docs/KNOWN_ISSUES.md)
8. [`docs/DECISIONS.md`](docs/DECISIONS.md)

The first Codex task is stored in [`prompts/01-first-vita-port.md`](prompts/01-first-vita-port.md).

## Development rule

Every meaningful experiment must leave evidence in the repository. A failed attempt is useful only if we record **what was tried, why, the exact result and what not to repeat**.
