# Attempt log — append only

Do not rewrite old failed attempts. Add a new entry when a later change supersedes one.

## 2026-09-28 — A000 — Project scaffold

**Goal:** create a persistent Vita-specific engineering workspace before code changes begin.

**Hypothesis:** a structured status/attempt/decision system will prevent repeated dead ends across Codex, ChatGPT and manual work.

**Changes:** created repository documentation, Vita directory placeholders, environment rules, roadmap and first Codex prompt.

**Result:** SUCCESS — organizational milestone only. No Vita source build attempted yet.

**Next:** import upstream source and record its exact SHA, then start A001 with a native ARM compile audit.

## 2026-09-28 — A000.1 — WSL script CRLF bootstrap failure

**Goal:** run `scripts/import-upstream.sh` under Ubuntu WSL.

**Observed error:** `/usr/bin/env: 'bash\r': No such file or directory` (rendered by WSL as `env: $'bash\r': No such file or directory`).

**Cause:** the local checkout converted shell-script line endings to Windows CRLF, so the shebang was parsed as `#!/usr/bin/env bash\r`.

**Fix:** enforce LF for shell/build scripts through `.gitattributes`; existing affected local checkouts can be repaired with `sed -i 's/\r$//' scripts/*.sh`.

**Result:** ROOT CAUSE IDENTIFIED / REPOSITORY POLICY FIXED. Runtime import should be retried after local normalization.

**Do not repeat:** do not debug Bash, `env`, PATH, or VitaSDK for this error until checking line endings first.

**Next:** normalize the current WSL checkout and rerun the upstream import.

## 2026-09-28 — A001 — Imported baseline and native SDK discovery

**Goal/hypothesis:** use the existing native ARM32 SDK without changing installed SDKs.

**Environment/commit:** Ubuntu WSL; HaloCEVita `080e0c5b2eca00e809f5b2c5b70fbbd004ca8f4a`; imported upstream `.upstream-base-sha` = `21714ac0860e9b9ca08fbdc8a1d620f1b8a03797`. Required source/, port/linux/, port/android/, configure.py and tools/ already exist; no reimport needed.

**Commands:** `bash scripts/check-env.sh`; `/usr/local/vitasdk-hardfp/bin/arm-vita-eabi-gcc --version`; `/usr/local/vitasdk-hardfp/bin/arm-vita-eabi-gcc -Q --help=target`; corresponding commands for `/usr/local/vitasdk`.

**Result:** initial VITASDK/PATH unset; CMake 4.2.3 present, Ninja/clang absent. `/usr/local/vitasdk-hardfp` has GCC 15.2.0, default `-mfloat-abi=hard`; `/usr/local/vitasdk` has GCC 10.3.0, default `softfp`. Standard SDK contains vitaGL, VitaShaRK and SDL3 headers/static libraries.

**Change/decision:** select `VITASDK=/usr/local/vitasdk-hardfp`, prepend its bin directory only in project commands. Neither SDK mutated. Begin GCC native compilation using the upstream generated MSVC semantics and an isolated Vita prefix.

**Do not repeat:** do not use the unqualified SoftFP SDK or infer ABI from directory naming alone. No runtime state proven yet.

**Next:** compile real game units; audit actual structure sizes and renderer API against installed headers and archive symbols.

## 2026-09-28 — A002 — First native game unit / newlib prefix conflict

**Goal/hypothesis:** compile untouched `source/memory/data.c` with VitaSDK GCC and upstream semantic header.

**Environment/commit:** A001 native hardfp SDK and baseline.

**Command:** `python3 tools/linux_msvc_semantics.py --output build/vita/halo_msvc_semantics.h --all-inlines --tags source --inlines source --inlines port/include/xdk`; `arm-vita-eabi-gcc -std=gnu89 -D__STRICT_ANSI__ -DDEBUG -Dxbox -fms-extensions -fshort-wchar -fcommon -fno-strict-aliasing -fwrapv -ffp-contract=off -fno-omit-frame-pointer -O2 -g -include port/vita/include/halo_vita_prefix.h -include build/vita/halo_msvc_semantics.h -Iport/vita/include -Iport/linux/include -Isource -Isource/cseries -Isource/memory -Isource/tag_files -idirafter port/include/xdk -c source/memory/data.c -o build/vita/data.o` (stdout/stderr: `build/vita/logs/gcc-data.txt`).

**Failed result:** newlib `stdio.h` reports `duplicate 'static'`; `stdlib.h` reports unknown `wchar_t`; Halo's `LONG_MAX` enum collides with newlib macro. Linux prefix assumes glibc/clang: `_WCHAR_T_DEFINED` suppresses GCC typedef and redefining `__inline` reaches newlib.

**Change:** preload libc headers before upstream MSVC macro environment; undefine only the integer-limit macros conflicting with Halo's enum. Retry justified by corrected header ordering, not warning suppression. Added HALO_VITA to upstream architecture guard; no game algorithms changed.

**Conclusion/do not repeat:** do not force-include the Linux prefix directly before newlib or use glibc-only inline assumptions. Next: repeat compile and retain errors for GCC-specific semantics.

## 2026-09-28 — A003 — Full native GCC compile audit / inline linkage

**Goal/hypothesis:** expose the remaining ARM/GCC blockers across all configured upstream game C units without deleting modules.

**Environment/commit:** A001 SDK/baseline. **Command:** `PATH=/usr/local/vitasdk-hardfp/bin:/usr/bin:/bin VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_compile_audit.py --jobs 6 > build/vita/logs/gcc-audit.txt 2>&1`. Exact per-unit commands/errors retained under `build/vita/audit/`; report.json lists results.

**Result:** 280/466 units COMPILES; 186 fail. Major common signatures: `xdk_d3d8.h: duplicate 'static'` and `static declaration of D3DDevice_SetRenderState follows non-static declaration`. Newlib wint_t is already declared (32-bit); it must not be redefined by the Linux wide wrapper. `wchar_t` remains 16-bit.

**Changes:** isolated Vita prefix marks newlib wint_t present; GCC uses ordinary gnu89 inline plus generated weak definitions instead of clang's static-inline extension. All original source remains. Retry uses `--output build/vita/audit-inline` to preserve initial logs.

**Conclusion/do not repeat:** GCC cannot tolerate duplicate static or static redeclaration after external prototype; use weak external inline copies, then check weak undefined symbols at link. Next: rerun and classify remaining errors, build a real-core executable.

## 2026-09-28 — A004 — Native core CMake/VPK build, first generator failure

**Goal/hypothesis:** link real Halo cseries/debug heap/profile/data/pool/CRC/cache-header verification and upstream NV2A translators into a native executable while auditing the rest separately.

**Commands:** `VITASDK=/usr/local/vitasdk-hardfp PATH=/usr/local/vitasdk-hardfp/bin:/usr/bin:/bin cmake -S port/vita -B build/vita -G Ninja -DCMAKE_MAKE_PROGRAM=/home/vegettosandev/HaloCEVita/build/vita/tools/ninja/usr/bin/ninja`; same environment `cmake --build build/vita -j6`. Logs: `build/vita/logs/cmake-configure.txt`, `cmake-build-a004.txt`.

**Environment:** A001 SDK. Missing Ninja supplied by `apt-get download ninja-build` / `dpkg-deb -x build/vita/tools/ninja-build_1.13.2-1_amd64.deb build/vita/tools/ninja`; extracted only within ignored build directory. No SDK/system installation changed.

**Result:** configure succeeds. Compile fails because CMake deduplicates the second `-include`, turning the semantic header into another input: `cannot specify '-o' ... with multiple files`.

**Change:** keep each force-include option group together with CMake `SHELL:`; native libc and game short-wchar compilation remain separate. New Vita startup records logs/controls/map-header checks/native service probes and diagnostic graphics, with milestone 010 deliberately withheld. No game menu/main entered yet.

**A003 retry result:** 433/466 COMPILES. Remaining errors: GCC weak declarations of static inline functions, MSVC `ui64` literal suffix, terminal_printf's `char *` pretending to be va_list, old libtiff malloc declarations and newlib isspace macro collision. Fixed each specifically under HALO_VITA or in Vita semantic generation; final audit pending.

**Next/do not repeat:** do not repeat ungrouped CMake -include flags. Resolve actual compile/link signatures and inspect output imports; no success claim based on configure.

## 2026-09-28 — A005 — Header boundary and native service build

**Commands:** A004 environment, `cmake --build build/vita -j6`, logs `cmake-build-a004-retry.txt`, `cmake-build-a005.txt`.

**Failures:** XDK/newlib `fd_set`, `timeval`, `select` collisions in the new service unit because it included xtl directly under the platform macro. Fixed by including upstream platform.h first, which implements the exact Winsock namespace boundary. Next compile found `vglEnd` is not declared by installed vitaGL; removed the assumed call after verifying the real header (process teardown handles global GPU pools).

**Changes:** all selected actual Halo units and NV2A translator units now compile past these points; native Vita/SDL3/pthread glue compiles. Also corrected a Vita-only upstream debug diagnostic's incompatible csprintf cast: it omitted the destination buffer and could crash while reporting heap corruption. Preserve original branch for other targets.

**Conclusion/do not repeat:** use the upstream platform header boundary and installed vitaGL APIs; do not follow x86 stack chains on ARM. Next: resolve linker signatures and packaging.

## 2026-09-28 — A006 — Link service imports / SDK stub spelling

