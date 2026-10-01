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

Linux reserves a fixed Xbox 128MiB virtual window with mmap at 0x80000000; physical_memory_map.c requests exact tag-cache address 0x803A6000 and game-state 0x81A00000 (expanded native capacity). VitaSDK's public alloc options do not offer that mmap/fixed-address interface; A017 implements a native arena/placed XPhysicalAlloc, validated on hardware in A018. Game maps contain pointers and D3D resources mask/OR Xbox physical addresses. An ordinary malloc arena without rebasing is insufficient. Bring-up calls original physical_memory_allocate/verify and game_state allocation; full main remains withheld.

A017 advances this contract:00.06 links a kernel-backed 96MiB native arena with original offsets, guarded address translation in physical_memory_map.c/game_state_xbox.c and subtraction/addition for resource-address conversion. It calls the original physical-memory and game-state allocation functions. Host policy/lifecycle tests pass; A018 real-Vita reservation and original memory/state functions pass. The fixed tag range also overlaps the previously observed Vita executable load segment, so restoring fixed Xbox VA is not a usable solution. This implements allocation, not complete cache relocation: only checked directory/root/name pointers are translated for a temporary original-tag-API checkpoint, then detached. Nested tag pointers, BSP data, D3D resources and dirty-page tracking remain unresolved. Full main010 stays withheld.

Next: complete typed cache/resource pointer registration and scenario/BSP loading while preserving the hardware-proven placed arena. Do not copy Linux mmap or Android loader to hide the issue.

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

The imported reconstruction targets Xbox build 2342; its real cache verifier requires version 5. Retail Xbox maps are the intended data. PC Halo/Custom Edition map versions 7/609 and separate PC bitmaps.map/sounds.map do not establish compatibility. Bring-up checks ui.map/a10.map headers case-insensitively and logs the actual version. The UI tag section and known typed fields now load/rebase; full scenario/BSP/GPU resources do not.

A017 reads/decompresses the actual ui/a10 tag sections on the host, including zlib checksum and logical-size validation. Both have valid directories; runtime native reader is linked in00.06. A018 hardware reads both sections and exercises original tag APIs successfully. A020 verifies typed UI metadata/accessors on Vita. The 00.08 UI fast path keeps only `ui.map` mounted; it does not diminish the proven `a10` reader.

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

## KI-016 — Protection/recursive relocation and real menu startup pending

Severity: blocking full engine/menu. A018/00.06 BOOTS: memory and both real tag-directory checkpoints PASS.

The new96MiB aligned USER_RW reservation is additional to newlib64MiB and vitaGL RAM16MiB/CDRAM24MiB. A018 demonstrates allocation at0x85c00000 and free user62MiB/CDRAM72MiB/phycont26MiB after memory/state tests. Allocation success is a CPU-memory contract, not direct GXM-address validity or a performance result. QueryMemoryProtect tracks owned RW pages; READONLY/NOACCESS enforcement and Linux dirty-page faults remain unsupported and explicitly log/fail. A future renderer must use an adapted dirty-resource policy before relying on texture cache generations.

A020/00.07 hardware confirms 3362 structure-aware UI pointers rebased and the original `e22586e4` tag CRC restored after accessors. A022/00.08 hardware confirms persistent mount, original widget-global initialization, recheck and both CRC restorations. BSP payload, texture/sound resource words and other tag groups remain outside the typed relocation coverage; never run the full scenario loader against this partial mount.

Compressed real-map tag sections pass the actual native reader's host tests. A018 validates runtime015/016=2/2 and original tag APIs on ARM; CRCs match host. Its older checkpoint rebased directory/name/root pointers only and detached. A020 added the known typed UI metadata; A021 retains that checked UI mount. Other nested tag groups, BSP sections and resource descriptors are not activated. Never feed this partial mount into scenario_tags_load/full main or scan all32-bit words heuristically to invent relocation. Full native XAPI/renderer and shader/texture coverage remain separate blockers.

## KI-017 — Real Main Menu initialization and original draw still pending

Severity:blocking. A020/00.07 BOOTS: hardware017..020 confirms typed metadata,9-widget menu graph, original accessors and CRC restoration. A022/00.08 BOOTS confirms checked persistent UI image, original `ui_widgets_initialize`, Cross recheck, CRC restoration twice and clean exit. The mount is still partial and cannot be fed to full `scenario_tags_load`/main. Other nested tags/BSP/resource registration are not complete.

