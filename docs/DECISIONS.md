# Architectural decisions

This is a lightweight ADR log. Append; do not silently reverse decisions.

## D001 — Independent repository, not a GitHub fork

Date: 2026-09-28

Decision: maintain HaloCEVita as a separate repository with its own history.

Reason: Vita work is expected to develop a distinct build/platform layer and should not be constrained by fork presentation/history. Upstream remains a remote/reference source.

## D002 — Native 32-bit Vita port first

Date: 2026-09-28

Decision: target native `arm-vita-eabi` code rather than Android's AArch64 ILP32 guest/host architecture.

Reason: Vita is already a 32-bit ARM platform, while the Android guest/host architecture primarily solves the conflict between Halo's 32-bit layout and modern 64-bit-only Android.

Revisit only if a concrete memory/layout/toolchain blocker proves the direct approach unworkable.

## D003 — Reuse upstream NV2A/D3D8 translation

Date: 2026-09-28

Decision: adapt the existing GL renderer/shader translation to vitaGL rather than recreating Halo materials/rendering from scratch.

Reason: upstream already implements the hard semantic translation from Xbox D3D8/NV2A state to shaders and GL state.

## D004 — 30 FPS before 60 FPS

Date: 2026-09-28

Decision: initial performance target is stable 30 FPS.

Reason: matches the original console design target and reduces premature optimization during bring-up.

## D005 — GCC native build and real-core bring-up closure

Date: 2026-09-28

Decision: use existing `/usr/local/vitasdk-hardfp` GCC 15.2 native ARM32 hard-float compiler and VitaSDK packaging. `port/vita/CMakeLists.txt` owns the build; root `configure.py`/Ninja delegate Vita targets to it. No SDK changes, host/guest loader, engine rewrite or deleted modules.

Reason: GCC successfully compiles the existing source with the upstream generated MSVC forward-tag/weak-inline semantics plus small isolated GCC adaptations. The full Halo main depends on an unimplemented Vita physical-memory contract, D3D8 resource layer, filesystem/XAPI and renderer compatibility. First executable links and calls actual cseries/debug heap/profile/data/pool/CRC/cache-header verifier/NV2A translation. It explicitly withholds milestone 010 and gameplay claims. All other game units are independently compiled and retained for incremental integration; linker gc-sections limits the *bring-up executable*, not the repository's engine.

Shared edits are guarded by HALO_VITA: terminal_printf uses real va_list; byte_swapping uses GCC bswap64 instead of MSVC ui64 constants; old libtiff sees libc malloc prototypes; ai_debug uses consistent private linkage; debug_memory diagnostic supplies csprintf's missing destination; hs.c reuses Android's real-valued fade signatures. The Linux architecture guard admits HALO_VITA. NV2A generators only adapt GLSL dialect/output conventions and report unsupported packed/3D paths. These fixes preserve original branches for upstream targets.

## D006 — Explicit game/SDK ABI boundary and conservative budgets

Date: 2026-09-28

Decision: game uses pointer32/long32/wchar16/enum32/signed char with strict aliasing disabled, wrapping integers, no FP contraction and retained frames. Native Vita/newlib/SDL3/vitaGL units retain installed SDK wchar/enum conventions. Bridge arguments have explicit 32/64-bit scalar sizes; wide text/SDK-enum-containing structures do not cross it. GNU ld's wchar/enum warnings remain visible and documented.

Initial probe budgets: 64MiB cap for newlib heap, vitaGL 16 MiB RAM + 24 MiB CDRAM, 2MiB legacy pool, no phycont pool and 96-byte probe VBO. No Android or desktop streaming rings allocated. No claimed memory/FPS measurements before hardware testing. Full-stream budget must be chosen with overflow/flush behavior when actual Halo draws begin.

TITLE_ID `HCEV00001`, APP_VER `00.01`, diagnostic title `Halo CE Vita`: project-specific homebrew identifier, not copied from commercial software. Package no maps, saves, retail artwork, firmware or libshacccg.

Initial dormant full-renderer stream defaults: 2 MiB stream, 256 KiB indices, ring 1. Android's 54 MiB total and desktop 40 MiB are not carried into Vita. These constants compile but full renderer is not linked; no claim that these sizes suffice for campaign geometry or that its persistent-map strategy works. Implement overflow/flush behavior before enabling them for real draws.

## D007 — Indexed opaque LiveArea packaging contract

Date: 2026-09-28; evidence A012/KI-012.

Decision: generate all authored installer images directly as PNG-8/type 3, two-color RGB palette, no alpha/tRNS. Their artwork needs only two colors, so a deterministic lossless generator avoids adding a quantizer dependency. Verify all images including their PNG chunks, palette indices and decompressed rows before handing over a package. Preserve TITLE_ID HCEV00001 and increment package APP_VER to 00.02; engine bytes and ABI are unchanged.

Reason: 00.01 passed generic PNG/ZIP checks but was rejected by VitaShell with 0x8010113D. The platform's LiveArea image contract is stricter than generic PNG validity; the verifier must enforce the actual generated asset format. Corrected installation remains subject to a hardware retry.
