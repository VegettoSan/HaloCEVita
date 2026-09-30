# HaloCEVita

Independent PlayStation Vita porting workspace for **Halo: Combat Evolved**, based on the source-port work from [`cybersecurity/halo-ce-universal`](https://github.com/cybersecurity/halo-ce-universal).

This repository is intentionally **not a GitHub fork**. It is a separate Vita-focused project with its own history, build system, documentation, experiments and releases. The original project remains the upstream technical reference.

## Project goal

Current project milestone (2026-09-29): **RENDERS diagnostic + synthetic NV2A triangle on real Vita**. In **00.05**, both original NV2A-generated shaders compile, link succeeds and the user sees the triangle; controls/header rechecks remain responsive and Start exits at34.3s (A016). The unsupported invariant declaration is omitted only on Vita; multipass position invariance remains unverified. Full Halo rendering/main/menu/gameplay and repeated stability are not demonstrated. All466 configured game C units compile separately. Follow [the build and console test instructions](docs/BUILD.md).

Current package **00.07: LINKS** (A019) prepares real Main Menu tag access: typed relocation of widget/text/font/bitmap metadata and original Halo accessors. Host ui.map and its nine-widget menu graph pass; the new ARM checkpoint awaits hardware. Full UI/scenario/BSP/resource initialization and original D3D8 binding remain pending; no menu state or Halo draw yet.

Hardware-tested baseline **00.06: BOOTS** (A018). It adds a native 96MiB Xbox-offset arena, original physical-memory/game-state allocation and read-only streaming of compressed ui/a10 tag sections. Original tag APIs exercise a checked, temporary directory mount; nested tag/BSP/GPU relocation and full renderer/main/menu remain pending. Host tests pass on the actual reader and user-owned ui/a10 maps. Hardware013/014 and both015/016 checkpoints pass: native96MiB arena, ui983/a10 3357 tags with matching CRCs. The tested00.06 package/symbols are preserved in build/vita/attempts/a018-baseline-00.06/.

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
