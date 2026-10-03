Warning: truncated output (original token count: 67082)
Total output lines: 1342

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

**Exact library behavior:** inspected installed archive/matching ELF disassembly and primary https://github.com/Rinnegatamante/vitaGL/blob/6e7fe40/source/custom_shaders.c plus glsl_utils.c. VGL_MODE_POSTPONED makes glCompileShader return without compilation and GL_COMPILE_STATUS report true. Link translates/compiles both then queries vertex GXM parameters without a post-compilation NULL check; matching code at 0x8100e15a stores compile result, failed return skips registration, eventually reloads vertex prog and calls the faulting parameter query. The signature is consistent with a rejected vertex leaving NULL; exact compiler diagnostic/construct remains unconfirmed. Original GLSL includes 9 varying dec…51082 tokens truncated…01 — A097 — Resume interrupted shell integration, fix native declarations

Remote main f6debea contains A094–A096; local pending keyboard/frame changes were preserved while synchronizing the local branch. Vita Build170/run36938017740 failed with static declaration follows non-static declaration for game_options_menu_update_text_desc/pic_desc, introduced by their earlier selective calls. Add exact static forward declarations. No SDK available locally; required native CI will validate the next closure. No all-window/hardware claim.


## 2026-10-01 — A098 — Original keyboard and per-frame shell completion

Initialize/dispose the original event manager and keyboard beside saved-game owners. Add typed vcky/key reference relocation validated on the provided983-tag ui.map. Route Cross/Circle/Square/Start/L/R and four D-pad directions to the original queue while keyboard is active; regular shell input retains the tested original focus/dispatch bridge. Start no longer exits staged bring-up, so keyboard Done works; Select exits the test app. Execute original null-event recursive updates for animation, generated lists, authored keyboard completion predicates and auto-close; restore original history after root closes. World/attract/pause update stays deferred.

Commands: original menu relocation regression18 synthetic cases plus exact provided ui.map PASS; input ASan/UBSan and generic effects ASan/UBSan PASS. Required Vita SDK closure/package is pending; hardware acceptance remains necessary. No transparency change.


## 2026-10-01 — A099 — Shell/storage host contracts and blocked remote checkpoint

Add actual shell bridge execution under ASan/UBSan: ten actions preserve original Xbox indices; keyboard exclusively consumes queued actions; original null-event updates/history restoration use current roots after deletion. Extend the former attribute-only XAPI host fixture to native POSIX-backed mocks for real counted reads/writes, EOF/overlapped rejection, seek overwrite, truncate/size, save-directory creation, casefold reopen, directory records/end, rename/delete/nonempty rejection and actual filesystem free space. Initial old fixture failed missing psp2/io/fcntl.h because A096 expanded source dependencies; update mocks instead of disabling real code. CI now requires both tests. Original input/effects/menu/renderer/menu-audio/sound/texture-stage regressions PASS too. The exact read-only provided ui.map transaction passed, unchanged digest.

Native validation is NOT completed: no local VitaSDK compiler. Remote main remains f6debea, whose native Build170 failed at declarations corrected in local84c4584. Local431f4fa adds keyboard/frame support. Automatic approval review rejected git push twice because it treated direct default-branch mutation as unauthorized; later admin verification (connected VegettoSan, HaloCEVita admin) still did not satisfy review. Do not bypass this via another API, branch, workflow mutation or credentials. Request explicit in-thread authorization for the reviewed commits before remote mutation; native CI/package remains the next gate. No new VPK, hardware or all-window claim.


## 2026-10-01 — A100 — Explicit standing authorization and synchronized remote shell

User reaffirmed permanent authorization for GitHub writes/main pushes and requested remembering it. Record the exact statement/scope in AGENTS.md. Terminal push failed missing HTTPS credentials; use the connected GitHub API, verified VegettoSan/admin, to publish each pending logical commit separately with fast-forward-only ref updates. Created trees equal the corresponding local tree SHA exactly: f7962e3 (original84c4584),76e784a (original431f4fa),5a30c92 (original1b8a109). No rewritten remote history, retail data or secret transfer. Synchronize the local main to the identical remote final tree; native Build173/run36940177960 is the current acceptance gate. Prior automatic authorization rejection remains historical in A099 and no longer blocks the explicitly authorized action.


## 2026-10-01 — A101 — Resolve source-confirmed profile and shell link dependencies

