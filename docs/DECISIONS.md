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

Validation A018 of D010:00.06 hardware reserves96MiB, retains original subregion offsets and passes original physical/state functions; both real compressed tag sections and directory APIs pass. Remaining user62MiB/CDRAM72MiB/phycont26MiB are one snapshot, not a peak/gameplay budget. Complete typed nested relocation and resource/platform contracts remain required.

## D011 — Typed real menu metadata and original D3D8 preparation

Date:2026-09-29; evidence A019. Validate a bounded field-aware journal completely before any write; named widget/reference/block/data layouts from original Halo only. Check datums/groups/names/bounds/alignment/counts, UTF16 termination, font indices, pointer/scalar conflicts and menu child cycles. Restore known fields/directory after original accessors and compare CRC. Shared DeLa struct views/asserts move unchanged from ui_widget.c into ui_widget_tags.h; no widget behavior change. Native sizeof/offsetof checks protect portable offsets. Link original bitmap/tag/font/string and16-bit game msvc_wide implementations; game wide strings never cross into SDK/newlib wide APIs. No third-party schemas or arbitrary word scanning.

Scope remains partial: other nested tag groups/scenario data, BSP payloads, GPU resource words, unknown widget spans and unused bitmap editor/pixel records are inactive. Actual UI initialization and complete scenario/resource mount remain required; tag resolution is not menu entry. ui-only startup tests this changed contract without repeating campaign decompression.

Preserve original d3d8_gl architecture: HALO_VITA binds16 NV2A attributes before link and stores192 individually queried constant locations, uploading active changed registers by exact location/count1. Additional768B/program; desktop/Android branches retained. These changes COMPILE/CPU-pass but are not whole-backend binding, uniform compaction, paired shader-cache policy or GPU proof. No constants/state/shader handles are faked.

## D012 — Persistent transactional UI tag mount for staged original startup

Date:2026-09-29; evidence A020/A021. A020 hardware validates D011's original accessors and exact CRC rollback. For the next checkpoint, retain that checked `ui.map` image in the existing tag-cache arena while original UI consumers run. Split mount, original-accessor validation, runtime use and unmount; journal original directory pointers, restore them and the typed fields on every unmount, and compare source CRC. Rechecks unmount before overwriting arena bytes. Original `ui_widgets_initialize` is the first linked runtime function, with a read-only state query. This is a temporary integration checkpoint, not a replacement for `scenario_tags_load` or a claim of menu activation.

Reason: the 00.07 load-inspect-detach lifecycle invalidated all tag pointers before the original widget code could use them. Keeping the existing typed transaction alive is the smallest extension that preserves its validated data and rollback semantics. No new rebase rule, arbitrary word scan, menu renderer, fake widget state or success stub is introduced. The full original root path still requires game/scenario/BSP state and event/game-time/audio contracts before invocation.

## D013 — Dispose the original UI pool only in the no-widget checkpoint state

Date:2026-09-29; evidence A022/A023/KI-018. The 00.08 Vita heap dump identifies the 16 KiB original `ui_widgets_initialize` pool still allocated at exit. The current checkpoint has initialized globals but never creates a widget. Add a HALO_VITA-only checked teardown in original `ui_widget.c`: refuse active widgets, call the original pool free, clear pool base/size and zero widget globals before replacing or unmounting the cache image. Use the full original `ui_widgets_dispose` once its active-widget/game-time/audio/event closure is implemented for real menu startup.

Reason: directly linking `ui_widgets_dispose` pulls the original close-all event graph, which is not yet linked or initialized. A fake implementation of those dependencies would compromise startup correctness. The narrow no-widget teardown mirrors the reachable tail of the upstream function and makes the observed allocation lifetime explicit; an active widget turns it into a logged blocker rather than being silently discarded. Its actual heap-dump result still needs Vita validation in 00.09.

Validation A024: the real 00.09 log shows 023 before each 020, with reinitialization after Cross and a clean Start exit. The user reports no new heap dump; the original dump writer creates the file only for outstanding guarded allocations. This validates the no-widget checkpoint lifecycle for the observed run. Active-widget disposal still requires the full original closure.

