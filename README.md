# HaloCEVita

Independent PlayStation Vita porting workspace for **Halo: Combat Evolved**, based on the source-port work from [`cybersecurity/halo-ce-universal`](https://github.com/cybersecurity/halo-ce-universal).

This repository is intentionally **not a GitHub fork**. It is a separate Vita-focused project with its own history, build system, documentation, experiments and releases. The original project remains the upstream technical reference.

## Project goal

Current project milestone (2026-09-29): **RENDERS diagnostic + synthetic NV2A triangle on real Vita**. In **00.05**, both original NV2A-generated shaders compile, link succeeds and the user sees the triangle; controls/header rechecks remain responsive and Start exits at34.3s (A016). The unsupported invariant declaration is omitted only on Vita; multipass position invariance remains unverified. Full Halo rendering/main/menu/gameplay and repeated stability are not demonstrated. All466 configured game C units compile separately. Follow [the build and console test instructions](docs/BUILD.md).

Historical root package **00.11: BOOTS** (A031) creates the original nine-widget Main Menu root on real Vita after original handlers 86/23 complete. The vitaGL logo remains because original UI update/render and D3D8 frame submission are still pending; no Halo draw yet. The preserved **00.10: BOOTS** baseline validates the original Xbox `D:` file-reference/XDemos contract, UI mount/recheck/cleanup and Start exit. See [original Main Menu call graph](docs/MAIN_MENU_CALL_GRAPH.md).

Current hardware **00.20: BOOTS / HALO DRAW REACHED** (A058): actual320x240 screen color/depth, original640x480 coordinates, real shield-noise upload and first original Halo draw return error0. Native960x544 remains final4:3 presentation. Exact workflow121 ELF/core identifies a later NULL buffer-clone allocation in vitaGL SubData during original menu font rendering (KI-028). No Present/visible Halo menu yet. Current **00.21** adapts checked synchronized vertex/index mapping/reuse; host lifetime/guard-page tests PASS, native workflow and visible console image pending. Original shaders/resources/menu remain intact. See [status](docs/STATUS.md) and [attempt evidence](docs/ATTEMPTS.md).

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