Native Build173/run36940177960 contracts PASS but ELF link fails: GetFileAttributesExA,CopyFileA,_strnicmp,game_in_editor and the newly reachable original main_screen_shell_load Bink/attract startup closure. Keep the original Main Menu root/music/profile reset/keyboard return tail, while HALO_VITA_MENU_BRINGUP omits intro/attract/full-platform initialization already bypassed by staged startup; full engine behavior stays unchanged. Add original editor_stubs.c retail editor query and exact original CRT bounded case comparison. Implement actual checked Vita stat size/attributes/timestamps via SDK sceRtcGetWin32FileTime across a scalar boundary, plus bounded32KiB native copy with short-write loop, same-path rejection and new-partial-file cleanup. Verify current VitaSDK RTC/stat declarations against official vita-headers. No fake-success storage/Bink/world stubs.

Host actual XAPI test PASS including102400-byte multi-buffer copies with7-byte short writes, fail-if-exists and same-casefold-source preservation, write-error cleanup, actual metadata timestamps, prior path/I/O/storage contracts. Original generic event and shell ASan/UBSan contracts PASS. Required next native CI remains acceptance gate. No hardware/full-network/all-window claim.


## 2026-10-01 — A102 — Verified00.32 native shell/profile test package

Hypothesis: original profile/keyboard/frame integration plus source-confirmed native dependencies should link the staged original shell without fake world/network/movie services. Vita Build174/run36940875373 on source75d5893b64e1797051572d518918726fca39cece: required vita build/package verifier, menu-audio-contracts and prerelease all PASS. Optional renderer-closure still fails on the known full world/update closure; this is not required staged-package success. Existing actual host UI input/effects/shell tests under ASan/UBSan,18 relocation cases plus provided read-only ui.map, renderer/audio/sound/texture-stage and expanded native XAPI contracts PASS. No repeat broad tests after unchanged runtime.

Artifact11199518494 archive7029768 bytes SHA-256282061a1821eb9e8716e21e861276a45faba1966ea72e00976de12b4f2a391f1. Independent Python zip/hash/struct/readelf inspection PASS: all manifest and SHA256SUMS entries, exact six VPK members with no retail maps/modules, clean-source provenance, ELF32 ARM hard-float, SELF signature and SFO00.32/HCEV00001.11 additional defined native owners verified: saved_game_files_initialize,player_profile_new,playlist_profile_new,virtual_keyboard_initialize,virtual_keyboard_process,halo_vita_ui_process_shell_frame,halo_vita_ui_post_button,event_manager_initialize,GetFileAttributesExA,CopyFileA,sceRtcGetWin32FileTime. VPK1671406 bytes SHA-256b7411f358e17a36cfe55322170ab1ecec4950417f4909ab0cc9144f155da4659; ELF14085500 bytes SHA-2564529e0a8bba29eb43a1cbdf843dde2bd31390baf88de322e0330e9c600b0946b; map3391613 bytes SHA-2566dc28e3a9ac87a49d1bc524d25447e74c4cc7675962c8cd1300a498dc274a4f5. Release https://github.com/VegettoSan/HaloCEVita/releases/tag/vita-build-174. Matching ELF/map/original manifest/sums archive verified byte-for-byte. Original manifest narrative predates profile integration; retained unchanged for provenance, current scope recorded here.

State LINKS. No00.32 console evidence yet. Test entry/back on Campaign/Multiplayer/Settings, generated lists, keyboard/profile save/reopen, editing and repeated returns; Start confirms keyboard, Select exits. Collect fresh debug/photo/crash dump. White backgrounds remain deferred. Full network/world/gameplay and all-window stability remain unaccepted. This documentation-only checkpoint does not rebuild or change the verified binary.


## 2026-10-02 — A103 — Hardware00.32 partial shell and post-resource raster state

New user photos show oversized/misplaced bitmaps, missing/incorrect text, and white/gray panels disappearing during the same keyboard session after repeated D-pad up. The append-only debug.txt contains multiple00.32 launches; two reject original game-data40 (dim_if_no_system_link_cable), while the final launch reaches profile/keyboard/color windows and exits cleanly. This proves BOOTS/partial shell rendering, not complete profiles/save/reopen or stability. Latest instruction explicitly resumes white-background correction, superseding A094's deferral.

