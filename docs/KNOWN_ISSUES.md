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

A014/A015 identify the invariant rejection; A016 verifies current00.05 vertex/pixel compilation, link and visible synthetic triangle. This proves the probe pair, not every retail shader. Broader shader coverage and multipass invariance still need validation.

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

A017 advances this contract:00.06 links a kernel-backed 96MiB native arena with original offsets, guarded address translation in physical_memory_map.c/game_state_xbox.c and subtraction/addition for resource-address conversion. It calls the original physical-memory and game-state allocation functions. Host policy/lifecycle tests pass; real-Vita reservation has not yet been tested. The fixed tag range also overlaps the previously observed Vita executable load segment, so restoring fixed Xbox VA is not a usable solution. This implements allocation, not complete cache relocation: only checked directory/root/name pointers are translated for a temporary original-tag-API checkpoint, then detached. Nested tag pointers, BSP data, D3D resources and dirty-page tracking remain unresolved. Full main010 stays withheld.

Next: design/test Vita allocation and consistent cache/resource pointer rebasing or demonstrate a usable native placed-allocation mechanism. Do not copy Linux mmap or Android loader to hide the issue.

## KI-008 — Complete renderer GL state/resource compatibility

Severity: blocking Halo draws/menu.

108 APIs audited against installed header/archive. Missing blend-color/draw-buffer/3D/readback/texture-vector entry points and persistent-stream/integer-attribute semantics require per-call implementations. A016 verifies current synthetic NV2A shaders/triangle on Vita, not the full D3D8 renderer or retail geometry. Broader runtime GLSL/state/texture coverage remains unverified; logs/source are saved on console. See GRAPHICS_COMPATIBILITY.md. No silent rendering stubs.

## KI-009 — Remaining cross-unit ABI contracts

Severity: high before full main.

Native hard-float changes float/variadic argument passing from x86. Known Android variadic and hs-fade fixes reused; terminal char-pointer va_list corrected, enums forced 32-bit. Still need systematic cross-unit/function-pointer verification (upstream android_abi_check.py consumes LLVM IR, while this GCC build does not). Successful layout/compile checks do not prove every reconstructed prototype correct. Upstream x86 stack_walk_windows is not linked; diagnostic sink reports lack of ARM unwind rather than walking invalid frames. Narrow CRT shims currently cover only the linked core closure; full path/case/saves/XAPI behavior remains pending.

## KI-010 — Console diagnostic rendering received; full Halo rendering unverified

Severity: BOOTS/diagnostic RENDERS evidence received; full renderer/gameplay evidence pending.

No attached Vita/Vita3K was available during the first build. A013 proves core/platform tests, A014 identifies deferred-link crash, A015 demonstrates safe recovery, and A016 verifies the 00.05 synthetic NV2A triangle on hardware: compile1/1, link1, visible draw and Start clean exit. Maximum state is diagnostic + synthetic NV2A RENDERS; full Halo rendering/gameplay remains unverified. Preserve logs/source/crashes with matching ELF/hash. Milestone010 remains withheld; one 34.3s run cannot be called STABLE.

## KI-011 — Xbox v5 maps, not PC Custom Edition caches

Severity: data compatibility.

The imported reconstruction targets Xbox build 2342; its real cache verifier requires version 5. Retail Xbox maps are the intended data. PC Halo/Custom Edition map versions 7/609 and separate PC bitmaps.map/sounds.map do not establish compatibility. Bring-up checks ui.map/a10.map headers case-insensitively and logs the actual version. It does not yet load/decompress/rebase map tags.

A017 reads/decompresses the actual ui/a10 tag sections on the host, including zlib checksum and logical-size validation. Both have valid directories; runtime native reader is linked in00.06. This is a restricted directory checkpoint, not full recursive relocation/activation or menu compatibility. Hardware results are still pending.

## KI-012 — VitaShell install error 0x8010113D / LiveArea PNG format