## D014 — Native Xbox D: attribute lookup through original Halo file references

Date:2026-09-29; evidence A025/A026. Keep original `file_reference_create_from_path`, `file_exists`, and `xbox_demos_available` rather than replacing the Main Menu child's creation handler. Implement their currently needed XDK boundary (`GetFileAttributesA`, per-thread `GetLastError`/`SetLastError`) under `port/vita/`: resolve each Xbox D: component case-insensitively under the user-owned data root, distinguish final-file and parent-path absence, and return actual Vita stat attributes. Constrain unsupported drives/traversal with a real failure. Run a contract checkpoint before root activation and require Vita evidence for 024 before relying on it.

Reason: the real child creation event executes before the root's `main_menu_initialize`, and the existing map-only Vita lookup cannot satisfy arbitrary original file references. A direct XDK contract preserves the upstream call path and is reusable for later startup files. No broad fake-success filesystem layer is introduced; full open/read/seek/enumeration and overlapped semantics remain separate work. Source-shared Halo file code is linked unchanged.

Validation A027: real Vita 00.10 logs 024 with `casefold_ui=1 maps_directory=1 missing_file=1 XDemos=absent`, plus separate worker/main last-error values and the prior UI recheck/cleanup. This proves the invoked attribute path; full file I/O and widget event execution remain untested.

A029 preserves an exact complete 00.10 Vita log with the same 024 result, Cross remount, Start teardown and clean exit. It supplies inspectable evidence after A027's first log was accidentally deleted.

## D015 — Selective original UI-event dispatch for Vita root bring-up

Date: 2026-09-29; evidence A030. The original 102-function-pointer event table retains unused multiplayer/campaign/profile closures. Under HALO_VITA only, dispatch by the same index to the original functions 0, 23 and 86 used by the current root creation; unsupported indices (including present but interactive 87/101) fail visibly. Desktop/Linux/Android keep the original table. The actual user-owned `ui.map` graph has nine widgets, creation events 86 then 23 with run-function flag 0x80, and no game-time-pause definitions. The bring-up path accepts only that observed event-effect subset, with a logged failure if the data differs. This is a temporary linker-boundary decision, not a new menu implementation.

The UI-only checkpoint initializes original player UI before root creation and calls original `main_menu_initialize`, including original player UI, initial-null network disposal, remote-connection flag, spawn count, profile state and music lookup. Shared-source changes are guarded by HALO_VITA/HALO_VITA_MENU_BRINGUP. Music playback is deferred explicitly because original game sound state is not initialized; the widget music-active flag remains false. If root becomes active, process exit retains the mounted cache/arena instead of freeing live tag pointers. Full teardown and original update/render remain future contracts. A031 real Vita confirms 00.11 root and both required creation handlers BOOTS; it does not validate rendering or full teardown.


## D016 — Treat Xbox map bitmap records as compiled runtime metadata

Date 2026-09-30; A038/A039. The original cache loader does not run bitmap-group postprocess. The Vita typed mount must preserve already cached bitmap flags, absolute pixels_offset, pixels_size and tag_index, and validate unloaded handles/ranges before the first draw. It must not clear cached to replay texture_cache_bitmap_new. That hook adds the group's resource offset and belongs to building/processing uncached tags; replaying it changes an already absolute address. No shared game source or asset-specific replacement is needed. Raw unprocessed bitmap records are rejected for this Xbox-v5 cache path.


## D017 — Cache complete eager NV2A shader pairs on Vita

Date 2026-09-30, A041. vitaGL SHADER_PAIR alternates its varying-semantic pool per actual compile. Vita therefore caches the original generated VS+PS together, keyed by original vertex ID/packed declaration/immediate flag and complete original pixel key, and compiles both in explicit order. Reuse the existing program linker, attribute binding and original shader generators; preserve independent caches elsewhere. Avoid POSTPONED (A014 NULL-program crash), dependence on C argument order, or pair state left half-complete by independent cache misses. Uniform arrays use individually queried active element locations. This changes the graphics platform boundary, not the game's material/shader equations. Pair-cache retention until process exit matches the staged renderer's existing program lifetime; reset/teardown work remains separate.


