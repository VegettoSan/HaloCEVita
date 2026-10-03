# Project status

Last updated: 2026-10-02

V5 source00.36 (A111–A113): default target now selects original
`game_initialize`, complete UI callback tables, original XInput/event collection,
`process_ui_widgets`, `main_screen_shell_load` and
`main_pregame_render`/`render_frame_present`. The prior focus/history substitute
and deferred world-resume aliases remain only in the explicitly selected
recovery build. Native logical-range map reading, typed relocation, vitaGL/NV2A
and original 2D menu sound/cache adaptations are reused.
All21 host regression commands pass locally after adapting their presentation
check to both original/recovery callers. Local exact-SDK00.36 ELF/SELF/VPK passes the full native verifier after fixing
archive selection of the Xbox resource reader (A115). Final pushed CI artifact and
Vita runtime acceptance are pending; no BOOTS/menu/gameplay claim for00.36.
Scenario/BSP/camera world activation remains a separate unaccepted boundary.
The matching original source closure must link/package before delivery.

Source00.35 (A108/A109): repaired the obsolete package-symbol gate and restore
the original D3D color/depth target after Vita resource preparation, before
raster state. A stateful host execution reproduces the historical destination
loss and passes the correction on cold/warm draws. Native Build218 on
a080234 passes all required host/native/package gates and publication (A110).
The exact00.35 VPK is independently verified and delivered; state LINKS.
Real-Vita visual acceptance is pending. No complete ui.map or white-panel-fix claim.

Source00.34 comparison (A107): native Vita presentation now swaps the original
D3D8 aspect-preserving blit once. Removed its obsolete second full-panel blit
and the redundant blend-only texture lookup wrapper; original raster state is
applied after resource binding. The original 640x480 UI/map data is unchanged.
Vita Build216 and first real-console cold/warm UI evidence are pending; this is
not a verified fix for white panels or complete ui.map execution. The optional
full original UI/world closure still has unresolved dependencies (A106).

2026-10-02 scope correction: removed the external BirchWoodGod full-UI reference
workflow and its donor-only atomic bridge. HaloCEVita build outputs remain this
repository's native ARM32 vitaGL port only. External Halo Vita projects may be
inspected for code, behavior and implementation ideas, but their VPK/eboot or
GXM backend must not be packaged or presented as HaloCEVita. Real-Vita tests
of the current source remain pending.

Source00.32 shell integration (A097–A102): original profiles, generic event effects, keyboard queue and per-frame completion connected. Required native Vita Build174/run36940875373 on75d5893b64e1797051572d518918726fca39cece now PASS, including the profile filesystem/retail editor/shell return dependencies that failed Build173 (A101). Host relocation/input/effects/shell/storage contracts PASS. Standing GitHub/main authorization is recorded in AGENTS.md; exact-tree fast-forward publication and local synchronization completed. Start confirms keyboard; Select exits staged build. All-window/hardware acceptance remains unverified.

Latest hardware **00.31: BOOTS / partial HALO DRAW RENDERS / original D-pad focus visible** (A094). User confirms selected option feedback. Supplied log proves focus wrap, Campaign conditional root/back, then separate fatal unsupported game-data5 (Multiplayer) and14 (Settings) after blocked creation24/34. Full window navigation is now the priority; user defers the remaining white backgrounds. No complete-menu/gameplay/stability claim.