Severity: original installation blocker resolved by subsequent runtime evidence (A013).

The user reports 00.01 fails at the end of VitaShell installation with `0x8010113D`. All three original authored images had PNG bit depth 8 / color type 6 (RGBA), despite fully opaque pixels. VitaSDK samples require indexed palettes; the existing verifier checked only PNG signature/icon dimensions and missed this contract. A012 changes the generator to opaque indexed PNG-8/type 3 with a two-color PLTE and no alpha/tRNS. All three decoded images are unchanged; ELF/map/eboot hashes are identical. The verifier now rejects all old images and validates every new image's chunks/CRC/palette/pixel stream/dimensions and XML references.

Subsequent logs establish installation/boot; KI-013/KI-014 are corrected and KI-015 tracks the next shader blocker. Keep indexed assets in current 00.05. Do not return to RGBA assets or treat a valid ZIP/generic PNG as proof of installer acceptance. Sources: [VitaSDK samples image contract](https://github.com/vitasdk/samples#notes-on-images), [LiveArea format investigation](https://github.com/hammerill/livearea-specs).

## KI-013 — Misread vitaGL init flag leaves splash permanently visible

Severity: resolved on real Vita in 00.03 (A014); retained for regression.

Both user logs report `vglInitWithCustomSizes failed` after the library splash, then `graphics=0 shaders=0`. Input remains alive and Start exits; this is not a load that needs more time. Our condition treated zero as failure. The exact displayed vitaGL revision 6e7fe40/source/vgl.c initializes the context, sets vgl_inited, and returns res_fallback: zero is normal when requested resolution is retained; one indicates clamping. Installed archive disassembly confirms returning the saved fallback flag. Header GLboolean alone did not document this meaning.

00.03 logs the flag, checks GL version/viewport, guards duplicate initialization and preserves budgets. It submits a diagnostic frame before GPU/compiler experiments and logs each begin/result. Runtime banner now matches SFO 00.03. Host regression compiles the actual init function with mocked SDK and accepts normal 0 / fallback 1, rejects NULL-version/zero-viewport, and checks no second init. This is control-flow evidence, not hardware rendering. Source: [exact vitaGL implementation](https://github.com/Rinnegatamante/vitaGL/blob/6e7fe40/source/vgl.c).

Resolution evidence A014: 00.03 returns0 with valid 960x544 viewport/version, logs006/012, shows the app screen and passes GPU-copy. The subsequent link crash is KI-014. Keep init regression and010 withheld. Do not increase pools, reinstall libshacccg or wait longer as a workaround for this old branch.

## KI-014 — Deferred NV2A shader-link data abort

Severity: shader rejection recovery demonstrated on real Vita in 00.04 (A015); retained for regression. Full shader acceptance tracked by KI-015.

Signature: log ends at `NV2A shader generation returned vertex=OK pixel=OK; runtime compile begin`. Dump main thread HCEV00001 reason0x30004 data abort, PC0xe007eb04 in SceGxm+0x135c4, r0=0; LR0x81083e7b rebases to ELF0x8100de7a in glLinkProgram at sceGxmProgramGetParameterCount. Stack word0x810778e3 rebases to probe ELF0x810018e2, vita_graphics.c:79 in matching 00.03 symbols. This is a stack candidate, not an ARM unwind. The deferred link implementation compiles both then queries vertex parameters without rechecking NULL after compilation; initial checks only precede compilation. GL_COMPILE_STATUS reports true for uncompiled postponed GLSL. Compiler failure causing a NULL program is consistent with this exact path; the rejected GLSL/Cg construct is still unknown. The dump does not include the shader heap, so its individual program fields cannot be read directly.

00.04 uses VGL_MODE_SHADER_PAIR, always compiles vertex then fragment to finish semantics, checks real status before any attach/link, and logs/reclaims failed objects while the diagnostic loop continues. A VitaShaRK log callback captures diagnostics even if the archive lacks HAVE_SHARK_LOG; failed retained Cg is written to vertex_probe.cg/fragment_probe.cg with a 256KiB temporary-buffer bound. Original generated GLSL, NV2A operations, SDK and pool sizes are preserved. Nine actual-function host regression cases pass; native build/package verification passes. These do not prove hardware recovery or shader acceptance.

Resolution evidence A015: 00.04 logs vertex0/pixel1, captures compiler line177, saves rejected Cg, skips attach/link, reaches011 and remains responsive until Start clean exit at21.5s. This demonstrates recovery of the observed compiler rejection; it is not repeated-run STABLE evidence. Do not restore POSTPONED or attach failed shaders. Reference: [vitaGL deferred shader implementation](https://github.com/Rinnegatamante/vitaGL/blob/6e7fe40/source/custom_shaders.c).

## KI-015 — GLSL invariant declaration reaches Cg unchanged

Severity: observed probe compilation blocker resolved in 00.05 on real Vita (A016); multipass invariance remains an open fidelity risk.

Signature: `stage=vertex level=2 line=177: expected type specifier, but found 'identifier' instead`. Exact supplied vertex_probe.cg line177 is `invariant gl_Position;`. vitaGL's translator changes attributes/varyings/types but leaves this GLSL-only declaration intact. Vertex status0, pixel status1, recovery succeeds. This identifies a concrete syntax failure; other unsupported constructs are not yet excluded.

00.05 moves that declaration inside the existing non-Vita prologue guard and logs its omission. NV2A operations, arithmetic helpers, position/viewport/Y/Z transforms, attributes/varyings and fragment output are preserved. Desktop/Android retain the declaration. Host compilation of actual generators compares114 cases per backend to archived00.04: Vita differs only by the declaration, desktop/Android outputs byte-identical; old synthetic probe matches hardware GLSL exactly. Native build/package validation passes. ShaccCg compilation/link/triangle must still be tested on Vita.

Resolution A016: exact00.05 log shows vertex1/pixel1/link1, compile GL errors0,011 shaders1 and clean exit at34.3s; user confirms visible triangle without error. Hardware GLSL matches the intended one-line omission and unchanged pixel source. No renderer change is needed for this solved syntax blocker.

The GLSL invariant qualifier promises consistent position results across programs for multipass alignment; removing it does not establish that guarantee. Track a real multipass depth/position test before claiming renderer fidelity. Do not replace it with an unverified Cg pragma or silently drop position calculations. Sources: [GLSL1.20 section4.6](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.1.20.pdf), [vitaGL translator](https://github.com/Rinnegatamante/vitaGL/blob/6e7fe40/source/utils/glsl_utils.c). Next: expand shader/geometry coverage during full renderer integration; preserve this successful probe as regression evidence.

## KI-016 — New memory/tag checkpoint needs hardware; protection/recursive relocation pending

Severity: blocking full engine/menu. A017/00.06 LINKS, no new runtime evidence yet.

The new96MiB aligned USER_RW reservation is additional to newlib64MiB and vitaGL RAM16MiB/CDRAM24MiB. The real console must demonstrate allocation and sufficient remaining memory; host mocks cannot establish this. Allocation success is a CPU-memory contract, not direct GXM-address validity or a performance result. QueryMemoryProtect tracks owned RW pages; READONLY/NOACCESS enforcement and Linux dirty-page faults remain unsupported and explicitly log/fail. A future renderer must use an adapted dirty-resource policy before relying on texture cache generations.

Compressed real-map tag sections pass the actual native reader's host tests. Runtime015/016 and original tag APIs remain untested on ARM. The checkpoint rebases directory/name/root pointers only, validates scenario sky/BSP-reference ranges, and detaches. Nested tag pointers, BSP sections and resource descriptors are not activated. Never feed this partial mount into scenario_tags_load/full main or scan all32-bit words heuristically to invent relocation. Full native XAPI/renderer and shader/texture coverage remain separate blockers.