The inspected original startup requires game_precache_new_map -> dispose/load/new-map -> scenario/BSP registration -> main_screen_shell_load -> widget creation/events -> rendering. `ui_widgets_initialize` ran on 00.08 hardware; root widget creation, original update and render remain unentered. Host inspection of the actual UI graph identifies a root `_widget_event_created` handler, function23=`main_menu_initialize`, which immediately needs player UI/network/audio state; the game-demos child has another creation handler. A forced link of `ui_widget_load_by_name_or_tag` exposed 17 references across 11 missing original functions including game time, scripts, event handler invocation, audio, player control and update-server/new-map. These are real dependencies, not permission for success stubs. Native XAPI file/event/thread/alertable primitives need hardware contracts before startup uses them. No menu-active flag shortcuts.

D3D8 attribute/constant helpers COMPILE/CPU-pass; full GL binding, paired compiler/cache policy, constant/varying specialization, streaming/format/overflow and retail resources remain pending. HALO DRAW REACHED and HALO DRAW RENDERS remain unverified. Next: original startup/resource/backend closure and exact first-draw trace.

## KI-018 — 00.08 leaves the original UI pool allocated at exit (resolved for 00.09 checkpoint)

Severity: contained 16 KiB lifecycle leak in the staged checkpoint. A022 hardware `heap_dump.txt` has one 16,384-byte allocation at original `ui_widget.c:117`, exactly the `ui_widgets_initialize` pool; 00.08 never called disposal before `halo_vita_core_dispose`. A024/00.09 hardware logs 023 before both unmounts and the user reports no new dump. The original detector opens `heap_dump.txt` only when a guarded allocation remains; 00.08 proved the same path can write it. This resolves the observed checkpoint leak for this run. Do not call the full `ui_widgets_dispose` while the runtime remains partial: linking its active-widget closure requires real game-time, sound, player-control and event contracts. The 00.09 checkpoint refuses disposal if any widget becomes active; full disposal must be integrated before root creation.

## KI-019 — Original XDemos file contract validated; broader XAPI and dashboard warning remain

Severity: remaining XAPI file operations still block full startup. A027/A029 hardware validates the separate original file-reference/XDemos probe: 024 reports casefold UI map, maps directory, missing file and absent XDemos correctly, with per-thread last-error values. A029 retains the exact full-run log through Start clean exit. The child widget creation event itself has not executed. The bridge deliberately covers attributes only; full Xbox path/open/read/seek/directory enumeration/overlapped contracts remain pending for full main/campaign.

Compiling the original marketing module also exposes `IDirect3DDevice8_PersistDisplay(&global_d3d_device)` with a `D3DDevice **` argument where the XDK signature expects `D3DDevice *`. Its dashboard-launch cleanup is not called by the current XDemos availability probe and is discarded by section GC. Correct the actual upstream contract before activating dashboard/image launch; do not suppress the type warning or treat that code path as working.

## KI-020 — 00.11 root works on Vita; original update/render closure remains open

Severity: blocks visible Main Menu. A031 real Vita verifies original `ui_widget_load_by_name_or_tag`, both required creation handlers and the active nine-widget root. The user reports the vitaGL logo remains for five minutes; the exact log ends at 028. This is the expected output of 00.11 because it never enters original update/render or presents a new frame. A link experiment forcing `process_ui_widgets` found 48 distinct unresolved input/time/event/bink/keyboard symbols; forcing `render_ui_widgets` found 34 distinct text/rasterizer/resource symbols. Both are disabled in the delivered VPK, so no retail bitmap pixels, shader, D3D8 draw or present has been reached.

The temporary Vita dispatcher supports original functions 0/23/86 only; present interactive indices 87/101 and any unexpected event effects explicitly fail. Main-menu music playback remains deferred with a log because the game sound manager is not initialized. Active-root shutdown retains cache/arena until process exit to prevent dangling tag pointers; full original widget teardown is outstanding, and 00.11 cannot be considered STABLE. These limits must be resolved from real runtime evidence, without a fake menu, fake shader success or unconditional subsystem stubs.

## KI-021 — 00.12 logo report and missing original draw/present

