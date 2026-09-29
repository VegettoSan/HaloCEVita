# Codex task — First native PlayStation Vita port

Read `AGENTS.md` and every document it marks as mandatory before modifying code.

Your task is to turn this repository into the first functioning native PlayStation Vita build of Halo CE, based on the imported `cybersecurity/halo-ce-universal` source.

Do not stop at an architectural proposal. Inspect the source, make the changes, invoke the toolchain, iterate through compiler/linker errors, and produce the furthest objectively testable Vita artifact possible in this run.

## Primary goal

Produce:

```text
build/vita/eboot.bin
build/vita/HaloCE.vpk
```

The ideal first result is a VPK that boots on PS Vita, writes startup milestones to `ux0:data/HaloCE/debug.txt`, finds the user-provided `maps/` directory and progresses into Halo initialization. If rendering can also be brought up safely, continue toward the main menu.

If a complete menu is not achievable in the current run, still leave a reproducible VPK at the furthest valid milestone and document the exact remaining blocker. Do not fake success with a dummy app unless it is an explicitly documented temporary bring-up milestone that still links real Halo initialization code.

## Non-negotiable architecture

1. Target Vita natively with the standard VitaSDK `arm-vita-eabi` toolchain.
2. Preserve Halo's 32-bit data/pointer/layout assumptions.
3. Do not port Android's AArch64 ILP32 guest/host loader by default. Vita is already 32-bit ARM.
4. Use `port/linux/` as the primary native platform reference.
5. Use `port/android/` for ARM fixes, GLES fallbacks and ABI lessons.
6. Keep Vita-specific implementation in `port/vita/` and/or well-scoped `HALO_VITA` conditionals.
7. Do not delete or bypass major game systems merely to obtain a link.
8. Do not include proprietary Halo maps/assets in the VPK.

## First actions

1. Verify the source import is present. If `source/`, `port/linux/` or core upstream files are missing, stop code changes and report that the import step from `docs/UPSTREAM.md` is required.
2. Run `scripts/check-env.sh` and append the environment facts to a new `docs/ATTEMPTS.md` entry.
3. Inspect the existing build generator (`configure.py`, `tools/linux_build.py`, `tools/android_build.py`) and choose the least invasive reproducible Vita build integration.
4. Prefer adding a first-class `vita` / `vita_vpk` Ninja target. A temporary `port/vita/CMakeLists.txt` is acceptable for initial bring-up only if integrating Ninja would significantly delay the first executable; document that decision.

## Compiler/ABI work

Audit and deliberately reproduce every Xbox/MSVC assumption that matters on ARM Vita, including:

- 32-bit pointers/longs where required by the source/data format;
- 16-bit wide characters where required;
- struct packing/alignment;
- Microsoft extensions used by the code;
- small struct/union returns and incompatible function declarations;
- variadic calls already fixed for Android;
- frame-pointer or stack-walking assumptions;
- floating-point contraction/rounding behavior;
- dangerous x86-specific inline assembly/intrinsics.

Reuse upstream compatibility generators/headers where practical. Add compile-time layout assertions for important structures rather than assuming ARM layout is correct.

Do not respond to compiler errors by globally suppressing them if they indicate real ABI/type problems.

## Platform layer

Implement only the Vita equivalents needed to advance startup, in this order:

1. process entry / exit;
2. logging;
3. paths/filesystem;
4. memory allocation and any fixed-address assumptions;
5. timing;
6. threading/synchronization;
7. input;
8. graphics bootstrap;
9. audio;
10. networking later.

Default writable root:

```text
ux0:data/HaloCE/
```

Create stable startup markers in `debug.txt`, following `docs/TESTING.md`.

## Graphics

Start from upstream's existing D3D8/NV2A -> OpenGL translation. Do not rewrite Halo's renderer from scratch.

Preferred path:

```text
Halo NV2A/D3D8
  -> upstream d3d8_gl.c / nv2a_vsh.c / nv2a_psh.c
  -> Vita-compatible GL calls / adapted generated GLSL
  -> vitaGL
  -> SceGxm
```

Audit each required GL function against the installed/current vitaGL headers. Update `docs/GRAPHICS_COMPATIBILITY.md` with direct/fallback/blocked status and evidence.

Known initial guidance:

- framebuffer/blit fallback should be preferred over `glCopyImageSubData` where necessary, similar to upstream Android;
- runtime generated GLSL should be tested through vitaGL/VitaShaRK instead of replacing all shaders manually;
- treat 3D textures as a known risk; first prove whether campaign content needs the path before designing a large emulation system;
- reduce stream/index buffer budgets for Vita instead of copying Android sizes;
- target correctness at 30 FPS before performance tuning.

A graphics API may be temporarily stubbed only if startup does not depend on its visual correctness, the stub emits a clear warning, and the limitation is recorded as a known issue.

## Packaging

Create a proper Vita homebrew package with a stable title ID chosen for this project, `eboot.bin`, LiveArea minimum assets if required, and VPK output.

Do not package:

- `maps/`;
- Xbox disc images;
- `libshacccg.suprx`;
- SDK binaries;
- user saves.

Document installation prerequisites in `docs/BUILD.md`.

## Testing and iteration

After each meaningful build attempt:

- append an attempt entry;
- include exact failing command and concise error signature;
- search previous attempts before trying a workaround;
- update subsystem status when evidence changes.

When you reach a VPK:

1. calculate its SHA-256;
2. state exactly what is expected to happen when launched;
3. state the expected data path;
4. list prerequisites such as `libshacccg.suprx` only if actually required by the current build;
5. identify the log path and startup marker sequence;
6. keep debug symbols/map files useful for resolving a Vita crash address.

## Completion response

At the end, report:

- what you changed;
- exact files added/modified;
- exact build command;
- whether `eboot.bin` exists;
- whether `HaloCE.vpk` exists;
- artifact path and SHA-256;
- highest achieved status: `LINKS`, `BOOTS`, `RENDERS`, etc.;
- what must be tested on real Vita;
- remaining blockers ordered by impact.

Do not claim the game boots/renders/plays unless hardware or emulator evidence proves that level. Compilation/linking alone is not runtime success.