**Commands/environment:** A001/A004; `cmake --build build/vita -j6`; logs `cmake-build-a006.txt`, `cmake-build-a006-retry.txt`.

**Failures/result:** linker first rejects nonexistent `-lSceThreadmgr_stub`; actual `psp2/kernel/threadmgr.h` specifies `SceKernelThreadMgr_stub`. After correcting the name, only missing game-core imports are halo_linux_fopen/fprintf from the real heap dump. Implemented logging/file wrappers under port/vita; kept upstream debug allocator, not a replacement malloc-only heap.

**ABI observation:** GNU ld reports game short-wchar versus native newlib/SDK wchar4. The explicit bridge uses only narrow strings, integer sizes, pointers and floats; no native wide CRT calls are used by the linked game closure. Warning is retained and documented, not hidden. No hard/soft-float mix rejection.

**Next/do not repeat:** resolve actual imports, never add unresolved-symbol linker bypasses or substitute a commercial SDK stub name by guess.

## 2026-09-28 — A007 — First real-core native ELF/SELF/VPK

**Command:** A004 native environment, `cmake --build build/vita -j6 > build/vita/logs/cmake-build-a007.txt 2>&1`.

**Result:** LINKS. `build/vita/HaloCE.elf`, `.elf.map`, `.elf.velf`, `eboot.bin` and `HaloCE.vpk` generated by native GCC/linker and VitaSDK packaging. First VPK SHA-256 `2db784d600e0239941f7b806ed0135166376a51f508c4e5bb9dedea2bcd6c964` (superseded by later ABI/input/probe changes; use final BUILD.md hash).

**Changes/evidence:** package includes only authored LiveArea images/XML, SFO and eboot. Real cseries/debug allocator/profile/data-array/compacting-pool/CRC/map-header verifier/NV2A translators retained in the linked closure. readelf confirms ELF32 ARM EABI5, hard-float VFP arguments; debug symbols/map kept. `nm -u` shows only optional SDK/CRT weak hooks, no unresolved game functions. No Vita/Vita3K runtime execution; no BOOTS/RENDERS claim.

**Next:** stricter enum/signed-char/variadic ABI audit, complete configured game compilation and graphics API inventory before handing over a console test build.

## 2026-09-28 — A008 — Wider ARM ABI and installed graphics audit

**Commands:** `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_gl_audit.py`; A001 native environment `python3 tools/vita_compile_audit.py --jobs 6 --output build/vita/audit-final`; `arm-vita-eabi-readelf -h -A build/vita/HaloCE.elf`; `cmake --build build/vita -j6 > build/vita/logs/cmake-build-abi.txt 2>&1`.

**Result:** 465/466 configured game C units COMPILES (before strict-enum changes); only ai_debug_drawstack_setup external prototype/static definition conflict remains. Fixed prototype only under HALO_VITA. Graphics inventory verifies 108 entry points against the installed vitaGL.h and libvitaGL.a (93 declarations/exports present, others classified with exact evidence).

**ABI changes:** SDK GCC defaults to variable-sized enums (object attribute observed). Force `-fno-short-enums` in game units to preserve MSVC enum32, `-fsigned-char`, and assert enum size and 64-bit-member offset8. SDK/libc/native units keep their real ABI. Reuse upstream Android variadic prototypes and the hs.c fade-in/out float signature fix for Vita hard-float. Repeat compilation justified by changed ABI flags, not an identical retry. Explicit struct assertions and existing upstream layout checks remain enabled.

**Shaders:** keep NV2A operations/combiner logic; Vita-only GLSL1.20 attribute/varying/fragment-output/lookup syntax. NORMPACKED3/sampler3D/atomic sample counters log/reject until supported paths exist. The installed archive imports runtime GLSL/VitaShaRK/ShaccCg; shader probe therefore needs console libshacccg, which is never packaged. No assertion of runtime shader success yet.

**Next/do not repeat:** complete enum32 audit under `build/vita/audit-abi`, aggregate all game objects to reveal platform imports, exercise first-class root Ninja targets. Never use SDK default short enums for Halo wire/layout types or assume that compilation validates function-pointer ABI.

## 2026-09-28 — A009 — All 466 native game units and complete import inventory

**Commands/environment:** A001 native SDK; `python3 tools/vita_compile_audit.py --jobs 6 --output build/vita/audit-abi`; then `python3 tools/vita_compile_audit.py --output build/vita/audit-abi --retry-failed`; `python3 tools/vita_link_audit.py`; platform audit `python3 tools/vita_compile_audit.py --platform --only port/linux/src/dsound_sdl.c port/linux/src/d3d8_gl.c port/linux/src/xbox_textures.c --output build/vita/audit-platform`.

**Failure:** initial ABI audit 465/466: ui_widget_event_handler_functions.c locally declares `error(long,char*,...)`, conflicting with actual `error(short,const char*,...)`. Corrected declaration only for HALO_VITA. Shared flags/headers unchanged, so only the failed unit was retried; original error saved as .failed.log.

**Result:** 466/466 COMPILES, report under build/vita/audit-abi. All objects aggregate with native ld -r into HaloGame.audit.o; 584 unresolved CRT/XDK/platform imports inventoried in game-imports.txt. This is not a final Vita link. Actual dsound_sdl.c/d3d8_gl.c/xbox_textures.c also compile 3/3 against upstream desktop GL declarations (not a finished Vita backend).

**Root build:** `VITASDK=/usr/local/vitasdk-hardfp PATH=/usr/local/vitasdk-hardfp/bin:/home/vegettosandev/HaloCEVita/build/vita/tools/ninja/usr/bin:/usr/bin:/bin python3 configure.py --release`; same environment `ninja vita_vpk` succeeds (root-configure.txt/root-vita-vpk.txt). Vita first-class targets delegate to CMake; no SDK install/update or history rewrite.

**Next/do not repeat:** incrementally implement 584 boundary imports with real types/semantics and memory/tag rebasing; do not mistake relocatable aggregation or desktop-header renderer compilation for runtime Vita support.

## 2026-09-28 — A010 — Native input, GPU-copy probe and final package verification

**Goal/hypothesis:** make the linked milestone useful for testing actual Halo/Vita interfaces, not a hello-world display.

**Changes:** native Vita controls->Xbox mapping plus typed XInputOpen/GetState/Close with compile-time layout checks; Halo core calls the XDK-facing bridge. White/Black/L3/R3 left unassigned for ergonomic testing. Add Android-derived framebuffer/glBlitFramebuffer level-copy path and known-color GPU readback probe, retaining NV2A shader-generation tests. Initial Vita stream defaults 2 MiB / indices 256 KiB / ring 1 documented; not allocated by diagnostic VPK, full flush strategy pending. Current active pools remain 64 MiB newlib cap + 16 MiB GL RAM + 24 MiB CDRAM + 2 MiB legacy. No measured FPS/memory assertion.

**Commands:** A009 environment `ninja vita_vpk > build/vita/logs/final-build.txt 2>&1`; automatic `python3 tools/vita_verify.py`; `python3 tools/vita_compile_audit.py --platform --only port/linux/src/d3d8_gl.c --output build/vita/audit-stream` succeeds after budget change; `git diff --check` clean.

**Verification result:** LINKS. Final package SHA-256 `6a323854be1c7eae452379769d31926eb5522610a1a51652f2d24d94a8595498`. ELF32 ARM hard-float/required actual Halo symbols verified. No unresolved game hooks; eight known optional SDK/CRT weak hooks retained. Valid SELF, VPK ZIP CRC, TITLE_ID HCEV00001/APP_VER 00.01, matching eboot bytes, authored 128x128 icon. Exactly six package entries; no maps/retail data/saves/compiler module. SHA256SUMS/artifacts.json retain ELF/map/package hashes.

**Runtime result:** not executed. No available attached Vita/discovered Vita3K install; BOOTS/RENDERS/PLAYABLE/STABLE remain unproven. Installed libvitaGL archive query implementation inspected; exact query semantics still require hardware. Console probe outcomes are expected checks, not reported successes.

**Conclusion/next:** hand over executable and exact console instructions in BUILD.md. Obtain debug.txt/shader sources/crash PC or dump with matching package hash to advance from LINKS. Full main still blocked by placed-memory/cache pointers, XAPI and renderer compatibility (KI-007/008/009); keep sources and 584 import inventory for the next milestone.

## 2026-09-28 — A011 — Real Xbox campaign volume-resource inventory

**Goal/hypothesis:** establish whether 3D textures occur in campaign content before designing a renderer fallback. Prior A008/A010 only established the missing Vita API and upstream volume code paths; no failed idea was repeated.

**Environment/commit:** A001 baseline; user-provided existing files under F:/UtilidadesHaloCe/xemu-win-x86_64-release/Halo/Data/maps, accessed read-only through WSL. Native executable/package unchanged.

**Command:** `python3 tools/vita_map_audit.py /mnt/f/UtilidadesHaloCe/xemu-win-x86_64-release/Halo/Data/maps/ui.map /mnt/f/UtilidadesHaloCe/xemu-win-x86_64-release/Halo/Data/maps/a10.map /mnt/f/UtilidadesHaloCe/xemu-win-x86_64-release/Halo/Data/maps/*_DECOMP.map > build/vita/logs/map-metadata-a011.jsonl`.