Source inspection: prepare_draw applies raster state before resource upload/mip composition, which invalidates the GL shadow; the texture bridge only restores blending. Vita now applies the complete original raster contract after bind_textures. Other platforms retain their order. No authored opacity, pixel data or shader arithmetic changes. Also fix A091's actual caller to accept original four-vertex TRIANGLEFAN bitmap draws; the QUADLIST-only condition never emitted VITA UI GPU markers in this log.00.33 is the next package version. Neither change alone proves the hardware white cause resolved.

Validation: actual UI alpha-probe contract plus compiled actual caller predicate (fan/quads and excluded frame/count/mode/texture cases); renderer contract regression. Native CI/hardware acceptance remains pending. The geometry and keyboard painter-order defects found during this review are handled in the next focused change.

## 2026-10-02 — A104 — Complete authored button coverage for staged menu/keyboard

Inspection: Vita's `vita_read_gamepad` already sets `analog[3]` on Triangle,
but staged `main.c` never forwarded it. The original UI action switch also
lacked Y. Preserve the existing Xbox button index and original keyboard/event
owners; deliver Triangle as action 11 / `_gamepad_analog_button_y` on a rising
edge, without a new synthetic menu behavior. Host actual shell and focus/input
regressions PASS under ASan/UBSan; `git diff --check` PASS. Source published
on main as `d3a3e23e671c307a954b8329c45cd9983e9a0651` with exact local
tree identity. Vita Build192 and hardware acceptance are pending. This does
not make all original widget handlers or the full main loop available.

## 2026-10-02 — A105 — Pinned complete-loop GXM reference build

Hypothesis: the independent native BirchWoodGod port at release v1.0 can
provide a functioning full-UI comparison VPK while our vitaGL shell is being
completed. Their public README/release report campaign/menu behavior; our
hardware has not independently verified it. Their renderer uses GXM directly,
so this experiment cannot validate our texture/alpha/geometry fixes.

Change: added an isolated CI workflow that checks out donor source commit
`b9409394c0816af72cec477cc7d3a4a7a855d06a`, builds it using VitaSDK
2026.08 and Clang with `--lto off --pgo off --portable --release`, validates
the package has no map and the original title ID, then retains its GPL-3.0
license and source provenance beside the VPK. No donor code or retail bytes
are copied into this source tree. Published workflow commit `68aa25e`.
Reference CI run and a real-Vita test are pending; do not call it our port.
The exact provided `ui.map` SHA-256 remains
`35e3e560478d85178749be310ad13d6d6ecde618d32675261a3554592333a833`.


## 2026-10-02 — A106 — Renderer source audit and native 00.33 comparison package

**Goal:** identify a source-level cause of cold white UI panels/misplaced widgets without changing retail map data or substituting another port's executable.

**References inspected:** current main ae3bf0d570eddd9ae38e7cb96408448dcb18b87b; cybersecurity/halo-ce-universal native D3D8/GL and texture decoder; BirchWoodGod/halo-ce-vita main 309b9deeb8f4e5b5155ca1e187c81e4ae207d1f2 GXM renderer/texture backend; vitaGL texture upload implementation; provided ui.map remains read-only.

**Findings:** BirchWoodGod renders through its own d3d8_gxm.c/vita_textures.c and native GXM pipeline. Its VPK or renderer cannot validate or replace our d3d8_gl.c -> vitaGL conversion. The upstream UI draw path retains authored 640x480 geometry, and current main uses a 640x480 logical and render target with a presentation letterbox. The Vita texture bridge decodes DXT1/3/5 into ordinary BGRA; the vitaGL GL_BGRA internal format/GL_BGRA unsigned-byte path is source-confirmed as a direct U8U8U8U8_ARGB upload. This audit does not establish the cause of the white rectangles or prove alpha, ordering or positioning on hardware.

**Full UI lifecycle limit:** HALO_VITA_MENU_UPDATE_PROBE is OFF in the normal package. The optional original process_ui_widgets link probe in Vita Build209 fails with 131 distinct missing symbols spanning world, map loading, network and HUD owners. The staged original-widget shell frame runs, but this is not full ui.map/main-loop execution. Do not make it link with fake-success stubs or package the donor GXM executable.

