# Known issues / open risks

Use stable IDs so attempts and commits can reference the same problem.

## KI-001 — Source baseline imported (resolved)

Resolved by existing import, verified A001.

Source baseline `21714ac0860e9b9ca08fbdc8a1d620f1b8a03797` recorded in docs/UPSTREAM.md and .upstream-base-sha. No reimport required. Initial scaffold status was stale.

## KI-002 — Compiler/ABI differences from x86/MSVC assumptions

Severity: high.

The upstream Linux build uses compiler flags and generated compatibility headers to reproduce important MSVC/Xbox semantics. ARM Vita needs an equivalent audit for struct layout, calling conventions, wchar width, FP behavior and undefined-behavior assumptions.

Do not solve this by globally disabling warnings/errors without understanding the affected ABI.

## KI-003 — Dynamic GLSL subset on vitaGL

Severity: high.

Halo Universal generates vertex and pixel shaders dynamically from Xbox NV2A state. vitaGL supports runtime shader compilation, but the exact generated GLSL syntax/features must be validated.

## KI-004 — 3D textures

Severity: high for faithful rendering.

Upstream supports `GL_TEXTURE_3D`/`sampler3D`; the installed vitaGL has no 3D upload entry points. A011 metadata inspection of existing Xbox v5 maps finds five volume bitmaps in ui and six in each campaign c10/c20: distance attenuation, four default-3d bitmaps and Elite plasma-shield noise in campaign. Dimensions are 32x32x32 / 4x4x4. Content presence is proven; actual runtime sampling and compressed a10 usage remain untested. Do not omit volumes silently. Next: instrument loaded shader/texture use, then implement/test a bounded 2D-atlas/slice fallback with correct filtering if needed.

## KI-005 — Memory pressure

Severity: high later in rendering.

Do not copy Android's large stream-buffer ring sizes. Establish Vita-specific budgets once rendering starts.

## KI-006 — Vita3K is not authoritative for all homebrew behavior

Severity: medium.

Plugin/runtime shader compiler/data-path behavior may differ from real hardware. Track test platform explicitly.

## KI-007 — Full Halo memory/main integration

Severity: blocking menu/campaign.

Linux reserves a fixed Xbox 128MiB virtual window with mmap at 0x80000000; physical_memory_map.c requests exact tag-cache address 0x803A6000 and game-state 0x81A00000 (expanded native capacity). VitaSDK's public alloc options do not offer that mmap/fixed-address interface; no equivalent placed XPhysicalAlloc has been implemented/tested. Game maps contain pointers and D3D resources mask/OR Xbox physical addresses. An ordinary malloc arena without rebasing is insufficient. Bring-up uses real guarded heap/data/pool code but deliberately does not call physical_memory_allocate or full main.

Next: design/test Vita allocation and consistent cache/resource pointer rebasing or demonstrate a usable native placed-allocation mechanism. Do not copy Linux mmap or Android loader to hide the issue.

## KI-008 — Complete renderer GL state/resource compatibility

Severity: blocking Halo draws/menu.

108 APIs audited against installed header/archive. Missing blend-color/draw-buffer/3D/readback/texture-vector entry points and persistent-stream/integer-attribute semantics require per-call implementations. Current shaders/triangle diagnostic is not the D3D8 renderer or retail geometry. Runtime GLSL is unverified; shader logs and generated source are saved on console. See GRAPHICS_COMPATIBILITY.md. No silent rendering stubs.

## KI-009 — Remaining cross-unit ABI contracts

Severity: high before full main.

Native hard-float changes float/variadic argument passing from x86. Known Android variadic and hs-fade fixes reused; terminal char-pointer va_list corrected, enums forced 32-bit. Still need systematic cross-unit/function-pointer verification (upstream android_abi_check.py consumes LLVM IR, while this GCC build does not). Successful layout/compile checks do not prove every reconstructed prototype correct. Upstream x86 stack_walk_windows is not linked; diagnostic sink reports lack of ARM unwind rather than walking invalid frames. Narrow CRT shims currently cover only the linked core closure; full path/case/saves/XAPI behavior remains pending.

## KI-010 — Console execution not yet available

Severity: blocking BOOTS/RENDERS evidence.

ELF/SELF/VPK creation is proven locally only. No attached Vita or discovered Vita3K installation was available during the first package build. Test on hardware using docs/BUILD.md; preserve debug.txt, shader dumps and crash address/dump with this build's ELF/hash. Milestone010 is withheld; successful diagnostic graphics cannot be called Halo menu/gameplay.

## KI-011 — Xbox v5 maps, not PC Custom Edition caches

Severity: data compatibility.

The imported reconstruction targets Xbox build 2342; its real cache verifier requires version 5. Retail Xbox maps are the intended data. PC Halo/Custom Edition map versions 7/609 and separate PC bitmaps.map/sounds.map do not establish compatibility. Bring-up checks ui.map/a10.map headers case-insensitively and logs the actual version. It does not yet load/decompress/rebase map tags.

## KI-012 — VitaShell install error 0x8010113D / LiveArea PNG format

Severity: blocks installation; code correction in 00.02, hardware confirmation pending.

The user reports 00.01 fails at the end of VitaShell installation with `0x8010113D`. All three original authored images had PNG bit depth 8 / color type 6 (RGBA), despite fully opaque pixels. VitaSDK samples require indexed palettes; the existing verifier checked only PNG signature/icon dimensions and missed this contract. A012 changes the generator to opaque indexed PNG-8/type 3 with a two-color PLTE and no alpha/tRNS. All three decoded images are unchanged; ELF/map/eboot hashes are identical. The verifier now rejects all old images and validates every new image's chunks/CRC/palette/pixel stream/dimensions and XML references.

Retry only package 00.02/hash in BUILD.md. Installation success is not claimed until the user confirms it. Do not return to RGBA assets or treat a valid ZIP/generic PNG as proof of Vita installer acceptance. Sources: [VitaSDK samples image contract](https://github.com/vitasdk/samples#notes-on-images), [LiveArea format investigation](https://github.com/hammerill/livearea-specs).