**Changes:** add bounded, metadata-only host utility using the exact upstream cache tag header/32-byte instance, bitmap-group/tag-block and 48-byte bitmap layouts. It verifies Xbox v5/header/footer/tag signatures and pointer bounds; it never reads/extracts pixel payloads, decompresses, rewrites or packages maps.

**Result:** ui/a10 retail headers are Xbox v5 build 01.10.12.2276; compressed tag sections are explicitly skipped. Existing decompressed ui has 983 tags, 195 bitmap groups, 368 2D / 5 volume / 11 cube bitmaps. Campaign c10: 3369 tags, 476 bitmap groups, 830 2D / 6 volume / 27 cube; c20: 1860 tags, 334 bitmap groups, 713 2D / 6 volume / 23 cube. Beavercreek/bloodgulch each also contain six volumes. Volumes: distance attenuation 32x32x32, default-3d four 4x4x4 bitmaps, plus Elite plasma-shield noise 32x32x32 in campaign/multiplayer. All metadata parses pass; JSON log retained under ignored build/vita.

**Conclusion/do not repeat:** campaign content demonstrably contains 3D resources, so absence of the Vita upload API is a real compatibility blocker. Metadata presence does not prove runtime sampling, shader acceptance or cache loading. Do not infer a10 support from another map or silently substitute 2D texture sampling. No retail assets were imported into Git/VPK; maximum executable state remains LINKS.

**Next:** obtain console probe results first; once full tag/renderer integration is available, trace active volume bindings/shader modes and validate a bounded fallback with correct filtering.

## 2026-09-28 — A012 — VitaShell 0x8010113D installation feedback / indexed LiveArea correction

**Goal/hypothesis:** repair the concrete installation failure reported by the user at the end of installing 00.01. Prior attempts had no hardware installer evidence; neither boot nor install success had been demonstrated. Searching history found no earlier attempt for this error. The old ZIP/signature/dimension verifier was insufficient for Vita's image-format contract.

**Environment/commit:** A001 native SDK/baseline. User's real Vita/VitaShell reports `0x8010113D`; firmware/VitaShell version were not supplied. Original package SHA-256 `6a323854be1c7eae452379769d31926eb5522610a1a51652f2d24d94a8595498`. Original VPK/ELF/eboot/map/manifest/hashes saved without overwriting under ignored build/vita/attempts/a012-original-rgba/.

**Inspection/evidence:** all original images are PNG depth 8/type 6 (RGBA): icon128x128, bg840x500, startup280x158. VitaSDK's sample README requires indexed palettes; a primary LiveArea format investigation associates 0x8010113D with incompatible images and disallows alpha channels for icon/background. Sources: https://github.com/vitasdk/samples#notes-on-images and https://github.com/hammerill/livearea-specs. Existing upstream Vita CMake/tooling and our exact asset generator/verifier were inspected; engine/SDK changes are not needed for this repair.

**Changes:** tools/vita_assets.py emits PNG-8/type 3, two-entry RGB PLTE, no alpha/tRNS; same authored artwork. tools/vita_verify.py checks all three images, chunk CRC/order, palette entries, bounded decompressed rows, filter/palette-index validity, dimensions, XML references and APP_VER. CMake keeps TITLE_ID HCEV00001, increments APP_VER00.01 -> 00.02. No proprietary assets/dependency installation or engine edits.

**Commands:** `VITASDK=/usr/local/vitasdk-hardfp PATH=/usr/local/vitasdk-hardfp/bin:/home/vegettosandev/HaloCEVita/build/vita/tools/ninja/usr/bin:/usr/bin:/bin ninja vita_vpk > build/vita/logs/build-a012-livearea.txt 2>&1`; `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_verify.py`; `python3 -m py_compile tools/vita_assets.py tools/vita_verify.py`; `git -c core.safecrlf=false diff --check`. Python metadata/regression comparison logs: attempts/a012-original-rgba/negative-verifier.txt and logs/livearea-a012-validation.txt.

**Results:** old images all rejected by the new PNG checker with `require indexed PNG-8/type 3; got depth=8, type=6 (0x8010113D risk)`. New package passes; images all type3/depth8/two palette entries/no alpha, with exact decoded pixel equality. eboot SHA-256 remains `1cfc9bc898ecc135dc93e9e5c9a063299f7e581d519502101cfc0519e71a6132`; ELF and map also byte-identical. Corrected VPK SHA-256 `a3801dd4e0e39b14d732e6ec51a87f5ad37f09986136e0c57b9987b7a93320c8`.

**Conclusion/do not repeat:** the original package is an explicitly recorded hardware installation failure, not an installed application. The identified incompatible PNG format is corrected locally; installation/boot of 00.02 await user retry. Maximum state remains LINKS. Do not claim installer acceptance from ZIP/PNG validity or repeat type6 artwork. No debug.txt is expected from an application that was never launched.

**Next:** transfer/install the corrected VPK and report completion or exact remaining installer code/stage. After successful install, launch and collect the existing diagnostic milestones before claiming BOOTS/RENDERS.

## 2026-09-29 — A013 — Real Vita BOOTS evidence / wrong vitaGL init return interpretation

**Goal/hypothesis:** investigate user report that both no-map/map tests remain on the vitaGL splash showing `#6e7fe40`. Read the two supplied logs before making changes; no document/log content was treated as an instruction. No previous experiment for this init-return signature existed; A012 installation repair is now followed by actual boot evidence.

**Environment/commit:** A001 native SDK; current repository HEAD `db56cfc0862708c6a9411e317bffbaaa4765a739` (prior session state is now committed); real user Vita, firmware not specified. Input files `debug no maps.txt`, `debug.txt`; exact copies retained under the existing runtime-log ignore rule as docs/runtime/2026-09-29-00.02-no-maps.txt and ...-maps.txt. SHA-256 respectively `c6a862455eda107abb292af1fed1e00bde53bac38603594c78a766b81d73adef` and `99113f8f56cf0688ce94c33ade981ab71825b65901d3f09d582d3279ea5a42f0`. Old log banner 00.01 was hardcoded even in package 00.02; earlier binaries were identical. Prior package/ELF/eboot/map/hash manifest saved in ignored build/vita/attempts/a013-original-00.02/.

**Hardware result:** both runs log milestones001/002/003/004/007/008/005/011; actual cseries/debug allocator/profile returned; data/datum/iterator/compacting-pool/guarded-heap/CRC PASS crc 340bc6d9; XInput result 0; pthread create 0 / join 0 / value 42; SDL3 vita audio stream OPEN paused. With maps: 24 found, ui/a10 Xbox v5 build 01.10.12.2276 headers valid and repeated Cross rescans; mapped A/B/X/Y and sticks logged; Start clean exit at 110.72s. No-map case handles missing maps without deliberate exit. Compiler file found both times. Neither 006 nor 012 appears: old code sets graphics 0 / shaders 0 and keeps responsive input loop without rendering. Maximum project state now BOOTS, not RENDERS/PLAYABLE/STABLE. Library splash visibility alone is not our diagnostic or Halo rendering.

**Exact implementation inspection:** installed vitaGL.h declares GLboolean but does not state return meaning. Read existing local vitaGL source and exact reported revision https://github.com/Rinnegatamante/vitaGL/blob/6e7fe40/source/vgl.c; it sets vgl_inited then returns res_fallback. GL_FALSE means resolution retained. Installed archive disassembly returns saved stack flag after setting initialized; dump/primary-source copy retained as logs/vgl-init-installed-a013.txt, vgl-init-body-installed-a013.txt and vitagl-6e7fe40-vgl.c. No SDK/library changes. Our `if (!vglInitWithCustomSizes(...))` was incorrect; the logged failure was ours, not an allocation diagnostic.

**Changes:** vita_graphics_initialize logs resolution-fallback flag and checks GL version/positive viewport, guards duplicate init, logs post-init free memory. Budgets unchanged. main submits diagnostic frame (GLSL TEST PENDING) before GPU-copy/compiler experiments and logs each begin/return. Keep012's meaning as first diagnostic frame submission, now earlier than011. Version00.03 is shared between SFO and runtime banner. Verifier distinguishes current LINKS artifact from prior BOOTS project evidence. Add reproducible host regression tools/vita_init_regression.py compiling the actual init function with mocked SDK.

**Commands:** `python3 tools/vita_init_regression.py > build/vita/logs/graphics-init-regression-a013.txt`; `VITASDK=/usr/local/vitasdk-hardfp PATH=/usr/local/vitasdk-hardfp/bin:/home/vegettosandev/HaloCEVita/build/vita/tools/ninja/usr/bin:/usr/bin:/bin ninja vita_vpk > build/vita/logs/build-a013-init-return.txt 2>&1`; `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_verify.py`; `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_gl_audit.py`; `python3 -m py_compile tools/vita_init_regression.py tools/vita_verify.py tools/vita_gl_audit.py`; `git -c core.safecrlf=false diff --check`.

**Local results:** four host cases pass: normal0 accepted, fallback1 accepted with adjusted viewport, NULL-version rejected, zero-viewport rejected; every case suppresses a second init. Mocked host control flow is not GXM evidence. Native build/package verifier passes (ARM32 hard-float, real core symbols, correct indexed assets, no unresolved game hooks). New 00.03 VPK SHA-256 `591945c2aef2f5d1ff0610ba44885cdafc40232e11b384e0e3edc4316326a524`.