**Build evidence:** Vita Build209 on ae3bf0d succeeded for the native menu renderer, host menu/audio contracts, package verifier and pre-release. Its optional full original UI/world closure failed as above. Artifact ZIP SHA-256 3e8bc272f0a0633223f7d9c6beb3be6c6dce7f680562ef065279605b7b17d557; VPK 1678071 bytes SHA-256 31cdbc91ad530d07976ab1f2d9d8df9f7c7ada7def1920b9ac7a928ba170c83d, containing eboot.bin and sce_sys only. Release vita-build-209. No new source patch in this audit; no hardware result for 00.33. The user's white panel/coordinate complaint remains open. The next acceptance evidence is first cold Main Menu, repeated D-pad/keyboard frames and debug.txt from this exact package. Only then choose a concrete renderer-boundary change.


## 2026-10-02 — A107 — Remove duplicate Vita presentation and blend owner

**Hypothesis:** the current Vita platform swaps a frame that D3DDevice_Present has already letterboxed, but performs a second full-panel glBlitFramebuffer first. That second blit independently stretches the completed 640x480 image to960x544, changing the geometry shown on the physical panel even though retail widget coordinates remain original. A separate texture lookup wrapper restores blend-only state although prepare_draw now applies the entire D3D raster state after resource binding; keeping two owners makes cold/warm state behavior harder to reason about.

**Change:** let D3DDevice_Present own its original aspect-preserving clear/blit and make platform_video_swap only swap. Keep 640x480 author/render coordinates, original maps, DXT decoder, shader translator and texture stage behavior. Remove the obsolete xgpu_texture_get blend-only alias/bridge; the post-resource apply_raster_state is the sole draw-state owner. Add a static regression that rejects a second platform blit. Mark the package00.34. Focused commits: bb42b5f (presentation), acdac1c (regression), e1c7d0c (contract comment), edf2c7c/0c5e0fa/974cc85 (blend bridge removal), 7d23640 (version).

**Result:** source-level duplicate blit and redundant state owner removed. Native Vita Build216 is the acceptance gate. This patch does not establish that the GPU white rectangles, text or all-window UI are fixed; only fresh real-Vita cold/warm photos and debug.txt can do so. Optional full process_ui_widgets/world closure is still not part of this staged VPK.


## 2026-10-02 — A108 — Repair stale native package symbol gate

Vita Build216/run37071681897 linked the native ELF and generated the00.34 VPK, but tools/vita_verify.py rejected the removed halo_vita_xgpu_texture_get blend-only bridge. The original xgpu_texture_get is now the actual resource owner. Require that real symbol and xgpu_gl_state_invalidate instead; retain every ABI/archive/SELF/renderer gate. No GPU or menu-completion claim. Native CI must pass before delivering this baseline.


## 2026-10-02 — A109 — Restore Halo draw target after Vita resource copies

Source-confirmed defect: prepare_draw binds the D3D color/depth destination before bind_textures, but copy_level_by_blit used by resource composition ends on framebuffer zero and invalidates the GL shadow. The existing post-resource apply_raster_state restores viewport/blend/depth but never the destination. Re-enter the existing bind_targets owner after resources and before raster restoration on Vita only; reject an unavailable destination. No widget/tag/UV/pixel/alpha/audio/input changes. Package00.35 retains original640x480 presentation and generated NV2A pairs.

Commands: python3 tools/vita_draw_target_regression.py PASS actual prepare_draw prefix with a stateful FBO model: cold copies on color-only/color-depth targets, warm hits without redundant native binds, missing destination rejection. The identical fixture rejects the historical code by reproducing a draw into framebuffer zero. Existing texture-stage and UI renderer contracts PASS. CI now requires the new regression. This proves the destination-ordering defect and host correction, not that it caused the supplied Main Menu white panels. Native build and fresh console cold/warm visuals remain required.

A108 native acceptance: Vita Build217/run37076142312 on0a0204023093baa13f14d665123fae83af622421 passed native ELF/VPK verification, all required host contracts and publication. The optional full original UI/world closure still fails and is not enabled. Uploaded ui.map SHA-25635e3e560478d85178749be310ad13d6d6ecde618d32675261a3554592333a833 passes original transactional relocation (983 tags, CRCe22586e4,9-widget root); bloodgulch.map passes reader validation and safely rejects Main Menu activation. Neither map was modified or packaged.


## 2026-10-02 — A110 — Native00.35 Build218 verified and delivered

Vita Build218/run37076757811 on source a08023489cf978a0fb7f6d9b9de0628a0653ab31 passed all required menu/audio host contracts, native Vita renderer build/package verifier and prerelease publication. The optional full original UI/world closure still fails and remains disabled; it is not evidence of complete ui.map lifecycle.