## D018 — Restore original shell-state owners used by rasterizer queries

Date2026-10-01; A042. The UI-only frame still invokes original window-count and cinematic/player queries. Add the original clock/player/control/cinematic allocations and per-map resets under the Vita renderer owner instead of returning hardcoded query values or allocating imitation globals. Original gameplay/input updates remain disabled. State is allocated once from the existing process-lifetime game-state buffer, as for the staged decal owner; full game_initialize integration must later reconcile these owners to avoid double allocations.


## D019 — Serialized cached bitmap pointers are opaque before LRU load

Date2026-10-01; A045/A046. Refine D016's cold-cache validation using actual00.17 hardware and original source: cached bit + cache_block_index NONE defines an unloaded bitmap; serialized base_address/hardware_format need not be NULL. Preserve these fields without rebasing or consuming them. Original texture_cache_start_loading_bitmap must replace base with the new LRU address before hardware registration/read; cached hardware lookup selects the live cache-entry hardware instead of bitmap.hardware_format. Keep matching owner and resource bounds, and reject a foreign resident cache index. This avoids both a false rejection and a fake bitmap registration.


## D020 — Completed subsystem flags own pre-root failure teardown

Date2026-10-01; A047. Menu readiness can fail after original rasterizer allocation succeeds. Track original completion independently and invoke original close/dispose owners before tag/arena/context teardown, only before an active root exists. Keep original game-owned decal destruction before device destruction. Avoid synthetic frees, hiding allocation reports or treating a failed partial rasterizer initializer as fully initialized. Active-root shutdown and complete platform GL cache reset need their own validated owner contract.

## D021 — Vita depth objects retain one owning FBO

Date2026-10-01; A049/A050. Adapt the original D3D8 Z/stencil surface to vitaGL's exposed renderbuffer request API, retaining a separate handle from sampleable color textures. vitaGL6e7fe40 implements physical depth/stencil per framebuffer, not per renderbuffer; attaching one RB to multiple color-keyed FBOs would silently lose shared depth identity. Therefore cache the Vita depth FBO by depth handle and update only an equal-size color attachment. Never reattach the RB on a cache hit. Color-only read/copy FBOs remain keyed by color. Preserve original Z/stencil clears/tests; unsupported depth-only, size mismatch and depth sampling are not fake-success paths. Lazy physical allocation and STORE_DEPTH_STENCIL persistence remain hardware/build capability gates. Non-Vita backend behavior remains unchanged.

## D022 — Reuse original streamed color conversion on Vita

Date2026-10-01; A051 and renderer plan6.5. vitaGL rejects desktop's GL_BGRA attribute-size convention. Share the existing Android CPU B/R byte swap for declared D3DCOLOR elements, then use4 normalized unsigned bytes. Keep authored source data immutable and existing strides/registers/shader equations intact. Add bounded Vita allocation/declaration checks; do not assume color values are all white or remove color attributes. Immediate float-color path remains original. Host byte/format tests are not a hardware draw test.

## D023 — Express tight upload rows through supported vitaGL pixel state

Date2026-10-01; A054/A055. Preserve original Xbox decoding and direct DXT uploads; replace only the Vita unsupported UNPACK_ALIGNMENT request with supported UNPACK_ROW_LENGTH0. Decoded output is tightly packed32-bit BGRA, and compressed blocks do not consume byte-row alignment. Reset row stride explicitly without altering authored texture dimensions, compression, mip offsets or cache data. Check every upload phase/level. Do not call a declared-but-rejected token, suppress0x500, discard mip levels or replace the real bitmap with a placeholder.

## D024 — Scale screen targets while retaining original Xbox coordinate space