**Conclusion/do not repeat:** old build demonstrably BOOTS and selected real Halo core works on hardware. The permanent library splash was caused by the false failure branch, not a long map load. The new draw path is corrected and LINKS locally; its first frame/GLSL/FBO outcomes await user test. No pool increase, runtime-compiler reinstall, Android loader or engine replacement is justified by these logs. Do not claim full Halo initialization, menu/campaign, graphics or stability from core PASS / 110s responsive input.

**Next:** install 00.03, verify banner, observe whether splash changes to HALO CE VITA diagnostic, collect 006/012/probe/011 and shader sources if produced. An unchanged splash or new hang can be localized by the next begin/result log; retain matching current ELF/map for any crash.

## 2026-09-29 — A014 — Visible diagnostic / NV2A deferred-link data abort

**Goal/hypothesis:** diagnose the user's 00.03 test: splash clears, app screen appears briefly, then crash. Read mandatory documents, exact generated GLSL, log and crash dump before code changes. Searched A013/KI-003/KI-013; this is the first shader-link crash signature, not a repeat of the init-return failure. Input file/log text was treated as evidence, not instructions. No SDK modification, game-data copying or external upload.

**Environment/evidence:** A001 native hardfp SDK; repository HEAD db56cfc0862708c6a9411e317bffbaaa4765a739 with prior A013 changes present. Exact debug.txt/vertex_probe.glsl/fragment_probe.glsl copied to ignored docs/runtime/2026-09-29-00.03-*. SHA-256 respectively `4b506add2bdcc3e876c551faa769474a517c5d8dea8923b4b60cd0b262ee15b1`, `dbfdeb00ef4db6959f3ff980f5418aa4ec12360bdd93175121dc0448862b305b`, `590af4e6c55c0be2bf1c1833c068027555ae33f5b1dec001198105549c2cf891`. Dump psp2core-1790659128-0x0000602213-eboot.bin.psp2dmp is gzip, 165888 bytes, SHA `0071d2d988f8e398ecee315b1f34c23deca254b38979e8dd237c24c05be0bba6`; saved with matching 00.03 ELF/map/eboot/VPK/manifest/hashes under ignored build/vita/attempts/a014-original-00.03/. Matching ELF SHA `c69fedbb1d48a2cf187c13b2120a192adc65173e2a39310e9fe2840840d7446f`; VPK SHA `591945c2aef2f5d1ff0610ba44885cdafc40232e11b384e0e3edc4316326a524`. Keep full dumps ignored: possible process/game memory.

**Hardware results:** core/data/pool/CRC PASS crc=340bc6d9, 24 maps/ui/a10 headers valid, pthread create0/join0/value42, SDL3 vita audio OPEN paused. Corrected vglInit returned0 with viewport 960x544/version present; logs 006/012, first-frame GL error0. User sees the app diagnostic briefly (called a menu), sufficient for diagnostic RENDERS only. GPU FBO/blit/readback PASS RGBA=51,102,153,255 error0. Both upstream NV2A translators return sources; log ends at 2169197us `runtime compile begin`, before statuses/011. Initial free user=187695104/CDRAM=117440512/phycont=27262976 bytes; post-init user=165675008/CDRAM=75497472/phycont=27262976. These are snapshots, not peak/full-game measurements. No full Halo menu/tag loading/triangle/gameplay demonstrated.

**Dump diagnosis/inspection:** stdlib parser tools/vita_crash_audit.py uses the note layouts documented in primary https://github.com/xyzz/vita-parse-core; the old Python2 parser was inspected, not executed/installed. Main HCEV00001 stops with 0x30004 data abort. PC 0xe007eb04=SceGxm+0x135c4; r0=0. App code loaded at 0x81076000 versus matching ELF code0x81000000. LR 0x81083e7b -> ELF 0x8100de7a glLinkProgram, immediately after its sceGxmProgramGetParameterCount call. Stack candidate at 0x817fff04 value 0x810778e3 -> ELF 0x810018e2 vita_graphics_shader_probe, original vita_graphics.c:79 glLinkProgram call. This is not an unwound ARM backtrace. No app shader heap captured; an attempted direct shader-field read reports unmapped 0x81417a50, so individual compile results cannot be inferred from heap fields.

**Exact library behavior:** inspected installed archive/matching ELF disassembly and primary https://github.com/Rinnegatamante/vitaGL/blob/6e7fe40/source/custom_shaders.c plus glsl_utils.c. VGL_MODE_POSTPONED makes glCompileShader return without compilation and GL_COMPILE_STATUS report true. Link translates/compiles both then queries vertex GXM parameters without a post-compilation NULL check; matching code at 0x8100e15a stores compile result, failed return skips registration, eventually reloads vertex prog and calls the faulting parameter query. The signature is consistent with a rejected vertex leaving NULL; exact compiler diagnostic/construct remains unconfirmed. Original GLSL includes 9 varying declarations and unused helpers; those are candidates, not proven causes. No speculative translator rewrite or memory increase.

**Changes (D008):** APP_VER/banner 00.04. Use VGL_MODE_SHADER_PAIR and compile vertex then fragment, checking/logging each real status. Always finish the pair to reset semantics; never attach/link a failed shader. Guard shader/program allocation failure and release objects on all rejected paths. Install a VitaShaRK log callback after existing vitaGL compiler initialization to capture stage/line/level/messages even without HAVE_SHARK_LOG; do not initialize/terminate/replace allocators of a second compiler. Save retained rejected Cg via bounded 256KiB glGetShaderSource buffer and exact returned length. Keep original GLSL dumps and NV2A generators. Failure visibly reports GLSL BLOCKED SEE LOG and returns to the diagnostic/input loop. First-frame/init/GPU-copy behavior and budgets unchanged.

**Commands:** `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_crash_audit.py build/vita/attempts/a014-original-00.03/crash.psp2dmp --elf build/vita/attempts/a014-original-00.03/HaloCE.elf --output build/vita/logs/crash-a014-analysis.json > build/vita/logs/crash-a014-analysis.txt`; `arm-vita-eabi-objdump -d --disassemble=glLinkProgram .../HaloCE.elf > build/vita/logs/crash-a014-glLinkProgram.txt` and equivalent glGetShaderiv. `python3 tools/vita_shader_regression.py > build/vita/logs/graphics-shader-regression-a014.txt`; `python3 tools/vita_init_regression.py > build/vita/logs/graphics-init-regression-a014.txt`; `VITASDK=/usr/local/vitasdk-hardfp PATH=/usr/local/vitasdk-hardfp/bin:/home/vegettosandev/HaloCEVita/build/vita/tools/ninja/usr/bin:/usr/bin:/bin ninja vita_vpk > build/vita/logs/build-a014-shader-pair.txt 2>&1`; `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_verify.py`; `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_gl_audit.py`; `python3 -m py_compile tools/vita_crash_audit.py tools/vita_shader_regression.py tools/vita_init_regression.py tools/vita_verify.py tools/vita_gl_audit.py`; `git -c core.safecrlf=false diff --check`.

**Local results/failures retained:** first mock harness build failed -Werror=sign-compare in its GLuint-versus-int assertion, not application code; log preserved as graphics-shader-regression-a014-initial.txt. Explicit GLuint cast fixes the assertion, no warnings disabled. Nine actual-probe host cases pass: success, vertex/fragment/both compilation rejection, absent compiler, either shader allocation failure, program allocation failure and link rejection. Rejected compiles finish both stages, never attach/link, release objects and write failed source using exact length even without a NUL terminator. Four init regression cases still pass. Native executable/package verification passes; existing documented wchar/enum link warnings retained. 00.04 VPK 1319459 bytes SHA `a9f437f6ec114ab4c9203346eff82c584188b76127d725d3ce69b8ca15b3eb4b`; eboot SHA `021e6f57720ca7ba6e63a90305ab85796aa7e005fc4543c19efd0fcd2d4a7875`; ELF SHA `283f554c7a3a9989bf932b4488db8ee3c4f8462c156997eaa026fd9226233390`. These host/link checks are not console recovery or shader acceptance evidence.

**Conclusion/do not repeat:** KI-013 is fixed on hardware; diagnostic visible and bounded GPU-copy PASS. Current shader acceptance and recovery remain unverified, KI-014. Preserve matching archived ELF for the old dump. Do not repeat POSTPONED with identical sources, attach failed shaders, silently skip dynamic compilation, assume a map load caused this crash, or claim the full game menu/PLAYABLE/STABLE. Maximum project state RENDERS diagnostic only; current 00.04 artifact LINKS.

**Next:** install 00.04, verify banner, collect individual compiler diagnostics/status and any vertex_probe.cg/fragment_probe.cg. Expect milestone011 and responsive diagnostic/Start after compiler rejection, or a triangle after true compile/link success. If another stage crashes, retain the new log/dump with current 00.04 symbols. Adapt the exact unsupported construct only once the emitted compiler evidence identifies it; then continue upstream renderer integration.

## 2026-09-29 — A015 — Safe shader recovery / unsupported invariant qualifier

**Goal/hypothesis:** investigate 00.04 hardware feedback: opens normally, displays GLSL BLOCKED SEE LOG. Read mandatory docs and the supplied debug/GLSL/Cg as evidence before editing. Searched attempts/issues for invariant/type-specifier/line177; no prior correction existed. A014 changed compile observability/recovery, now exposing the actual rejection; retry is justified by that new evidence.

