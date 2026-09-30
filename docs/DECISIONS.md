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

## D008 — Checked eager shader-pair compilation during bring-up

Date: 2026-09-29; evidence A014/KI-014.

Decision: compile each upstream-generated vertex/fragment pair in VGL_MODE_SHADER_PAIR, in that order, finish both stages even after rejection, and validate actual status before attach/link. Capture compiler messages through the existing VitaShaRK logging API and dump retained failed Cg. Rejection keeps the logged diagnostic running and does not pretend the renderer works.

Reason: 00.03's dump identifies a GXM parameter query inside deferred glLinkProgram. POSTPONED reports compile success before compilation and its link path fails to recheck NULL programs after compilation. The known pair meets the documented SHADER_PAIR contract and exposes real compiler results without changing SDK binaries or replacing NV2A translation. Before full renderer integration, enforce this pair ordering in its shader cache/creation path or use a separately validated semantic policy. Hardware recovery and actual shader acceptance remain pending.

## D009 — Explicit Vita limitation for the invariant qualifier

Date: 2026-09-29; evidence A015/KI-015.

Decision: exclude `invariant gl_Position;` only under HALO_VITA in the existing upstream vertex prologue. Preserve it for desktop/Android and keep the original position arithmetic and NV2A translation. Log the unsupported qualifier and record cross-program multipass invariance as unverified.

Reason: the real compiler rejects that exact untranslated GLSL declaration in the supplied Cg at line177. A guarded shared-source change is needed because the original generator owns the declaration; editing the SDK or replacing the translator is unnecessary. This removes a proven syntax blocker, not a fidelity guarantee. Compare rendered depth/position across actual multipass shaders before enabling that claim, and avoid an unverified substitute pragma.

Validation A016: the 00.05 hardware log shows both compiles and link succeed; user confirms the triangle and Start clean exit. This validates the isolated probe and eager pair policy in D008. It does not validate the multipass invariance guarantee or full Halo shader coverage.

## D010 — Native movable Xbox-offset arena and restricted directory checkpoint

Date:2026-09-29; evidence A017/KI-007/KI-016.

Decision: allocate one96MiB USER_RW kernel block outside the64MiB newlib heap. Retain Xbox physical offsets in the original placed/top-down allocation policy, translate original virtual addresses to the native base, and use addition/subtraction for resource-address conversion. Preserve the native upstream16MiB game state and22MiB tag/texture plus4MiB sound budgets; the remaining32MiB includes holes and later resource allocations. This is a CPU arena; cached memory is not advertised as direct GXM memory. vitaGL RAM16MiB/CDRAM24MiB budgets stay unchanged. Do not copy Android's ILP32 loader or128MiB host virtual mapping.

Reason: the public VitaSDK allocation interface does not supply Linux mmap's fixed VA reservation; the original0x803A6000..0x819A6000 tag range overlaps the observed Vita executable segment. Native pointers cannot preserve that VA. Shared edits are narrowly HALO_VITA-guarded in physical_memory_map.c (expected native addresses), game_state_xbox.c (CPU/GPU split and state bookkeeping on translated address), cache_files.c (native protection address and temporary checked directory mount), and platform.h (offset conversion). Other targets retain their old behavior. Original game layouts and allocation functions remain intact.

The initial map integration reads Xbox-v5 tag sections in place, streaming zlib with64KiB scratch and validating signature, bounds, datum/name/root/scenario/resource directories. It temporarily translates the directory/name/root pointers to exercise original tag_iterator/tag_get APIs and detaches before returning. Nested tag/BSP/GPU pointers are explicitly inactive; do not guess pointer locations by scanning arbitrary32-bit values, and do not call full main with this checkpoint mount. No third-party tag schemas are imported into the executable. Complete typed relocation and renderer/platform integration remain necessary.

Protection policy: USER_RW cannot emulate the Linux SIGSEGV dirty-page mechanism. Allocation queries track owned RW pages; VirtualProtect returns failure and logs for READONLY/NOACCESS rather than claiming enforcement. No renderer dirty-cache correctness is established by these metadata queries. Kernel allocation, ARM execution and real tag API checkpoint require a Vita test even though host allocator/reader tests pass.