Date2026-10-01; A056, explicit user request. Choose original640x480 game/UI coordinates and320x240 internal screen color/depth through existing screen_scale0.5. Preserve original viewport/clear/scissor/texture-target and aspect-preserving Present behavior; keep native960x544 context as the final display surface. This avoids unsupported sub640 game coordinates and an untested new scanout mode.4:3 presentation is intentional, with side bars. Named platform dimensions make policy and logs explicit. Native display memory/non-screen resources/CPU arena remain; pixel-count reduction is not an FPS or GPU-overload-resolution claim. Hardware must verify the new mode before RENDERS.


## D025 — Reuse Vita stream storage only after explicit GPU completion

Date2026-10-01; A058/A059/KI-028. The exact00.20 core/ELF identifies an unchecked NULL allocation in vitaGL whole-buffer SubData cloning during original font drawing. Keep original bounded2MiB/256KiB allocations and draw bytes, but synchronize and map existing storage on Vita. Reserve all streams of a draw before offsets are assigned; copy only authored byte count, retaining16-byte offset alignment. Reuse on wrap instead of orphaning. Check map/unmap/errors and bounds. Synchronous GPU waits trade throughput for a bounded correct bring-up contract; no asynchronous safety, measured FPS, depth persistence or visible-menu claim. Do not enable a global driver copy speedhack or increase memory to hide per-glyph cloning. Non-Vita remains original.


## D026 — Complete the texture transfer queue before destination/staging reuse

Date2026-10-01; A061/A063/KI-029. vitaGL DXT mip uploads schedule asynchronous GXM transfers and grow destinations between levels. Wait separately through the native sceGxmTransferFinish bridge after each mip and before subsequent mutation; drawing-context glFinish does not replace this. Keep original compression, mip data and decoded staging; fail on SDK errors. Synchronous bring-up sacrifices throughput while protecting lifetime. Exact dump identifies allocator corruption, not its sole writer; hardware confirmation required. No mip truncation/global upload speedhack/pool increase.

## D027 — Reflect base-array spans for installed non-strict uniform ABI

Date2026-10-01; A062/A063/KI-030. Exact linked glUniform4fv uses a whole-uniform pointer and offset0. Supersede D011/D017's element-location assumption for this native archive. Reflect active vec4 array length/base name and upload its checked complete original prefix when dirty; do not manufacture indexed handles or add offsets to locations. Preserve all192 CPU constant registers, original indices/arithmetic and pixel array shadows; inactive compiled arrays need no upload, malformed/oversize contracts reject. Shader constant compaction remains future work; this does not assert that all192 vectors fit all programs. Previous host-only sparse-location evidence remains historical, not native proof. Non-Vita remains unchanged.


## D028 — Expand packed normals at the Vita stream boundary

A065: reuse the exact original NV2A unpack arithmetic on CPU. Keep authored interleaved data immutable; upload separate float3 attributes into the existing bounded stream buffer, reserving the complete draw first. Retain original declaration/register identities and shader instructions. Avoid GLSL integer attributes and modifications to game model data; preserve desktop/Android paths. Hardware geometry and full scenario execution are still unverified.


## D029 — Exact bounded source snapshots replace unavailable Vita page watching

A066: compare guest texture bytes at every bind; skip GPU reupload only when an existing checked GPU copy has an exact source/palette match. Bound total CPU shadows to4MiB; budget/allocation misses refresh safely. Dynamic font writes, LRU memory reuse and palette changes remain visible. Keep the original format/decoder/mip/transfer owners. No hash-only cache validity, stale frame shortcut, mprotect emulation or unmeasured FPS claim.


## D030 — Reuse original native frame time for the staged renderer

A067: expose original main_update_time_unthrottled through a Vita-only bridge; advance original UI render timestamp from the same real system clock. This restores the time inputs used by existing render equations while full process_ui_widgets is still unlinked. Keep original clamps/profiler/frame index. It supplies no event, automatic close, scenario tick, gameplay, input or audio update and must be reconciled when full main_loop owns these clocks.


## D031 — Start the original presentation consumer at staged Vita Present