**Environment/evidence:** same existing native hardfp SDK, HEAD db56cfc0862708c6a9411e317bffbaaa4765a739 with earlier working changes preserved. Exact copies retained under ignored docs/runtime/2026-09-29-00.04-*. debug.txt9547 bytes SHA `a94fa1f412e8495aa4e27e1984af08863654288eac10c32dfe4335cdc9750d69`; vertex_probe.cg11605 bytes SHA `3ca438158ec270c85e5382f28fbce572372ec5d9ea49b2128a74b1bf6e8723ca`. Vertex/fragment GLSL hashes remain `dbfdeb00ef4db6959f3ff980f5418aa4ec12360bdd93175121dc0448862b305b` / `590af4e6c55c0be2bf1c1833c068027555ae33f5b1dec001198105549c2cf891`. Matching 00.04 ELF/eboot/map/VPK/manifest/hashes plus original nv2a_vsh.c saved in ignored build/vita/attempts/a015-original-00.04/. Prior VPK SHA `a9f437f6ec114ab4c9203346eff82c584188b76127d725d3ce69b8ca15b3eb4b`, ELF SHA `283f554c7a3a9989bf932b4488db8ee3c4f8462c156997eaa026fd9226233390`.

**Hardware result:** core CRC/data/pool PASS,24 maps/ui/a10 valid,pthread/value42 and paused audio open, first frame GL error0, GPU-copy PASS. Vertex compile callback at2345351us: `level=2 line=177: expected type specifier, but found 'identifier' instead`; vertex status0/error0, saved Cg11605 bytes. Pixel status1/error0. Attach/link correctly skipped; runtime compile returns0, milestone011 platform/core/maps/graphics1/shaders0 at2629807us, no full main010. Diagnostic opens normally; repeated controls/header checks continue, Start clean exit at21515103us. This demonstrates A014 recovery and original pixel compilation on hardware, not vertex/link/triangle or repeated-run STABLE.

**Exact diagnosis/inspection:** numbered supplied Cg line177 is `invariant gl_Position;`, directly after nine varying outputs. It was emitted by port/linux/src/nv2a_vsh.c's shader_prologue and forwarded unchanged by vitaGL's GLSL-to-Cg translator. The previous varying/helper suspicions are not needed to explain this first syntax rejection. Inspected full original generator, real xgpu_text builder/header, exact vitaGL6e7fe40 translator and GLSL1.20 section4.6. The invariant qualifier is a cross-program/multipass position-consistency guarantee; removing it does not prove that guarantee. Primary sources: https://github.com/Rinnegatamante/vitaGL/blob/6e7fe40/source/utils/glsl_utils.c and https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.1.20.pdf.

**Changes (D009):** move the invariant declaration into the existing non-HALO_VITA shader-prologue guard. Desktop/Android retain it. Add one Vita log stating the omitted qualifier and unverified multipass invariance. Preserve all NV2A instructions/helpers, position/viewport/Y/Z calculations, attributes/varyings and the successful pixel generator. Version/banner00.05. No SDK/library replacement, invented pragma, memory change or retail data. Safe eager compiler/recovery remains enabled.

**Commands:** root `VITASDK=/usr/local/vitasdk-hardfp PATH=/usr/local/vitasdk-hardfp/bin:/home/vegettosandev/HaloCEVita/build/vita/tools/ninja/usr/bin:/usr/bin:/bin ninja vita_vpk > build/vita/logs/build-a015-invariant.txt 2>&1`; `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_verify.py`; `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_gl_audit.py`; `python3 -m py_compile tools/vita_verify.py tools/vita_gl_audit.py`; `git -c core.safecrlf=false diff --check`.

**Host source comparison:** `python3 -` generates standalone C snapshots combining the actual archived/current nv2a_vsh.c and actual xgpu_text.c, uint32_t DWORD, actual xgpu_text declaration/constants, a silent log stub and the actual two-MOV probe function. Host `cc -std=c11 -Wall -Wextra -Werror` builds before/after snapshots with `-DHALO_VITA`, no target define (desktop), and `-DHALO_ANDROID`. Each emits114 cases: exact two-MOV probe, all14 MAC x8 ILU combinations, and a packed-input case. Saved files under build/vita/tests/vertex-a015/{vita,desktop,android}-{before,after}.c and .glsl.txt. Reproduce each captured snapshot with the same flags, run to a file, and compare: Vita equals old output with only `invariant gl_Position;` lines removed; desktop/Android are byte-identical. Result summary logs/vertex-regression-a015.txt. Exact old host probe matches the hardware-provided GLSL. These checks exercise actual generation/string building, not the Vita compiler or native game ABI.

**Results:** all342 host case comparisons pass, no arithmetic/operation/varying differences. Native shared generator compiles, selected real core links, SELF/VPK/ABI/assets/banner checks pass. Existing documented wchar/enum link warnings retained. VPK00.05 is1319659 bytes SHA `0ed8fe2f3f1d855b8f64691b6a9e6e598cb244c1017bbb0d77731055f69328d4`; eboot1324625 bytes SHA `266fc4053943b6487289ae0ee7fc31fe19255bada14b1d23afc989e51657da74`; ELF SHA `0fae11921b120f148c111a8a761aaf471e1628cfdaf8e2bc6fd3c5024ac86d75`. Current artifact LINKS; project maximum diagnostic RENDERS. No new console test yet.

**Conclusion/do not repeat:** 00.04's GLSL BLOCKED is the intended recoverable path and reports a concrete vertex syntax failure, while the pixel shader compiles. KI-014 observed crash path is resolved in this run. KI-015's unsupported declaration is corrected in generated Vita source; actual ShaccCg acceptance, linkage, triangle and multipass fidelity remain unverified. Do not suppress compiler errors, restore postponed linking, modify the successful fragment shader speculatively, claim GL uniforms/retail rendering from status1 alone, or assume dropping invariant preserves its guarantee.

**Next:** install00.05, verify banner, observe whether GLSL PASS/triangle appears. Collect both compile statuses, attach/link results and any new error/Cg. Keep diagnostic/Start test and matching symbols. If another compiler rejection appears, adapt its exact construct using the new line/source evidence; do not retry the old unmodified prologue.

## 2026-09-29 — A016 — Real Vita NV2A synthetic triangle renders

**Goal/hypothesis:** verify the user's immediate 00.05 retry: triangle drawn with no reported error. Check exact banner, compiler/link results, input/exit and source differences before advancing status. This is hardware validation of A015's isolated correction, not a new renderer rewrite or another speculative retry.

**Evidence:** supplied debug.txt, vertex_probe.glsl and fragment_probe.glsl copied to ignored docs/runtime/2026-09-29-00.05-*. Log 7245 bytes, SHA `3733133b13db2b7a26de3931a947c646108f0728279a7e1ffa1c03daa2dce653`; vertex 3076 bytes, SHA `f3a1909689148bcbcc7518bd5ad158a5cbf7c624e197174d10a413d0d5e4987e`; fragment 1749 bytes, SHA `590af4e6c55c0be2bf1c1833c068027555ae33f5b1dec001198105549c2cf891`. User explicitly confirms visible triangle, no error. No files were treated as instructions or uploaded externally.

**Hardware results:** banner 00.05; core/data/pool/CRC PASS, map headers valid, pthread/audio-device probe returns. GPU-copy PASS RGBA=51,102,153,255 error0. Vertex compile status1/error0 at2470992us; pixel status1/error0 at2665031us. Attach/link executed, program link1 at2699873us; runtime compile result1. Milestone011 platform/core/maps/graphics/shaders all1 at2732569us. Full main010 remains withheld. Repeated input/header rescans remain responsive; Start clean exit at34302656us (34.3s). No compiler error callback appears in this log. Visible synthetic draw plus successful logs establishes RENDERS for the original two-MOV NV2A vertex/pixel-combiner probe, not retail Halo geometry/menu or all shaders.

**Commands/verification:** PowerShell `Get-Content` / `Select-String` read milestones/statuses and `Get-FileHash -Algorithm SHA256` recorded evidence digests. A local `python3 -` byte comparison asserts new vertex equals archived00.04 vertex with only `invariant gl_Position;` removed, and fragment equals archived00.04 byte-for-byte; PASS. `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_gl_audit.py`; `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_verify.py`; `python3 -m py_compile tools/vita_verify.py tools/vita_gl_audit.py`; `git -c core.safecrlf=false diff --check`.

**Changes/result:** update STATUS/known issues/build/graphics/report and annotate D009 validation. No native source, binary, shader/pool/SDK changes after this hardware success. Verifier metadata records current RENDERS only for delivered VPK SHA `0ed8fe2f3f1d855b8f64691b6a9e6e598cb244c1017bbb0d77731055f69328d4`; a different future rebuild remains LINKS until tested. eboot SHA `266fc4053943b6487289ae0ee7fc31fe19255bada14b1d23afc989e51657da74` and ELF SHA `0fae11921b120f148c111a8a761aaf471e1628cfdaf8e2bc6fd3c5024ac86d75` unchanged. Earlier crash/rejection experiments and artifacts remain intact.

**Conclusion/limits:** KI-015's synthetic vertex syntax blocker is resolved on real Vita. The path original NV2A translation -> vitaGL GLSL/Cg compilation -> GXM -> visible synthetic triangle works for this pair. Multipass invariance, retail shader/texture/state coverage, placed/rebased Xbox memory, full XAPI and menu/tag loading remain pending. One successful 34.3s run is neither PLAYABLE nor repeated-test STABLE, and no gameplay FPS claim is made.