Severity: visible Main Menu blocked. The user reports the vitaGL startup image remains visible for more than five minutes in a build labelled 00.12. No matching console log or exact installed VPK digest was supplied. Current source calls a single clear/swap handoff before the UI mount, then loads the real widget root; with `HALO_VITA_MENU_RENDER_PROBE=OFF` it never calls original UI rendering, D3D8 draw or a menu frame present. The report cannot establish whether the installed package reached handoff/root. Inspect a fresh 00.13 `debug.txt` for `[VITA 006A]`, `[VITA 028]` and `[VITA 032]` before attributing the displayed frame to a specific phase. Clear/swap only removes a stale splash; the real fix still requires the original UI renderer, resources and device state.

The original Xbox texture cache also has a blocking wait for `texture->loaded`. If the Vita synchronous resource reader returned a failed incomplete request, that wait would never end. A034 makes the Vita-only cache bridge fail explicitly after logging the exact reason, so this specific read failure cannot turn into an unbounded wait once the texture cache is linked. The checked logical-range reader has source/host evidence only; there is no hardware observation of a retail bitmap request or texture upload yet. The reader currently reinflates and validates the full compressed map for each request; measure real request order and latency before optimizing it. Do not replace the bitmap path with named menu assets.


## KI-022 — Compiled-cache bitmaps re-postprocessed in 00.16

Blocking visible Main Menu. A038 real Vita reaches 036T then asserts at xbox_texture_cache.c:157 because the first mounted bitmap already has `_bitmap_cached_bit`. scenario_tags_load reads prepared cache records directly; it does not repeat bitmap_group postprocess. The Vita bridge incorrectly calls texture_cache_bitmap_new on those records. Preserve compiled absolute pixels_offset/pixels_size/tag_index and validate cold runtime handles. Do not clear the cached bit and re-run registration, add pixel_data.file_offset twice, or remove the original assertion. No real bitmap read/upload/draw has yet been observed.


KI-022 source correction (A039/00.17): the bridge now validates prepared records without mutations; hardware confirmation pending. The assertion itself remains unchanged. A host regression also exposes a pre-existing compressed tag-read integrity regression: prefix completion can return before a damaged/truncated zlib trailer. Track the full tag validation separately from prefix resource reads.


## KI-023 — Staged shell omitted state read by window end

A042 source review finds original _rasterizer_window_end always calls main_get_window_count, which reads cinematic/player game-state pointers. The partial Vita game initialization left both NULL. Restore their original initialization/new-map owners plus original player-control storage and game-time state before first frame; do not replace the query with one. Host owner/query regression passes; native compile and real Vita confirmation remain necessary. Original scenario/BSP, input and audio/gameplay closure are still separate pending work.


A044 validation: KI-022 and KI-023 source corrections compile/link and are included in verified workflow113/VPK00.17. Host metadata/state contract tests pass. Both issues remain awaiting the real Vita first-frame log; a CI pass does not demonstrate runtime resolution or visible Halo rendering.


## KI-024 — Cold cached bitmap rejected for serialized Xbox pointers

A04500.17 hardware shows bitmap.cached=true/cache_block_index=NONE with nonzero serialized base_address0x024f0040. The Vita validator rejects it although original cached load overwrites base_address before registration and ignores hardware_format. False rejection causes controlled clean exit before any draw. Correct only that contract, retain flags/owner/NONE/resource bounds, and test actual loader/query ordering with stale sentinel addresses. Hardware confirmation pending.

## KI-025 — Renderer allocations left live on pre-root failure cleanup

A04500.17 heap_dump lists1,925,688 bytes owned by original rasterizer subsystems. Current pre-root cleanup frees UI/arena/core without calling original rasterizer disposal after successful rasterizer initialization. Restore original close/dispose while context, arena and tags are still valid; do not hide debug-memory reporting or free arbitrary owner allocations. Active-root teardown remains a separate pending contract.


KI-024 source correctionA04600.18: opaque pointer words accepted only in cold cached records; eight invalid metadata/range cases remain rejected. Actual original load/query regression proves replacement before registration/read. Native/hardware pending.

KI-025 source correctionA04700.18: pre-root cleanup now closes/disposes completed original renderer owners before tags/arena/core/context. Actual Vita owner-order regression passes; new native dispose closure/hardware still pending. Upstream disposer leaves some default texture/surface headers and native GL programs for process exit; no empty-heap/stability claim. Active-root teardown/full restart remain pending.


