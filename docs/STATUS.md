# Project status

Last updated: 2026-09-29

Maximum demonstrated project state: **RENDERS diagnostic + synthetic NV2A triangle on real Vita** (A016). Tested baseline **00.05** compiles both generated shaders, links successfully and draws the triangle per user observation. It remains responsive through header/input tests and Start clean exit at34.3s. Full Halo rendering/menu/gameplay, multipass position invariance and repeated stability remain unverified.

Current package **00.06: LINKS** (A017), not yet tested on Vita. It integrates a 96MiB native Xbox-offset arena, original physical_memory_allocate/verify and game_state_allocate_buffer, plus streaming read/decompression and a restricted tag-directory mount using original tag APIs. Actual reader host tests:25 synthetic cases and both user maps pass (ui983 tags/a10 3357). Original allocator/state functions pass mocked-SDK lifecycle/overlap/alignment/reuse checks. Full nested tags/BSP/GPU resources and the full renderer/main/menu remain BLOCKED;010 is still withheld. Kernel reservation and this new checkpoint need hardware evidence.

Hardware feedback: A012 repaired installer images, A013 fixed the init flag, A014/A015 expose and recover vertex rejection, and A016 verifies the invariant-declaration fix on hardware. 00.05 vertex1/pixel1/link1, compile GL errors0, GPU-copy PASS,011 shaders1, visible triangle and clean exit. Hardware GLSL differs from00.04 only by the removed invariant line; fragment is byte-identical. No engine/SDK/pool rewrite. Host114-case comparisons per backend preserve all arithmetic and desktop/Android output. Runtime/SFO versions agree; broader retail shader coverage remains untested.

Status vocabulary: `COMPILES`, `LINKS`, `BOOTS`, `RENDERS`, `PLAYABLE`, `STABLE`, `BLOCKED`.

| Area | Status | Evidence / next requirement |
|---|---|---|
| Upstream source / independence | COMPILES | Existing independent import verified: 21714ac0860e9b9ca08fbdc8a1d620f1b8a03797; no reimport/history changes. |
| Entire configured game C source | COMPILES | 466/466 original configured C units compile for native ARM32 hard-float (A009); aggregate object retains 584 CRT/platform imports. Not a complete executable link. |
| Vita executable / packaging | LINKS | Current00.06 native VPK verified, hardware test pending. Prior00.05 BOOTS/RENDERS baseline preserved in build/vita/attempts/a017-baseline-00.05/. Verifier never transfers its hardware claim to00.06. |
| ABI / MSVC semantics | COMPILES | pointer32/long32/wchar16/enum32, signed char, 64-bit alignment8; native SDK ABI separated. Existing upstream asserts pass; cross-unit/function-pointer audit still incomplete (KI-009). |
| Real Halo core initialization | BOOTS | Both user logs show real cseries/debug heap/profile initialization returned and data/datum/iterator/compacting-pool/guarded-heap/CRC PASS crc=340bc6d9. This proves the selected core, not full game initialization. |
| Native Xbox-offset memory arena | LINKS | A017:96MiB USER_RW kernel arena, game16MiB/tag22MiB/texture22MiB/sound4MiB requests retain original offsets; original allocation/verification/state-buffer functions linked. Host mock tests pass; hardware allocation pending. No enforced per-page READONLY/NOACCESS policy. |
| Filesystem / maps | BOOTS | Prior hardware headers/log/input verified. New00.06 streaming reader and temporary directory checkpoint LINKS: host reads both compressed ui/a10 tag sections in place, checks bounds/CRC and datum/scenario metadata. Hardware tag read, recursive relocation, BSP/texture/sound activation, full filesystem/saves pending. |
| Logging | BOOTS | 00.04 proves rejection callback/Cg/recovery. 00.05 logs both compile successes, attach/link1,011 shaders1 and clean exit. Evidence retained under ignored docs/runtime/. Invariant omission logged;010 remains withheld. |
| Timing | BOOTS | Real-Vita logs show monotonic process times and input loop through ~110.7s; Halo profiler initializes. Precision/FPS/deterministic simulation still unverified. |
| Threads / synchronization | BOOTS | Both logs: pthread create=0 join=0 synchronized value=42. Full XDK handles/APCs/thread services not integrated. |
| Input | BOOTS | Logs show XInputGetState result=0, Cross/Circle/Square/Triangle mapped to A/B/X/Y, stick values, repeated header rescans and Start clean exit. Remaining buttons/ergonomics not yet demonstrated; White/Black/L3/R3 unassigned. |
| Audio | BOOTS | Both logs: SDL3 audio driver=vita stream=OPEN (paused). This proves device availability, not audible/game audio. dsound_sdl.c compiles; mixing remains unintegrated. |
| vitaGL / diagnostic display | RENDERS | Current 00.05: diagnostic plus visible synthetic triangle, no reported error/crash, responsive input/rechecks, Start exit. No retail Halo renderer or repeated stability claim. |
| Dynamic vertex / pixel translators | RENDERS | Current 00.05: original two-MOV NV2A vertex + pixel combiner compile1/1, link1, user confirms visible triangle. Proves this synthetic pair only; retail shader coverage and cross-program multipass invariance pending. |
| Full D3D8 GL backend / textures | COMPILES | d3d8_gl.c and xbox_textures.c compile with native GCC against upstream desktop GL declarations, not a completed Vita binding. 108 APIs inventoried; full renderer not linked. |
| Framebuffer copy fallback | BOOTS | 00.03 tiny GPU FBO/blit/readback probe PASS RGBA=51,102,153,255 error0. Proves this bounded test, not full renderer copies/formats/performance. |
| Cubemaps / 2D retail textures | BLOCKED | Header APIs present; formats/swizzling/sampling need real renderer/tag integration and tests. |
| 3D textures | BLOCKED | No installed 3D upload API; sampler3D generation explicitly rejects. A011: campaign c10/c20 contain six volume bitmaps each, ui five; runtime sampling/a10 usage unknown. |
| Visibility / lens flares | BLOCKED | Query APIs exported; exact semantics untested. Android atomics not copied. |
| Main menu / campaign a10 | BLOCKED | Full main not called. A017 adds memory and real tag-section/directory checkpoints, but detaches the temporary mount before return. Nested tag/BSP/resource relocation, XAPI and full renderer integration still required. No retail draw/gameplay. |
| Saves / networking | BLOCKED | Sources preserved; full platform integration deferred until single-player bring-up. |
| 30 FPS / stability | BLOCKED | Probe has 33,333us frame budget only; no gameplay or measured FPS/stability evidence. |

Console test instructions, final package SHA-256 and crash-symbol commands: [BUILD.md](BUILD.md). Attempt history is append-only in [ATTEMPTS.md](ATTEMPTS.md).