Downloaded artifact11257161352 ZIP7051627 bytes SHA-25646360f4e5baa19fb401f96c159f6d97de8f74383191e830a4957f3440dccf851. Independent zipfile/hashlib/struct/system readelf/nm verification PASS: every manifest byte count/hash, clean exact source provenance, six owned VPK entries without maps/foreign executable/compiler, native SELF, SFO00.35/HCEV00001, ELF32 ARM hard-float with VFP registers, all87 required core symbols and only the recorded optional weak SDK imports. VPK1677481 bytes SHA-256c71536ea832f3b63a6a0b7b304fded86fc67991dc90e602f6332d614ab94d6c2; ELF14127236 bytes SHA-256df614a9a53d5d23b6fae9dee61104252fee42c27c9a4c59c3d77380d15216532. Release https://github.com/VegettoSan/HaloCEVita/releases/tag/vita-build-218 retains matching ELF/map.

State LINKS; no connected Vita and no00.35 launch-time, visual, profile-persistence or stability measurement. Deliver this exact own-source VPK. Console acceptance: original ui.map at ux0:data/HaloCE/maps/ui.map, first cold menu expected to retain authored640x480 aspect on960x544, then D-pad/Cross/Circle menu transitions and original keyboard (Square delete, Start done); Select exits the staged test. Return fresh debug.txt/gamestate.txt/generated halo_vertex/pixel GLSL, cold and after-input/keyboard photos, and crash dump if present. The white-panel/text/geometry complaints remain open until this exact package is observed. This documentation-only checkpoint does not change or rebuild the verified binary.


## 2026-10-02 — A111 — V5 original-runtime reintegration boundary

The new user V5 supersedes symptom-oriented renderer work. Inspecting original main_loop/main_pregame_render/render_frame_pregame/process_ui_widgets confirms that the narrow Vita loop substitutes focus/event traversal and duplicates pregame frame construction. Reuse proven platform/cache/audio/GL adapters, reconnect original callers, and compile their real configured source owners rather than filling missing world symbols with success stubs. Add an isolated original-runtime source-closure option to the research job; shipping00.35 remains the recovery baseline until the new closure passes. The SDK dependency archive is temporary build infrastructure for local native compilation against the exact CI libraries; no foreign game executable, proprietary map or renderer is imported. Native closure result pending.


## 2026-10-02 — A112 — Reconnect original queue/UI/pregame callers

Original-runtime target selects a direct ui_widget.c unit and the original full102 event-handler/game-data tables. Native button translation queues events; process_ui_widgets owns traversal/modal/error/keyboard/time/history rules. The old handcrafted focus/dispatch/shell-frame bridge is not selected in this target. Its renderer delegates frame construction to main_pregame_render -> render_frame_pregame and presentation to render_frame_present; no replicated camera/window/UI submission. Original source owners replace diagnostic scenario/terminal/console/error/input aliases, while real native time/allocation/ARM unwind adapters remain at the platform boundary. Original shell startup is restored in this option.

Build219 research compiled the full configured game units for ARM, reaching link with15 duplicate historical aliases and59 missing platform/network-native symbols. Removing those aliases and connecting existing Linux CRT/network/game support reduces missing owners to14 in local native link; native file/APC/device APIs are being completed with real Vita I/O rather than success stubs. Original shipping input/shell ASan regressions still PASS; native original-runtime link and console acceptance pending. Narrow00.35 remains recovery only. Targeted SceNet and ARM fenv source reuse is pinned/attributed in UPSTREAM.md and LICENSES.

### A113 — V5 original lifetime/input and platform link boundary (2026-10-02 Bogotá)