Current package **00.32: LINKS** (A102). Vita Build174/run36940875373 on source75d5893b64e1797051572d518918726fca39cece: required native build/package verifier, menu-audio-contracts and publication PASS. [00.32 release](https://github.com/VegettoSan/HaloCEVita/releases/tag/vita-build-174); VPK1671406 bytes, SHA-256 b7411f358e17a36cfe55322170ab1ecec4950417f4909ab0cc9144f155da4659. Independent archive/manifest/ELF32 ARM hard-float/SELF/SFO verification PASS, including11 newly integrated shell/profile/storage symbols. Original keyboard/profile functionality is linked and host-validated; fresh console tests must verify menus, generated lists, save/reopen and repeated returns. Optional full renderer/world closure still fails and is not part of this staged package. White backgrounds remain deferred. No00.32 BOOTS/all-window/gameplay claim.

Latest hardware00.27 (A087): BOOTS / partial HALO DRAW RENDERS with original bitmap option lettering now visible. User reports good music/feedback; photo still shows white/gray rectangles and no navigation. Source has no active widget input processor. Full UI/transparency/navigation/scenario remain pending.

Current source00.27 (A085) selects the existing original CPU DXT3 decoder for a controlled mask-sampling experiment. Real9 menu mask frames and uploader/synthetic alpha contracts PASS on host. Required native SDK build, package verification and publication PASS in Vita Build #145/run36894968119 (A086);00.27 is LINKS, hardware validation pending. DXT1/DXT5 remain native. This is not a confirmed transparency/full-UI correction.

A084 exact uploaded-map audit corrects the label hypothesis: main-menu option lettering is authored in DXT3 bitmap alpha, while its auxiliary text alpha0 is intentional in the file. Raw Campaign mask retains0..119 alpha and14537 transparent pixels; focused frame1 retains0..255. Original background GLSL preserves this mask in a controlled host GPU replay. Vita sampling/compiler/actual draw inputs remain to isolate; do not force auxiliary text alpha. Build-number font text remains separate.

Latest hardware **00.26: BOOTS / partial HALO DRAW RENDERS / continuous audible 2D music** (A077–A083). User reports uninterrupted music and Square feedback,29–30FPS and unchanged partial UI; supplied log confirms frames30/120, repeated original feedback and Start/clean exit at155.46s. The comparison uses maximum overclock context; FPS is user-observed, not a new internal benchmark. No repeated-run STABLE or complete-menu claim. Seekable live sound reads are about16–18ms for65520 bytes; sampled steady frame bodies are about19.8ms, excluding the outer-loop pacing. Labels, transparency and full scenario remain unresolved.

The nearly one-minute startup has a source-matched gap: tag loading completes at7.28s; resource preparation precedes the typed-relocation marker at61.97s; mount/resource binding is reported ready at63.19s. The existing log does not time preparation and relocation separately, so their exact shares are not established. First actual menu Present occurs at72.56s after cold shaders and bounded first-draw traces. Compressed maps are regenerated into process-owned disk backing on each launch (D036); no persistent-cache identity policy exists. A083 records the hardware evidence and continuation constraints.

Text trace now isolates zero alpha at the widget input for three labels: tag/packed/glyph alpha all0 while instance/cumulative alpha remain1. A fourth text widget has alpha1, so one alpha cause must not be generalized to all missing elements. Font atlas coverage includes transparent/opaque texels, and the generated font shader multiplies sampled alpha by the vertex color alpha. Compressed image uploads bypass the current decoded-pixel coverage trace; absent image-coverage lines do not prove missing pixels or alpha. Source semantics/raw map bytes and independent background sampling/blend evidence are still required before changing authored opacity. Full widget navigation, scenario BSP/models/HS scripts and spatial/game audio remain pending.

Latest required native SDK2026.08 packaging/host contracts/release succeeded: workflow145/run36894968119, source59e2e3659c596b5c74fe35d9733d12781fe5d46e, VPK SHA-256 9ca42e90369ff4737ec5c2da87810d7ebe098df21dda25274c29d27c572566db (A086). [00.27 release](https://github.com/VegettoSan/HaloCEVita/releases/tag/vita-build-145). Prior00.26 workflow143 package remains the hardware baseline (A082/A083). A083 uses runtime banner00.26 and user delivery context, not a device-side package hash.

Historical hardware **00.23: partial HALO DRAW RENDERS, loop stall** (A070/KI-032). Logo and plain backgrounds visible, no text/animation; user reports orange rectangle absent and Start does not exit. Original render/shaders/source pixels and two Presents complete. No frame30/120 or input/exit follows. Missing flip consumer is a source-confirmed deadlock independently reproduced on host; later renderer failures are not excluded. Prior00.23 Vita CI workflow132/run36823114339 succeeded. A072 confirms00.24 continuous frames and Start exit.

Historical hardware: Latest hardware **00.21: BOOTS / HALO DRAW REACHED / frame submitted** (A061). Actual320x240 targets, original bitmap upload, both NV2A pairs and mapped stream writes pass. Original UI/window/frame ends and D3D8 Present return successfully at16.976s; user still sees black. Later compressed texture allocation aborts in _malloc_r with corrupt heap links (KI-029), decoded against matching workflow122 ELF. No visible menu/RENDERS/STABLE. Source inspection identifies asynchronous mip-transfer/reallocation lifetime risk and the installed non-strict uniform-location mismatch (KI-030/A062); both corrections are pending. Original640x480 coordinates/native960x544 presentation remain.

Current source **00.22** (A063) adds checked native transfer completion before texture mutation and after each mip, and replaces unsupported indexed uniform handles with reflected whole-array bases/compiled spans. Original pixels, mip chain, shaders, register indices,320x240 screen policy and mapped stream budgets preserved. Actual host reflection/lifetime/byte/failure/stream/original-owner contracts PASS. Native workflow and real visible/repeated console frames pending; allocator writer hypothesis remains unconfirmed.

Latest hardware **00.22: partial HALO DRAW RENDERS** (A064). User photo shows original logo and rectangular widget backgrounds; log reaches first/second Present, original NV2A shader pairs and nonblack source pixels with GL error0. Labels, widget update/input, scenario BSP/models/scripts, music and repeated stability remain unverified or unimplemented. main0efb6e4 ordinary native workflow131 succeeds; optional full UI-update link probe fails.

Historical demonstrated state: **DIAGNOSTIC RENDERS** (diagnostic + synthetic NV2A triangle on real Vita) (A016). Tested baseline **00.05** compiles both generated shaders, links successfully and draws the triangle per user observation. It remains responsive through header/input tests and Start clean exit at34.3s. Full Halo rendering/menu/gameplay, multipass position invariance and repeated stability remain unverified.

Historical menu-root package **00.11: BOOTS** (A031) first confirmed original root creation and handlers86/23. New00.18 evidence A049 confirms that root again after original shell/decal owners, then reaches first-frame/window initialization and fails at depth allocation. A058 now reaches original draw; visible output remains unverified. [Original call graph](MAIN_MENU_CALL_GRAPH.md).

Historical source package **00.13: LINKS** (A034). Vita CI built and linked 00.13, retained the VPK/ELF/map artifact, and the 00.12 splash handoff remains in source; a real-Vita log is required before a BOOTS claim. The 00.12 user reports the vitaGL logo remains, with no matching console log supplied. In source, the root-creation path still does not call original UI render/D3D8 present, so a visible Main Menu is blocked independently of cache pixel reads. No retail bitmap request or GL texture upload has been observed on hardware. The isolated original UI renderer probe (A035) compiles its original UI/render units but still has 42 unique unresolved link symbols after the Vita-only observed game-data callback selection; it is not enabled in the VPK.\n\nHardware-tested **00.10: BOOTS** (A027–A029). The inspectable full-run log is `docs/runtime/2026-09-29-00.10-start-exit-debug.txt`, SHA-256 `5d6bd171081e24bead791401d48db2711581658473345ca5fe7a98e011e51a15` (A029): 024, Cross remount, two 023/020 teardowns and Start clean exit. A028's separate partial rerun is also retained; the original A027 log was read/hashed but deleted before archival. Preserved VPK SHA-256 `82822ae873908bf82fee45a464d6b116481a8cb2cad8f59da8c5751c98f29686`; package association uses the version banner/latest delivered artifact, without a device-side digest.
Hardware-tested **00.09: BOOTS** (A024). The real Vita log SHA-256 `ea86ea8e183e919c69e8d936f28bfbce866433d108060f6aa997aedd64d032f3` confirms 023 widget-pool free before both Cross recheck and Start unmount, 020 CRC `e22586e4` restoration twice, 021 remount and 022 reinitialization, and clean exit at 43.050s. The user reports no new `heap_dump.txt`; original `debug_dump_memory_for_file` opens it only when a guarded allocation remains, and 00.08 proved this path wrote the prior dump. This supports resolution of the observed 16 KiB leak for this run, not repeated-run STABLE.

Hardware-tested **00.08: BOOTS** (A022). User log SHA-256 `11481d1e15adba4c6bdf4a285e42386af2412168d26dc7fd6134fde3eeb22847` confirms 021 persistent mount and original widget globals/pool, two full UI reads after Cross, both CRC `e22586e4` restorations, and Start clean exit. Heap dump SHA-256 `da8d81d57f7d93dabacdbbfcc3cb9fe2abcd6bf301917b97810f2847f048e4f6` identifies one 16,384-byte allocation at original `ui_widget.c:117` left allocated at exit. No root widget, original render or gameplay was entered.

Hardware-tested **00.07: BOOTS** (A020). User-supplied log SHA-256 `fb0f77a2553e6da501bdf768d765d352195f0b8143f90cb967aeb4b8f3f4c321` confirms native typed relocation017 (3362 pointers), actual Main Menu tag018 (datum `e285010f`, 9 reachable widgets), original bitmap/Unicode/font accessors019 (195/122/4), rollback020 (CRC `e22586e4`), and clean exit. No widget runtime or GPU resource activation occurred in that run.

Hardware-tested baseline **00.06: BOOTS** (A018), validated on real Vita. It integrates a 96MiB native Xbox-offset arena, original physical_memory_allocate/verify and game_state_allocate_buffer, plus streaming read/decompression and a restricted tag-directory mount using original tag APIs. Actual reader host tests:25 synthetic cases and both user maps pass (ui983 tags/a10 3357). Original allocator/state functions pass mocked-SDK lifecycle/overlap/alignment/reuse checks. Full nested tags/BSP/GPU resources and the full renderer/main/menu remain BLOCKED;010 is still withheld. Hardware013/014 PASS and016=2/2: arena0x85c00000, game0x87600000, tags0x85fa6000, texture0x8a600000, sound0x8a200000. Free user62MiB/CDRAM72MiB/phycont26MiB after allocation. ui4.82s and a10 57.12s, original tag APIs PASS and CRCs match host. Supplied run has no cancellation/recheck/clean-exit evidence; STABLE remains unverified.

Hardware feedback: A012 repaired installer images, A013 fixed the init flag, A014/A015 expose and recover vertex rejection, and A016 verifies the invariant-declaration fix on hardware. 00.05 vertex1/pixel1/link1, compile GL errors0, GPU-copy PASS,011 shaders1, visible triangle and clean exit. Hardware GLSL differs from00.04 only by the removed invariant line; fragment is byte-identical. No engine/SDK/pool rewrite. Host114-case comparisons per backend preserve all arithmetic and desktop/Android output. Runtime/SFO versions agree; broader retail shader coverage remains untested.

Status vocabulary: `COMPILES`, `LINKS`, `BOOTS`, `RENDERS`, `PLAYABLE`, `STABLE`, `BLOCKED`.

| Area | Status | Evidence / next requirement |
|---|---|---|
| Upstream source / independence | COMPILES | Existing independent import verified: 21714ac0860e9b9ca08fbdc8a1d620f1b8a03797; no reimport/history changes. |
| Entire configured game C source | COMPILES | 466/466 original configured C units compile for native ARM32 hard-float (A009); aggregate object retains 584 CRT/platform imports. Not a complete executable link. |
| Vita executable / packaging | LINKS current / BOOTS prior |00.32 workflow174 native verification/release and independent checks PASS (A102); hardware pending.00.31 boots with visible original focus but incomplete windows (A094). |
| ABI / MSVC semantics | COMPILES | pointer32/long32/wchar16/enum32, signed char, 64-bit alignment8; native SDK ABI separated. Existing upstream asserts pass; cross-unit/function-pointer audit still incomplete (KI-009). |
| Real Halo core initialization | BOOTS | Both user logs show real cseries/debug heap/profile initialization returned and data/datum/iterator/compacting-pool/guarded-heap/CRC PASS crc=340bc6d9. This proves the selected core, not full game initialization. |
| Native Xbox-offset memory arena | BOOTS | A017:96MiB USER_RW kernel arena, game16MiB/tag22MiB/texture22MiB/sound4MiB requests retain original offsets; original allocation/verification/state-buffer functions linked. A018: real kernel allocation, original physical/state functions and alignment test PASS;62MiB user free after allocation. No enforced per-page READONLY/NOACCESS policy. |
| Filesystem / maps | BOOTS | A018 compressed ui/a10 sections read on Vita;983/3357 tags, CRC e22586e4/1c90dc0f, original tag APIs PASS. A027 runs the original file-reference/XDemos attribute path and per-thread last-error isolation on Vita. Recursive relocation, BSP/texture/sound activation, full XAPI/filesystem/saves pending. |
| Logging | BOOTS | Exact A029 00.10 log preserves 024, both 023/020 teardowns, Cross remount and Start clean exit. A028 partial log and earlier logs remain under ignored docs/runtime/. 010 remains withheld. |
| Timing | BOOTS | Real-Vita logs show monotonic process times and input loop through ~110.7s; Halo profiler initializes. Precision/FPS/deterministic simulation still unverified. |
| Threads / synchronization | BOOTS | Both logs: pthread create=0 join=0 synchronized value=42. Full XDK handles/APCs/thread services not integrated. |
| Input | BOOTS | Logs show XInputGetState result=0, Cross/Circle/Square/Triangle mapped to A/B/X/Y, stick values, repeated header rescans and Start clean exit. Remaining buttons/ergonomics not yet demonstrated; White/Black/L3/R3 unassigned. |
| Audio | BOOTS / audible prior | A08300.26 user confirms continuous music and Square effects; log confirms original packets, repeated feedback and ordered sound disposal/clean exit. Seek/read misses about16–18ms. One run only; spatial/game audio remains guarded and pending. |
| vitaGL / diagnostic display | DIAGNOSTIC RENDERS | Current 00.05: diagnostic plus visible synthetic triangle, no reported error/crash, responsive input/rechecks, Start exit. No retail Halo renderer or repeated stability claim. |
| Dynamic vertex / pixel translators | DIAGNOSTIC RENDERS | Current 00.05: original two-MOV NV2A vertex + pixel combiner compile1/1, link1, user confirms visible triangle. Proves this synthetic pair only; retail shader coverage and cross-program multipass invariance pending. |
| Full D3D8 GL backend / textures | partial HALO DRAW RENDERS |00.25 hardware original logo/rectangular backgrounds, glyph upload and both original NV2A pairs pass (A077). Labels/transparency incomplete;00.26 adds color/shader/coverage evidence and mapped range reuse barriers, hardware improvements pending. |
| Framebuffer copy fallback | BOOTS | 00.03 tiny GPU FBO/blit/readback probe PASS RGBA=51,102,153,255 error0. Proves this bounded test, not full renderer copies/formats/performance. |
| Cubemaps / 2D retail textures | 2D partial RENDERS / cube pending | Original logo/background/font2D uploads observed on Vita (A077); exact DXT1/3/5/BGRA/stage/cache contracts PASS. Cubemap sampling/world materials and remaining bitmap visibility require original draw integration. |
| 3D textures | BLOCKED | No installed 3D upload API; sampler3D generation explicitly rejects. A011: campaign c10/c20 contain six volume bitmaps each, ui five; runtime sampling/a10 usage unknown. |
| Visibility / lens flares | BLOCKED | Query APIs exported; exact semantics untested. Android atomics not copied. |
| Main menu / campaign a10 | partial HALO DRAW RENDERS / world BLOCKED |00.31 hardware original lettering/focus and Campaign conditional entry/back (A094); other windows failed.00.32 links original profiles/keyboard/frame integration (A102), console acceptance pending. White backgrounds deferred; a10/full world update still unresolved. |
| Saves / networking | LINKS profiles / networking BLOCKED |00.32 original saved-game/profile owners, keyboard and native storage linked; host I/O contracts PASS. Console persistence and all-window acceptance pending; network/world handlers remain guarded (A102). |
| 30 FPS / stability | reported29–30FPS / STABLE unverified | A083 user reports29–30FPS in00.26 after prior13FPS/stalls, under maximum-overclock comparison context. Sampled frame bodies~19.8ms exclude outer pacing; uninterrupted audio and clean exit observed in one run. Full scene/repeated stability pending. |

Console test instructions, final package SHA-256 and crash-symbol commands: [BUILD.md](BUILD.md). Attempt history is append-only in [ATTEMPTS.md](ATTEMPTS.md).