A048 validation: KI-024/KI-025 corrections, actual original loader/state/disposal contracts and exact ABI declarations are included in verified00.18/source4ee1d99/workflow116. Required render/dispose/cache-close symbols, native package ABI/assets and publication PASS. Both hardware runtime corrections still require the new00.18 log; original draw/render and an empty heap report are not inferred from CI.

## KI-026 — Packed depth texture allocation unsupported by vitaGL

A04900.18 hardware passes039/036S/036D/root and reaches037W0, then packed depth texture960x544 fails with GL_INVALID_VALUE0x501 and explicit fatal exit. No037W1 or Halo draw/present. vitaGL supports color texture attachment and a renderbuffer request for lazily allocated per-FBO GXM depth/stencil; generic packed depth texture allocation/attachment is unsupported. Adapt both operations and preserve depth identity across equal-size color switches. Completeness alone is insufficient proof of later GXM allocation or cross-scene depth persistence. Depth-only and depth sampling remain separate unsupported contracts.

A049 hardware: KI-024's384 compiled bitmap records now validate on Vita; original root is active again. KI-025 pre-root cleanup is not exercised by this active-root fatal exit; no new heap evidence.

KI-026 source correctionA05000.19: separate depth renderbuffer plus checked FBO attachment, one owning FBO per depth across equal-size colors. Actual-function host contract passes API selection/cache/identity/failure cases. Native linking and first-window GPU execution are pending; a GL request/FBO completeness result does not prove lazy physical depth allocation or persistent contents across GXM scenes.

A053 validation: KI-026 allocation/attachment correction and A051 color/phase checks compile/link and pass native package verification in workflow119/source2d8d5ae/VPK00.19. Host actual FBO identity/error and immutable color byte/format tests PASS. New first-window/GXM/Halo draw/render remain hardware gates. KI-024's metadata rejection is confirmed resolved in00.18 (A049); KI-025 pre-root teardown and active-root lifetime still need matching hardware/heap evidence. Do not mark the visible menu or cross-scene depth persistence working from CI alone.

## KI-027 — Unsupported unpack-alignment call causes first texture upload exit

A05400.19 hardware loads first real shield-noise resource successfully and reaches texture upload, then GL_INVALID_ENUM0x500 triggers explicit fatal exit. upload calls glPixelStorei(GL_UNPACK_ALIGNMENT,1); vitaGL6e7fe40 implements only UNPACK_ROW_LENGTH, despite defining ALIGNMENT in the header. Preserve original DXT/BGRA layout and use the supported tightly packed row contract; add phase/level GL checks and actual-function tests. No evidence of GPU overload or corrupt resource offsets. No Halo shader/draw/present yet.

A054 validates KI-026 through real960x544 request/attachment, original clear and037W1. Shared depth identity across scenes and STORE_DEPTH_STENCIL remain separate unverified contracts. Active-root fatal cleanup/heap still lacks new evidence.

KI-027 source correctionA05500.20: supported ROW_LENGTH0 for tight BGRA/DXT input, plus phase/level GL checks. Actual-function mock regression reproduces old alignment0x500 and passes corrected mip/row/failure contracts. Native texture GPU acceptance and visible image remain pending.

A057 validation: KI-027 supported-row/mip correction and A056 low-resolution mode are included in native verified workflow121/source9405665/VPK00.20. Actual-function mocked upload/screen/FBO/Present contracts PASS. New320x240 mode, real texture GPU acceptance and visible Halo menu remain hardware gates; native context960x544 is intentional presentation. No GPU-overload diagnosis or measured FPS claim is inferred from00.19's unsupported-state exit.


## KI-028 — Vita SubData clones entire in-flight stream for each glyph

A05800.20 reaches original Halo draw with error0 at320x240, then aborts in SceLibKernel memcpy called by glBufferSubData during original font rendering. Exact workflow121 ELF/relocation and allocation-then-copy disassembly confirm a NULL clone destination. Primary vitaGL6e7fe40 clones the full2MiB vertex buffer after each draw; allocation failure has no guard in this archive. Bound the Vita upload path and synchronize before mapping/reusing storage. Preserve geometry and same-draw stream reservations; reject failed mapping instead of calling SubData with a NULL clone. No measured total/peak memory or FPS claim. Menu Present/render remain unverified. KI-027 texture upload and A056320x240 allocation now pass hardware; no shaders need replacement based on this crash.