**Next:** preserve the working probe and matching symbols as the graphics baseline. Continue full engine integration through the explicit memory/address and XAPI contracts, then real D3D8 state/geometry/textures/map tags; expand shader coverage and measure multipass agreement before claiming Halo menu/campaign fidelity.

## 2026-09-29 — A017 — Native engine-memory and real-map directory integration

**Request/hypothesis:** continue integration after the hardware triangle succeeds. Preserve00.05 as graphics baseline, adapt the original Xbox memory contract, and exercise real map directories through original engine APIs before activating nested tags/BSP/resources or full main. This is an integration checkpoint, not completed menu/gameplay.

**Inspection:** re-read mandatory project docs and exact physical_memory_map.c, game_state_xbox.c, cache_files.c/windows/decompress_windows.c, tag_groups.h, scenario definitions, Linux allocator/resource-address/platform/main paths and installed SDK sysmem headers. Linux assumes a fixed128MiB VA window; the tag range overlaps the observed Vita executable load segment. Public kernel allocation options offer alignment rather than that fixed virtual mapping. Reclaimer/Supyr/Binilla sources were fetched under ignored build/vita/tools only to inspect format descriptions; no definitions/library/schema is imported into the product or used by the tests. Full platform compile inventory12/15 passes; retained errors identify sys/mman.h, O_CLOEXEC and x87 fenv fields, not fabricated platform stubs.

**Changes:**00.06 reserves one96MiB USER_RW kernel arena,64KiB aligned, outside newlib64MiB. Game16MiB/tag22MiB/texture22MiB/sound4MiB retain original offsets using a placed/top-down allocator. It calls real physical_memory_allocate/verify and game_state_allocate_buffer/free_buffer; RW queries track allocation ownership. Unsupported protection fails/logs explicitly; no Linux SIGSEGV write tracking is claimed. Guarded shared edits translate physical-memory assertions/state bookkeeping/protection addresses and resource offset conversions only underHALO_VITA. A temporary validated directory/name/root mount exercises actual tag_iterator/tag_get/group checks, validates scenario type and sky/BSP-reference ranges, and always detaches. Nested tag/BSP/resource pointers are not activated;010 remains withheld.

**Map reader:** portable native-ABI C reader, read-only file access, direct seek for decompressed maps or zlib stream for compressed Xbox-v5 maps. Keeps only tag section in existing22MiB cache plus32KiB input/32KiB output and zlib state; no full-map allocation or decompressed-map copy. Checks logical512MiB cap, section capacity/bounds, compressed checksum/length, tag header/directory/name/root/datum/scenario and GPU-directory bounds. CRC32 recorded is a tag-section diagnostic, not a claim of retail cache checksum authentication. Progress logs/diagnostic frames continue, Start cancels and cleans up, Cross re-runs after header scan. Missing files/kernel reserve failures remain observable.

**Commands:**

```bash
cp -p build/vita/{HaloCE.vpk,eboot.bin,HaloCE.elf,HaloCE.elf.map,artifacts.json,SHA256SUMS} build/vita/attempts/a017-baseline-00.05/
VITASDK=/usr/local/vitasdk-hardfp PATH=/usr/local/vitasdk-hardfp/bin:/usr/bin:/bin python3 tools/vita_compile_audit.py --platform --jobs 6 --output build/vita/audit-platform-a017 --only [15 inspected platform units]
python3 tools/vita_memory_regression.py
VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_cache_regression.py /mnt/f/UtilidadesHaloCe/xemu-win-x86_64-release/Halo/Data/maps/ui.map /mnt/f/UtilidadesHaloCe/xemu-win-x86_64-release/Halo/Data/maps/a10.map
VITASDK=/usr/local/vitasdk-hardfp PATH=/usr/local/vitasdk-hardfp/bin:$PWD/build/vita/tools/ninja/usr/bin:/usr/bin:/bin ninja vita_vpk
python3 -m py_compile tools/vita_memory_regression.py tools/vita_cache_regression.py tools/vita_verify.py
git -c core.safecrlf=false diff --check
```

The exact15-unit compile commands are retained in build/vita/audit-platform-a017/**/*.log; command shorthand above is not a substitute for those records. Native log: build-a017-delivery.txt; reader/memory logs: cache-regression-a017.txt / memory-regression-a017.txt.

**Failures retained/corrections:** exploratory Python venv creation failed (ensurepip/pip absent); no SDK/system installation was performed, source inspection used ignored checkouts instead. Initial host reader compile lacked zlib development headers; portable SDK zlib headers and the existing host libz.so.1 were used (SDK ARM archive never linked into a host test). An initial host test incorrectly made XDK ULONG_PTR64, so its -1 offset did not equal the32-bit ANY sentinel and texture allocation asserted; mock corrected to uint32_t, failed log retained as memory-regression-a017-host-offset-failure.txt. Host review found/fixed free-page query0 versus PAGE_NOACCESS and absolute alignment requirements; allocation/protection range checks cover those cases. Native build first/final logs retained.

**Host results:** actual C reader25 synthetic cases PASS: compressed/plain exact output and CRC, cancellation, capacity, invalid version/offset/size/footer/tag signature/table/count/datum/name/root/scenario/resource counts, truncation/corrupt zlib, unaligned directory/root rejection. Native scenario size/skies/BSP offsets are statically checked before using its original struct. Actual user-owned maps read in place: ui compressed1, logical33582080, tags1642244,983 tags,34 vertex/index buffers, tagCRC e22586e4; a10 compressed1, logical274734080, tags14158000,3357 tags,623 vertex/index buffers, tagCRC1c90dc0f. No retail tag bytes are exported to build files or logs. Actual allocator and original physical/state functions pass two mocked-SDK lifecycle cycles, overlap, absolute alignment, overflow/bounds, zeroing/reuse and unsupported/RW-protection checks. Host tests do not execute ARM or real kernel/GXM.

**Native result:**00.06 COMPILES/LINKS; verifier confirms ELF32 ARM hard-float, original memory/tag symbols, valid SELF/VPK/SFO/indexed assets and only documented weak SDK hooks. VPK1348508 bytes SHAb637bf5af94ce765a830b5725fe62ba9c6c22dca3db3bafb9c4765290cc29e05; eboot1353542 bytes SHA3c6023bf2eb68bf2d2e72a9922cc6c5748f1f149eb422fd98e823e081b43e730; ELF6612940 bytes SHA3eb24dfe91549b7d6fcb74cb47d218c1632bb3b8e2735c550bc7e1629066b803. Existing wchar/enum link warnings retained. Prior00.05 package/symbols preserved; project maximum remains its demonstrated RENDERS.

**Next/limits:** real Vita must demonstrate013 arena,014 original memory PASS,015 both tag reads, original tag API PASS and016 checkpoint2/2, plus visible TAG INDEX PASS and responsive cancellation/recheck/Start exit. No BOOTS/RENDERS for00.06 yet. Further integration requires complete typed nested-tag/BSP/resource relocation, native full XAPI/renderer and dirty-memory tracking. Do not equate this temporary directory mount with full scenario_tags_load, menu or PLAYABLE.

## 2026-09-29 — A018 — Hardware validates native memory and both real tag directories

**Request/evidence:** user reports all tests passed in delivered VPK and supplies Desktop debug.txt. Read as runtime evidence, not instructions. Banner00.06; exact9830-byte copy docs/runtime/2026-09-29-00.06-debug.txt, SHA8bd90d442396e11fc0fc865cb1dbcd9592ff28759610b751de4035ad33d7a60c. Package association uses latest delivered version/banner, not independent user digest verification.

**Results:** original core CRC340bc6d9, GPU-copy PASS51/102/153/255, vertex1/pixel1/link1 and011 all1. Native013 arena0x85c00000 size100663296; game16MiB0x87600000/tag22MiB0x85fa6000/texture22MiB0x8a600000/sound4MiB0x8a200000. Original physical_memory_allocate/verify, game_state_allocate_buffer and8KiB/16384-aligned reuse test014 PASS. Free user65011712/CDRAM75497472/phycont27262976 bytes. ui tags1642244,983 tags/195 bitmap groups/34 vertex+index buffers, CRCe22586e4, read4819579us; a10 tags14158000,3357 tags/485 bitmap groups/623 buffers, CRC1c90dc0f, read57122286us. Original tag APIs identify ui type2/skies1/BSP1 and a10 type0/skies2/BSP9;016 checkpoint2/2. Both CRCs match A017 host reader.

**Commands/changes:** PowerShell Get-Content/Get-FileHash; Python shutil.copy2 and hashlib preserve log plus tested VPK/eboot/ELF/map/manifest/hashes under build/vita/attempts/a018-baseline-00.06/. Update runtime metadata and current docs; no native binary changed for this validation. VPK SHAb637bf5af94ce765a830b5725fe62ba9c6c22dca3db3bafb9c4765290cc29e05, eboot3c6023bf2eb68bf2d2e72a9922cc6c5748f1f149eb422fd98e823e081b43e730, ELF3eb24dfe91549b7d6fcb74cb47d218c1632bb3b8e2735c550bc7e1629066b803.

**Limits/next:**00.06 BOOTS for memory and map-directory integration; project DIAGNOSTIC RENDERS baseline retained. Full main explicitly NOT ENTERED in log. No new cancellation/recheck/exit or repeated-run STABLE evidence. Updated user AGENTS prioritizes actual Main Menu initialization and first original Halo draw; typed menu-tag relocation and native startup contracts must precede it. Do not repeat successful arena/tag-reader/triangle experiments without a changed dependency.