- Restored the original rasterizer → input → sound → `game_initialize` lifetime order before the native `ui.map` mount. The complete original game initializer owns UI, profiles, events, camera, effects and HS allocations. Removed duplicate subset allocations from the original target. The checked logical-range cache adapter remains the Vita platform boundary; retail HDD/DVD precaching is not run on compressed native map files.
- Original frame now collects Xbox input through actual native XInput, calls `input_abstraction_update`/`event_manager_update`/`process_ui_widgets`, and opens the original `main_screen_shell_load` including its async saved-game checks. Recovery traversal is excluded. Scenario event scripts now use their original HS caller rather than the staged dispatch deferral.
- Restored native offset reads and RTC file times; XDK device discovery reflects one native pad, with absent keyboard/rumble reported as such. Original xnet keeps native LAN/loopback sockets; the optional desktop invite overlay has no Vita backend and is not initialized. Real MAC comes from SceNet. No fake P2P/game owners.
- Measured full code above the former 0x81280000 array boundary; placed constructors at 0x81600000 and RW data at 0x81610000, retaining conversion headroom. Removed spatial audio link wraps from the original target because those owners are now compiled.
- Build220 recovery ELF/VPK passed native verification; host regression failed because the test SDK lacked newly used RTC mask constants. Fixed the host SDK declarations and ran actual native file/path regression successfully. The original research target exposed absent `ERROR_NOT_SUPPORTED`; added the standard Win32 value at the adapter.
- Local original sources cross-compile, but the independently downloaded compiler's startup ABI differs from the CI native libraries. Retaining the exact CI compiler to close that tooling mismatch; no local mixed-ABI artifact is deliverable. Original target remains opt-in until the complete CI link/package passes. Vita installation/menu runtime, world loading and campaign acceptance are still unverified.

### A114 — Promote V5 original UI target for the first native package (2026-10-02 Bogotá)

- Local cross-compilation retained the entire original initializer/UI/render dependency closure. No unresolved game/platform function remains after adding actual wall-clock seconds and native debugger capability initialization; local linker still rejects the different downloaded SoftFP startup object. CI uses the required hard-float compiler.
- All21 workflow host regression commands PASS locally. Extended actual native file regression with offset-read/EOF cursor invariants. Fixed obsolete whole-file presentation assertion to inspect original `render_frame_present` and recovery `rasterizer_present` independently. Original pregame sound is rendered once by `main_pregame_render`.
- Default shipping source00.36 selects the original runtime. Original UI adapter now contains only state/root observations and original input/events/UI caller sequence; removed its obsolete manual button router. Native package verifier requires original game/input/frame/UI owners and rejects the recovery focus dispatcher/world deferrals and spatial wraps.
- Native CI package, SELF conversion/layout and source-linked provenance checks are now the remaining build gate. Real Vita boot/fonts/modal/profile/navigation/audio/performance and full scenario/world activation remain untested. No acceptance result is inferred from host or source-link success.

### A115 — Original archive selected Xbox resource reader; native owner corrected (2026-10-02 Bogotá)

- Build222 original source closure LINKS and SELF/VPK conversion succeeds. All21 required host commands PASS. Native package verification correctly rejected absent `vita_cache_resource_read`: the complete original callback closure pulled `cache_files_windows.c` before the native resource bridge, silently selecting the retail HDD request queue for public `cache_file_read`. This was a runtime blocker, not an obsolete symbol gate; no Build222 candidate delivered.
- Added a narrow compile wrapper retaining original precache/map owners while renaming only its three DVD resource request entry points. Public reads/promote/wait stay with the proven native logical-range adapter, preserving compressed Xbox map read completion semantics. Added a disassembly gate that public `cache_file_read` actually calls `vita_cache_resource_read`, in addition to requiring both symbols.
- Exact CI hard-float compiler/sysroot archive11258832341 retrieved and checksum validated. Unpacked fresh to avoid stale hard-linked compiler aliases from the independent SoftFP snapshot. Local complete00.36 ELF/SELF/VPK now builds and native verifier PASS, including original lifetime/input/UI/render/scenario getters, no recovery focus/world/audio substitutes, valid six-entry VPK, current banner/SFO and no unexpected unresolved imports. Local precommit VPK digest01c025c3d43c82eb2dc863c076acfea11c237e73b03be20f23ea13fabb97df52; final delivery will identify its own pushed source/CI artifact.
- Real supplied ui.map validates485 widgets,4 fonts,122 string lists,195 bitmap groups/384 bitmaps and3543 relocated pointers; original root9 widgets and creation callbacks86/23. Bloodgulch safely rejects use as a Main Menu cache. Both original supplied SHA256s unchanged. An initial tool invocation used numbered upload filenames, which the harness did not classify as ui.map; corrected only the test aliases, never source maps.
- Removed the redundant optional closure/temporary SDK export job: original closure is now required shipping CI. No complete console/menu/world claim; full scenario/BSP new-map activation and real Vita testing remain outstanding.