KI-028 source correctionA05900.21 uses checked synchronized mapping of existing vertex/index allocations, avoiding per-draw full clones/orphans; exact-byte transfers preserve source bounds. Host actual-function lifetime/guard-page/failure contracts PASS; native map/sync symbols and hardware original font/frame/present remain required. Synchronous scene transitions retain KI-026 depth persistence risk; no STABLE/FPS/visible-menu claim.


A060: KI-028 mapped reuse correction passes exact native workflow122/sourcefb92bbf/VPK00.21, including real glMapBufferRange/glUnmapBuffer/glFinish symbol closure and actual downloaded artifact digests. Host contracts PASS; original font/full-frame Present/visible menu and repeated stability need hardware.320x240 policy remains unchanged. Retain workflow122 ELF/map for the new dump, never use workflow121 symbols to decode00.21.


## KI-029 — DXT mip transfers may outlive destination reallocations

A06100.21 passes original UI/font/frame and Present but later aborts in _malloc_r while allocating a compressed texture; free-list links are corrupt. Primary vitaGL6e7fe40 starts asynchronous DXT transfer copies and grows/moves the destination with vgl_realloc between mip levels. glFinish alone waits drawing, not explicit transfer completion. Synchronize native transfers before mutation and after each mip before reallocation/staging reuse. The dump proves heap corruption, not the exact writer; correction needs hardware confirmation and does not diagnose overall memory exhaustion.

## KI-030 — Installed non-strict uniform handles cannot identify array elements

A062 exact workflow122 glUniform4fv uses a negated uniform pointer with offset0; indexed offsets require STRICT_UNIFORMS_COMPLIANCE, absent in this archive. Existing c[i]/pixel-array lookups assumed a different driver contract; previous host mocks did not model it. Query active compiled vec4 array spans and base locations, then upload the bounded original prefix. No handle arithmetic, fake shader or unconditional192-vector write.00.21 completes Present but user sees black; RENDERS remains unverified.

A061 confirms KI-028 through original font/UI/frame completion in00.21. New allocator failure is separate; retain matching122 ELF/core.


A06300.22 corrects KI-029's transfer/reallocation lifetime hazard with checked native waits before mutation and after each mip, preserving all data; actual lifetime fixture PASS. KI-030 base-array reflection/bounded original-prefix uploads pass actual helper/non-strict fixture; no indexed-handle assumption remains. Native and visible/repeated hardware gates pending. Source-confirmed race is not yet proven sole cause of A061 heap damage.


## KI-031 — Partial visible UI without scenario/update/audio

A064/00.22: logo and rectangular backgrounds visible. Active Vita loop does not invoke original widget update, world render or HS camera/scenario update; music is explicitly deferred. Ordinary workflow131 passes; optional process_ui_widgets closure does not link (desktop pointer and full game/load/network/sound dependencies). Do not claim full ui.map execution from tag mount or UI-only Present. Missing labels require font/alpha and original widget timing investigation.


## KI-032 — Staged menu Present queues flips without its vblank worker

A070/00.23: real visible logo/backgrounds, two returned Presents, no frame30/120 and Start unresponsive. main_initialize_time normally starts the worker through the original vblank callback; staged startup skips it. With interpolation disabled, third Present waits forever after pending_flips reaches2 because no worker consumes flips. Actual host code reproduces the historical third-call hang. Vita Present now starts the same existing60Hz worker before queueing and fails explicitly on creation failure.120-frame/callback/interpolation/error host contracts pass. Native link and real console repeated-frame/Start verification remain required; later renderer stalls are not excluded.

A071/00.24 native closure correction: the now-reachable worker exposes absent clock_nanosleep; use a checked absolute CLOCK_MONOTONIC/native kernel-delay bridge instead. Native VPK links, host actual120-frame and deadline/error cases pass. Real Vita third/frame30/frame120/input/exit remain the acceptance gates. Full UI/labels/world/audio are still KI-031.


A072 hardware00.24 resolves KI-032 for this supplied run: third/frame30/frame120 and Start/clean exit. UI remains visually incomplete with opaque rectangular backgrounds, no labels or observed motion; KI-031 remains open.

## KI-033 — Texture uploads mutate the previously active stage

A072 source/actual-function fixture: later cold/refreshed textures bind on the earlier active unit before destination stage selection. Vita selects the target unit before upload/composition now; historical code fails and fixed host contracts pass. Console alpha/text/effects improvement remains unverified. Do not strip backgrounds or replace authored shader/texture alpha based on this hypothesis.