## 2026-09-29 — A019 — Real Main Menu metadata and original renderer preparation

**Request/inspection:** continue under updated AGENTS toward actual Main Menu/original draw. Read renderer plan/matrix and current local tree/hardware first. Local HEAD advanced externally to b2f02fa; preserved newer user work. Inspected original main_load_ui_scenario/main_menu_load/scenario_load/scenario_tags_load, widget creation/rendering and tag/bitmap/font/text accessors, plus original d3d8_gl program/constant code. A018 arena/tag/triangle successes are baseline, not experiments to repeat blindly.

**Changes/D011:** typed validate-then-commit journal for known menu layouts; count/alignment/cache-bound/reference datum/group/name/UTF16/font-index checks; reject conflicting pointer/scalar roles and child cycles/depth. No arbitrary word scan or unknown GPU/BSP rewrite. Bounded journal/hash1.5MiB plus at most32KiB graph state within existing heap; restore known fields/directories and compare source CRC. Move unchanged private DeLa structs/asserts to shared header, no UI behavior replacement. Link original tag_groups/bitmap_group/font_group/text_group/msvc_wide units and actual accessors. Native bridge treats implicit declarations/pointer conversions/return types as errors.00.07 reads ui only; completed017 rebase/018 actual menu tag/019 original accessors/020 restored CRC/0161-of-1. Accessor begin logs retain the exact tag datum/component if the native call crashes; bitmap sprite NONE follows the original frame0 fallback. Full UI/main/menu state and draw not entered.

**Renderer preparation:** only HALO_VITA path of real d3d8_gl: bind16 vN_in attributes before program link; resolve192 c[i] locations independently, preserve holes/nonconsecutive locations and upload changed active registers with count1.768B per cached program. Desktop/Android unchanged. This is COMPILES/CPU-tested code, not a completed/activated backend or shader-uniform compaction. No fake state/shader handles.

**Commands:** python3 tools/vita_menu_regression.py /mnt/f/UtilidadesHaloCe/xemu-win-x86_64-release/Halo/Data/maps/ui.map /mnt/f/UtilidadesHaloCe/xemu-win-x86_64-release/Halo/Data/maps/a10.map; python3 tools/vita_program_regression.py; VITASDK=/usr/local/vitasdk-hardfp PATH=/usr/local/vitasdk-hardfp/bin:/usr/bin:/bin python3 tools/vita_compile_audit.py --only source/interface/ui_widget.c --output build/vita/audit-menu-a019; same environment --platform --only port/linux/src/d3d8_gl.c --output build/vita/audit-renderer-a019; native SDK/local Ninja PATH ninja vita_vpk; VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_verify.py and tools/vita_gl_audit.py; python3 -m py_compile new tests/audit/verifier; git -c core.safecrlf=false diff --check.

**Failures/corrections preserved:** initial host -Werror=misleading-indentation fixed without suppressing warnings. Real cached references retain a valid name but length0; initial terminator-at-length check rejected ui offset0x37fd0/a10 offset0x4960a8. Retry justified by actual metadata: now bounded actual NUL validated while preserving length. menu-a019-first.txt retains incorrect synthetic overflow fixture (base still fitted); corrected to0xfffffffc. menu-a019-second.txt retains a10 missing-menu rejection, now expected no-write outcome. Build-first/second retain missing math/cache declarations and inadvertently extracted unrelated HUD views; exact DeLa views and includes corrected. Build-third missing msvc_towlower fixed by original msvc_wide.c. Initial compile audit missing compiler PATH retained as widget-compile-a019.txt; corrected environment passes. Documentation script syntax failure occurred before any edit and was corrected. New-line-only diff noise fixed while preserving original unchanged source. Existing unused editor postprocess void*/bitmap_data* callback warning and wchar/enum link warnings retained, not hidden; no wrong float/variadic prototype accepted at new boundary.

**Results:**17 synthetic cases PASS: rollback byte identity, late-corruption no-write rejection, count/definition/bounds/alignment/overflow, reference salt/group/name, UTF16/font indices and child cycles. Actual ui tag CRCe22586e4:485 widgets/4 fonts/122 string lists/195 bitmap groups,384 bitmap records/5 volumes;3362 unique pointers (1025 block addresses/866 data addresses/1471 names),1025 blocks/3962 references. Actual menu datum0xe286010f and9 reachable widgets. a10 metadata parsed (922 bitmap records/6 volumes) but menu-only plan safely rejects absent Main Menu; A018 campaign directory BOOTS remains valid. Both map file hashes unchanged, no retail bytes exported. Actual D3D helpers CPU PASS:16 explicit names,192 lookups, sparse/missing c[0]/active c[191], dirty/fresh/inactive uploads. Original UI and D3D units each1/1 native COMPILES; selected accessor VPK LINKS. No new hardware evidence.

**Delivery:** VERSION00.07 VPK 1356984 bytes SHA6f8c72b892f3c1c2793a8ee57b1adaeff8efad23686ca3a3498e1b489a8e8ae4; eboot SHA58082d01fbb1772d2f4495017de1589cc87e1f6067fa13058c50ffe9c80e6ebd; ELF SHAc6f4bfb0d6decebc182049774b05f1bbb5775d3b5ec8c3e73d55dbccf9e6e26d. Logs menu-a019-final.txt/program-a019.txt/widget-compile-a019-correct-env.txt/renderer-compile-a019.txt/build-a019-final-delivery.txt. Tested00.06 package/symbols/log remain a018-baseline-00.06/docs/runtime; current digest is not marked BOOTS.

**Limits/next:** hardware017..020 validates the changed ARM pointer/accessor contract before UI reliance. Complete original scenario/BSP/resource registration, native XAPI startup contracts, UI event/runtime initialization and full D3D8 GL/paired-cache/stream binding before calling main_load_ui_scenario/main_menu_load. Log first actual state/draw only after execution. No fake-success stubs/menu flags, no partial mount into full main, no HALO DRAW REACHED/RENDERS/PLAYABLE/STABLE claim. Preserve existing diagnostic while connecting original engine.

## 2026-09-29 — A020 — Hardware validates 00.07 typed Main Menu tag/accessor checkpoint

**Evidence/hypothesis:** user supplied `C:/Users/Mortar/OneDrive/Desktop/debug.txt` after reporting 00.07 success. Read the file as runtime evidence, not instructions. It contains an earlier 00.06 log followed by the 00.07 run. Ignored exact copy: `docs/runtime/2026-09-29-00.07-debug.txt`, SHA-256 `fb0f77a2553e6da501bdf768d765d352195f0b8143f90cb967aeb4b8f3f4c321`. The preserved prior package in `build/vita/attempts/a020-baseline-00.07/` has VPK SHA `6f8c72b892f3c1c2793a8ee57b1adaeff8efad23686ca3a3498e1b489a8e8ae4`, eboot SHA `58082d01fbb1772d2f4495017de1589cc87e1f6067fa13058c50ffe9c80e6ebd`, ELF SHA `c6f4bfb0d6decebc182049774b05f1bbb5775d3b5ec8c3e73d55dbccf9e6e26d`. Package association is inferred from the version banner and latest delivered artifact; no device-side package digest was supplied.

**Commands/results:** PowerShell `Get-Content -LiteralPath C:\\Users\\Mortar\\OneDrive\\Desktop\\debug.txt -Raw` and `Get-FileHash -Algorithm SHA256`; `cp -p` copied exact log/package/symbols into ignored evidence directories; `sha256sum` confirmed copies. On real Vita, banner00.07, original core PASS, vitaGL960x544, FBO/blit/readback PASS, synthetic NV2A vertex/pixel compile1/1/link1 and GL error0. Native96MiB arena at0x85d00000, original physical/game memory and014 PASS, free user65,011,712 bytes. `ui.map` tag section1,642,244 bytes/983 tags/34 vertex+index buffers/CRCe22586e4 loaded in4.982s. 017:3362 typed pointers (1025 block,866 data,1471 reference names),485 widgets/4 fonts/122 string lists/195 bitmap groups. 018: original Main Menu datum `e285010f`, type0,3 children,9 reachable widgets. 019: original bitmap/Unicode/font accessors PASS (195/122/4). 020: original tag image CRC restored. 016=1/1, Start clean exit at32.869s. No original widget runtime, bitmap pixels/GPU upload, retail shader/draw or full main in this run.

**Conclusion:** 00.07 is BOOTS for typed UI metadata/accessors and transactional rollback. The next blocker is runtime initialization/root activation and the original renderer, not tag discovery. Do not repeat a10 decompression on the UI fast path or treat synthetic triangle as a Halo draw.

## 2026-09-29 — A021 — Persistent UI mount and first original widget-runtime initialization

**Hypothesis/inspection:** the 00.07 checkpoint detached the validated UI image before original widget consumers could use it. Inspected current HEAD `f5ad763512aaa5689856af006c9801b355e4fb12` and clean semantic working tree (an unrelated line-ending-only `ui_widget_tags.h` edit appeared during inspection; its bytes were normalized without changing content), mandatory status/renderer docs, original `main_loop` -> `main_menu_load` -> `main_load_ui_scenario` -> `main_new_map` -> `game_load` -> `scenario_load` -> `scenario_tags_load`, `ui_widgets_initialize`, root creation, UI update/render and the screen-geometry D3D8 immediate draw. Recorded the exact source trace in `docs/MAIN_MENU_CALL_GRAPH.md`. No extra relocation rule was justified by this path yet.

