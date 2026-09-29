# AGENTS.md — HaloCEVita persistent instructions

These instructions apply to Codex and any other coding agent working in this repository.

## Mission

Port the source-port/decompilation of Halo: Combat Evolved to PlayStation Vita as a **native Vita homebrew executable/VPK**, preserving original gameplay and game-data compatibility as much as practical.

Do not redesign Halo, recreate it in another engine, or replace large working upstream systems without evidence that they cannot be adapted.

## Mandatory workflow

Before changing code:

1. Read `README.md`.
2. Read `docs/STATUS.md`.
3. Read the latest entries in `docs/ATTEMPTS.md`.
4. Read `docs/KNOWN_ISSUES.md` and `docs/DECISIONS.md`.
5. Inspect the exact upstream code involved before proposing a replacement.

After every significant experiment:

1. Update `docs/ATTEMPTS.md` with the command, hypothesis, changes, result and logs/error signature.
2. Update `docs/STATUS.md` if a subsystem changed state.
3. Update `docs/KNOWN_ISSUES.md` for unresolved reproducible problems.
4. Update `docs/DECISIONS.md` if an architectural choice was made.
5. Never erase a failed experiment from history merely because a later one works.

## Porting strategy

- Vita is a native 32-bit ARM target. Prefer a direct `arm-vita-eabi` build.
- Preserve the game's 32-bit pointer/layout assumptions.
- Do **not** copy the Android arm64 ILP32 host/guest loader unless a Vita-specific blocker genuinely requires it.
- Use the Linux native port as the primary platform-layer reference.
- Use the Android port as the primary reference for ARM issues, GLES-oriented renderer fallbacks and ABI fixes.
- Preserve upstream separation: Vita-specific code belongs under `port/vita/` or guarded by a clear `HALO_VITA` path.
- Avoid invasive edits to shared game code. If shared edits are required, explain why in `docs/DECISIONS.md`.

## Graphics strategy

Preferred route:

`Xbox D3D8/NV2A state -> upstream d3d8_gl/nv2a shader translation -> Vita-compatible GL subset -> vitaGL -> SceGxm`

- Reuse upstream shader translators (`nv2a_vsh.c`, `nv2a_psh.c`) where possible.
- Prefer adapting generated GLSL to vitaGL's supported translator subset over rewriting the entire Halo shader system.
- Use `libshacccg.suprx`/VitaShaRK when required by vitaGL runtime shader compilation.
- Treat unsupported GL calls one by one. Add wrappers/fallbacks and document them in `docs/GRAPHICS_COMPATIBILITY.md`.
- Do not silently stub a rendering function if it affects correctness. A temporary stub must log itself and be recorded as a known issue.

## Memory and performance

- Do not carry Android buffer sizes into Vita blindly.
- Keep memory budgets explicit. Vita has limited RAM/VRAM compared with modern Android devices.
- Start with correctness and observability, then optimize.
- Target 30 FPS first, matching the original Xbox design goal. Do not optimize for 60 FPS before a stable 30 FPS path exists.
- Record changes to stream/index buffer sizes and measured consequences.

## Build outputs

The intended outputs are:

```text
build/vita/eboot.bin
build/vita/HaloCE.vpk
```

Debug builds should retain useful symbols/logging when practical.

## Runtime data

Default target path:

```text
ux0:data/HaloCE/
```

Do not package proprietary `maps/` data in the VPK.

## Logging

Create a Vita-side log as early as possible, preferably:

```text
ux0:data/HaloCE/debug.txt
```

Startup milestones should be logged individually so a black-screen/crash can be localized without guessing.

## Git discipline

- Do not rewrite history.
- Do not delete upstream source merely to make the build pass.
- Keep commits focused by milestone when practical.
- Do not commit build artifacts, extracted game data, SDK binaries, `libshacccg.suprx`, secrets or copyrighted retail assets.
- If importing a newer upstream version, record the source commit in `docs/UPSTREAM.md`.

## Definition of progress

A change is not considered working merely because it compiles. Mark a feature working only when there is reproducible evidence at the appropriate level:

- `COMPILES`: builds successfully;
- `LINKS`: final Vita ELF/VPK links;
- `BOOTS`: launches on Vita/Vita3K far enough to produce expected logs;
- `RENDERS`: visible expected graphics;
- `PLAYABLE`: interaction/gameplay verified;
- `STABLE`: repeated tests without the known failure.

Use these terms consistently in `docs/STATUS.md`.

## Do not repeat known failures

Before trying a workaround, search `docs/ATTEMPTS.md` and `docs/KNOWN_ISSUES.md` for the same compiler error, GL call, crash signature or architectural idea.

If retrying a failed idea because conditions changed, explicitly state what changed and why the retry is justified.