Date2026-10-01; A070/KI-032. Keep the backend60Hz worker, two-flip limit, counters, callback and pthread synchronization. Vita Present must ensure this consumer exists because the staged root/render loop bypasses main_initialize_time. This is lazy platform lifecycle restoration, not a fake completion counter or an interpolation/queue bypass. Worker creation failure stops explicitly. The original complete-main callback can later reuse the same worker; desktop/Android paths remain unchanged.


## D032 — Isolate texture mutation to its intended Vita stage

A072/KI-033: original texture uploads/mip composition mutate GL binding state. Select their destination stage before mutation; shadow-cache hits cannot substitute for that selection. Preserve original resources, shaders and sampler assignments. Do not hardcode a transparent color or alpha discard to hide the symptom.


## D033 — Sound metadata relocation excludes external samples

A073: use original snd!/lsnd schemas and header layout assertions. Rebase only pitch/permutation/track/detail block pointers and reference names. Original sample resource addresses/file offsets and compiled runtime cache fields remain opaque, including Xbox-looking words. The original cache reader/mixer owns later activation; no replacement music files or arbitrary pointer scanning.


## D034 — Stage original 2D sound owners without inventing world semantics

A074: use the original classes/manager/cache, looping state machine, UI feedback and existing DirectSound-SDL backend. Rebase metadata through D033; external samples retain original addresses/offsets. Validate cold runtime state instead of resetting arbitrary serialized words. Arm music from original UI startup and refresh it after completed first presentation so shader compilation cannot consume its opening queue. Complete callbacks only on the original consumer, outside mixer lock. Require a real native SDL stream; reject decode/device failures explicitly. Initialize time/idle service delegation only with a ready sound cache and dispose voices/cache before stream/tags. Exact-signature unavailable spatial/world guards fail loudly and must be removed when real scenario/observer/object owners initialize. Square is an explicit sound diagnostic, not fake menu navigation. No audible or full-scene claim from host/native link evidence.


## D035 — Free the combined LRU allocation through its real owner

A077: keep original lruv_new/initialize combined storage. Verify/clear embedded datum metadata at delete without passing its interior pointer to debug_free; free the owner once. A signature check is evidence of the invalid caller, not a check to disable. Limit correction to Vita while other-platform compatibility is outside this experiment.


## D036 — Make compressed maps seekable with a validated disk backing stream

A078/KI-036: prepare one exact logical stream before runtime resources activate, with bounded64KiB inflate scratch and original tags/offsets/cache ownership. Device-generated scratch consumes the map's logical size on disk and is removed at unbind/failure. Always regenerate per bind; do not accept an old cache by name/version alone. Keep original map read-only, validate complete checksum/length/write/flush before handle publication, and fail explicitly if storage is unavailable. Live requests seek only their original range. This retains synchronous completion and does not claim fully asynchronous cache I/O or an audio scheduling replacement.


## D037 — Reuse barriers protect submitted ranges; fresh appends need no global wait

A079: after inspecting primary vitaGL mapping's fixed storage semantics, remove per-upload global completion from fresh append ranges. Preserve the bounded buffer owner, exact-byte writes, alignment and full same-draw reservation. Synchronize before every vertex/index wrap so CPU never overwrites submitted reads. A host negative fixture must reject a missing reuse barrier. No SubData cloning/orphaning/unsynchronized overlap or arbitrary pool growth; console FPS/depth behavior remains pending.


## D038 — Controlled original DXT3 decode fallback for Vita UI masks

A084/A085: reuse the existing original Android decoder for DXT3 with checked BGRA staging and original mip/face/shader/cache contracts. Exact user map shows auxiliary label text alpha0 but real bitmap-letter alpha; host original fragment replay preserves it. This is an explicitly pending Vita sampling experiment, not proof of a driver fault or complete UI fix. Native DXT1/DXT5 remain unchanged. The decoded GPU footprint is4x DXT3 source bytes before driver alignment; budget future scene residency before making this a permanent all-scene policy. No authored data/opacity changes, color keys or synthetic labels.