**Changes/D012:** split the existing cache checkpoint into `halo_vita_cache_mount_menu`, `halo_vita_cache_validate_menu`, and `halo_vita_cache_unmount_menu`. Keep the typed relocation journal and an exact per-entry directory-pointer journal alive until unmount. Recheck restores before overwriting the cache; failed validation/cancellation and process exit unmount. Unmount compares restored original CRC. The UI fast path still reads only ui. Link the original `ui_widgets_initialize` and its `stack_memory_pool_reset` dependency, with a HALO_VITA read-only initialized/pool query. It does not create a widget, set main-menu flags or draw. Accessor begin logging is limited to the first bitmap/string/font; final category counts remain. APP_VER 00.08; prior 00.07 VPK/ELF/map/manifest preserved.

**Experiments/failures:** first build passed native link but verifier required the no-longer-reachable transient `halo_vita_cache_index_probe`; updated required symbols to the three live mount APIs. Initial `ui_widgets_initialize` link then failed on exact missing original `stack_memory_pool_reset`; added its upstream source, not a stub. An ignored dry link forcing `ui_widget_load_by_name_or_tag` failed with17 references across11 distinct functions: game-time disposal/initialization/start/pause, script evaluation, update-server/new-map, player-control inhibition, sound pause/impulse and original event-handler invocation. Command `python3 build/vita/link_probe.py` from `build/vita` generated `build/vita/logs/menu-root-link-probe.txt`. Host metadata inspection (`python3 build/vita/menu_inspect.py`) of user-owned ui.map showed the root's `_widget_event_created`=24 handler function23=`main_menu_initialize`, and a game-demos child creation handler function86. The original function23 enters player UI/network/audio state immediately. This was a link/source/host-data experiment only; root activation was not executed or shipped as fake success. The new partial mount cannot run full scenario/BSP/resource path yet.

**Verification commands:** `VITASDK=/usr/local/vitasdk-hardfp PATH=/usr/local/vitasdk-hardfp/bin:$PWD/build/vita/tools/ninja/usr/bin:/usr/bin:/bin ninja vita_vpk` (log `build/vita/logs/build-a021-final.txt`); `python3 tools/vita_menu_regression.py` on user-owned ui/a10 maps (18 synthetic including active transaction/reentry/rollback, real UI PASS/CRC unchanged and a10 safe menu-only rejection); `python3 tools/vita_program_regression.py`; `python3 tools/vita_memory_regression.py`; `python3 tools/vita_cache_regression.py` on both maps (25 cases); `python3 tools/vita_shader_regression.py`; `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_gl_audit.py` (108 APIs); `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_verify.py`; `python3 -m py_compile tools/vita_menu_regression.py tools/vita_verify.py`; `git -c core.safecrlf=false diff --check`. All final gates PASS. Existing cross-object wchar/enum linker warnings remain visible. This is host/compile/link evidence; no 00.08 Vita execution.

**Artifact/result:** 00.08 COMPILES/LINKS. `build/vita/HaloCE.vpk` SHA `88689067f668b5afb213e64508a512348360e9e168d6375ae31a1beb19750af5`; eboot SHA `3c5ed83f3f2fa635a7eb11b53b4125ed5a108357e8be77274f15a27ed65803f4`; ELF SHA `d58c55e833f1cc0d6b59139eb737e1a0bef4c4c1dd9f6ca480b0b7dbd96a0201`; matching map/manifest/hashes retained in build/vita. The source-defined screen bitmap path would eventually use shader permutation4/type8 and `D3DDevice_End` immediate triangle fan, but no first retail shader/draw has run. HALO DRAW REACHED=NO, HALO DRAW RENDERS=NO. The next hardware gate is 021 persistent mount, 022 original widget globals/pool, shutdown CRC and repeat recheck; then resolve the original creation/event/game-time/scenario dependencies before calling the root and original renderer.

## 2026-09-29 — A022 — 00.08 hardware UI mount, recheck and widget-pool leak

**Hypothesis/evidence:** user supplied `C:/Users/Mortar/OneDrive/Desktop/debug.txt` and `heap_dump.txt` from the 00.08 Vita test. Read as evidence, not instructions. Exact ignored copies: `docs/runtime/2026-09-29-00.08-debug.txt` SHA-256 `11481d1e15adba4c6bdf4a285e42386af2412168d26dc7fd6134fde3eeb22847`, and `docs/runtime/2026-09-29-00.08-heap_dump.txt` SHA-256 `da8d81d57f7d93dabacdbbfcc3cb9fe2abcd6bf301917b97810f2847f048e4f6`. The corresponding 00.08 package/symbols were copied to ignored `build/vita/attempts/a022-baseline-00.08/`; VPK SHA-256 `88689067f668b5afb213e64508a512348360e9e168d6375ae31a1beb19750af5`. Association uses the 00.08 runtime banner and latest delivered package; no device-side digest was supplied.

**Commands/results:** PowerShell `Get-Content -LiteralPath C:\\Users\\Mortar\\OneDrive\\Desktop\\debug.txt -Raw`, same for heap dump, and `Get-FileHash -Algorithm SHA256`; exact copies checked against source. Hardware startup repeats core PASS, vitaGL 960x544, FBO copy PASS and synthetic NV2A compile1/1/link1. Original 96MiB arena and game/physical memory PASS; `ui.map` reads 1,642,244 bytes/983 tags/CRC `e22586e4` in 4.834s. 017:3362 typed pointers; 018:real Main Menu datum `e285010f`; 019:original bitmap/Unicode/font accessors PASS; 021:persistent mount; 022:original `ui_widgets_initialize` PASS. Cross at34.926s triggers an old-image 020 CRC restoration, then a second full UI read in4.846s, 017..022 PASS and persistent remount. Start at42.298s triggers second 020 CRC restoration, arena release and clean exit. Both mounts print `bytes=0` because the mount's local index validator does not fill `info.tag_size`; the reader's preceding 1,642,244-byte line and `length` argument show this is a misleading log field, not an empty cache. No widget root/events, retail shader/draw or full main ran.

**Leak signature:** `heap_dump.txt` contains one allocation, source `ui_widget.c:117`, id3, 16,384 bytes; the original `ui_widgets_initialize` allocates exactly `WIDGET_MEMORY_POOL_SIZE` there. 00.08 never calls UI disposal before the debug allocator reports at shutdown. This is a specific lifecycle gap, not evidence of unbounded repeated growth: `halo_vita_ui_runtime_initialize` skips reallocation while globals remain initialized. 00.09 must test that 023 appears before unmount on Cross/Start and that the `ui_widget.c:117` entry is absent from the device heap dump.

## 2026-09-29 — A023 — Checked disposal of the original widget pool before cache unmount

**Hypothesis/inspection:** A022 proves persistent mount and UI globals work but shows the 16 KiB pool left allocated. Inspected original `ui_widgets_initialize`/`ui_widgets_dispose` in `source/interface/ui_widget.c`: initialize calls `match_malloc` at line117; dispose frees the pool only after `ui_widgets_close_all`. No widget instance is created by the current Vita checkpoint. Also inspected the 00.08 `bytes=0` log: `vita_cache_validate_index` does not populate `info.tag_size`, while the caller knows the validated image `length`.

**Changes/D013:** add a HALO_VITA-only checkpoint disposal beside the original widget globals. It refuses any active widget, then uses the original `pool_free`, clears pool base/size and zeros globals, matching the no-widget tail of upstream disposal. Call it before unmount/replacement on Cross and before unmount/arena release on Start; log 023 only after successful free. Fix mount log to print validated `length`, and 016 wording to distinguish initialized widget globals from unentered root/events. APP_VER 00.09; 00.08 VPK/ELF/map retained. No gameplay state or renderer shortcut was added.

**Failed experiment:** directly calling original `ui_widgets_dispose` compiled but failed native link: its `ui_widgets_close_all` pulls active-widget deletion and 17 game-time/audio/player-control/event/script dependencies (same root closure identified in A021). The full function cannot be used in this partial checkpoint without implementing those original contracts. The checked no-widget path is confined to the current stage and will reject use after root activation.

**Commands/results:** `VITASDK=/usr/local/vitasdk-hardfp PATH=/usr/local/vitasdk-hardfp/bin:$PWD/build/vita/tools/ninja/usr/bin:/usr/bin:/bin ninja vita_vpk` passes ARM32 ELF/SELF/VPK link and package verification; `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_verify.py` PASS; `python3 tools/vita_menu_regression.py` 18 synthetic cases PASS; `python3 -m py_compile tools/vita_verify.py` PASS; `git -c core.safecrlf=false diff --check` PASS. Existing 2-byte-wchar/enum cross-object linker warnings persist. Final 00.09 VPK SHA-256 `cd364a2c7d692971816d9b93d92fef07fe3e7f39aa2fc6f42c183785aa88ab8f`, eboot `9f217a99802ec483a0ee96ff26ce143d39bd1ee20d54c3d728d618541d0335b0`, ELF `66a9d3565ee264b8aad68217d98b488e210718210cd0dcb9c2f1d984d8720851`. 00.09 is COMPILES/LINKS only; 023 and the absence of the 16 KiB UI entry in a device heap dump need a Vita run. HALO DRAW REACHED=NO; HALO DRAW RENDERS=NO.