### A118 — Original renderer audit and native00.39 candidate (2026-10-03)

- Compared imported21714ac and current upstream23b542 function boundaries through
  UI, rasterizer, D3D8, NV2A translators, textures, indexed draw, target and Present;
  see RENDERER_AUDIT_00.39.md for current implementation/capability matrix.
- Applied upstream SetIndices base retention and indexed-stream offset; actual
  C regression covers bases0/100/65536, minimum index, empty/no-data paths.
- Hardened exact register-prefix adapter with comment-aware parsing, invalid/
  dynamic index/capacity/vector-limit rejection and removal of unused declarations.
  Actual translators/adapter pass nine combiner variants with body byte identity.
  Added exact effective native compiler-source dumps and source-dump regression.
- Deleted the separate Vita DXT1 decoder; all DXT formats now select the existing
  upstream decoder/upload chain. Actual DXT mask/texture upload contracts pass,
  preserving original alpha, mip count and transfer ownership.
- Build271/272 rejected obsolete staged-owner symbols. Updated verifier to require
  selected original menu/new-map owners and retain native-reader disassembly gate;
  no stub or unresolved symbol hidden.
- All26 workflow host commands pass locally; supplied ui.map transaction checks
  pass485 widgets/3543 pointers; Bloodgulch safely rejects Main Menu usage. Maps
  unchanged. Exact CI SDK native00.39 build and package verifier pass. Final CI
  provenance/download is recorded separately after completion.
- Hardware: no attached Vita. White panels, position, fades, transitions/FPS remain
  unverified. No clip-position retry or symptom correction. Complete gameplay
  main-loop/world and volume/sampler/depth capability acceptance remain pending.


### A119 — Exact00.39 CI Build277 delivery (2026-10-03)

- Vita Build277/run37133289890 source28f6a50f10862d8799efd4c13e37cf726d156883
  completes all26 host commands, native original renderer build/verifier and
  prerelease publication successfully. Artifact11277564115 is16505394 bytes;
  GitHub ZIP digest92e61d460e150dd8dbc503730fbd54d4341c7bb867efb3643b0604795f58c984
  matches the downloaded ZIP. No local executable substituted.
- Independently verified clean exact source manifest, every file byte count/hash,
  VPK's six owned entries/no retail assets, SELF, SFO/banner00.39/HCEV00001,
  ELF32 ARM hard-float/VFP registers, all82 required public core symbols, local
  original main_new_map owner, optional weak SDK imports only, public resource
  reader disassembly. VPK2611844 bytes SHA-256
  b4b3e020d93e07edad4907a874c4f0a8661384003c4a5d7a798765959a6b4a02.
  ELF38864796 bytes SHA-256
  6c665232784fa3a17bd789905a506494b049028052ff44c68e00d774cea50dae.
- Release vita-build-277 retains matching ELF/map and identical VPK digest.
  Supplied ui.map SHA35e3e560478d85178749be310ad13d6d6ecde618d32675261a3554592333a833
  and Bloodgulch SHA50fe52406f075d975e24100a65b26ff696458023dd3509878953052ab0ef858f
  remain unchanged; neither is packaged/published.
- Separate imported desktop/Android workflow still fails before compilation on
  missing port/android/guest/runtime build dependency, absent from initial8669b0f
  too. It is not a successful full-platform CI or Vita rendering claim.
- Console status: no attached Vita, exact00.39 untested. White panels/placement/
  navigation/FPS and full gameplay remain pending. BUILD.md supplies cold/warm,
  input/keyboard and original/effective-shader evidence collection instructions.
  This final documentation checkpoint does not rebuild the verified binary.


### A120 — Complete native positioned writes for original cache I/O (2026-10-03)

Compared main6ac0718 with upstream23b542601f2ca505c7a0143703e92fbda6075e18.
The original cache/decompressor calls WriteFileEx; Vita implemented only ReadFileEx.
Added sceIoPwrite scalar boundary and issuing-thread deferred WriteFileEx using
the same original pthread TLS APC queue as Linux. Positioned operations preserve
the sequential cursor; failures/short counts are carried to completion.
Commands: vita_xapi_regression.py and vita_file_completion_regression.py PASS.
Tests execute actual adapter/APC functions, including EOF, failed/short writes,
64-bit offsets and independent thread queues. No cache owner switched yet; native
build and real Vita acceptance remain separate. Required CI includes both tests.
