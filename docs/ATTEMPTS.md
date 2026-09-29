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
