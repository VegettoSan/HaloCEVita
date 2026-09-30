# Vita build notes

Project milestone: **RENDERS diagnostic + synthetic NV2A triangle on real Vita** (A016). Tested native ARM32 baseline **00.05** compiles both generated shaders, links and draws the visible triangle. GPU-copy/core tests pass, input/header rechecks work and Start exits at34.3s. Full Halo renderer/main/menu/campaign, multipass invariance and repeated stability remain unverified. The verifier associates runtime evidence with the exact tested VPK digest; a changed rebuild remains LINKS until tested.

Current deliverable is **00.07: LINKS** (A019), a real menu-tag/accessor prerequisite awaiting Vita. Hardware baseline00.06 BOOTS: A018 hardware validates kernel reservation, original memory/state functions and both real map tag-directory checkpoints. The known00.05 graphics baseline and matching symbols are in build/vita/attempts/a017-baseline-00.05/. Full main/menu remains pending; TAG INDEX PASS is not a menu or gameplay claim.

## Toolchain

Preferred target: standard/native VitaSDK (`arm-vita-eabi-*`). This is a native source port, not an Android `.so` loader port.

On the current development setup, a standard VitaSDK may coexist with a separate SoftFP SDK. Codex must detect the active toolchain and **must not overwrite or mutate SDK installations**.

Check first:

```bash
echo "$VITASDK"
which arm-vita-eabi-gcc || true
which arm-vita-eabi-g++ || true
arm-vita-eabi-gcc --version || true
cmake --version
ninja --version
```

If a standard/hardfp SDK is installed separately, select it for this project before building. Do not assume a path without verifying it.

## Verified build command (Ubuntu WSL)

```bash
cd /home/vegettosandev/HaloCEVita
export VITASDK=/usr/local/vitasdk-hardfp
export PATH="$VITASDK/bin:$PWD/build/vita/tools/ninja/usr/bin:$PATH"
python3 configure.py --release
ninja vita
ninja vita_vpk
```

Both root targets are implemented. `vita_vpk` also builds eboot and runs `tools/vita_verify.py`. `--release` selects RelWithDebInfo for this experimental target; game assertions remain enabled until boot is validated. The existing SDK is not modified. Vita build wrapper rejects a SoftFP/non-ARM32 compiler.

Ninja was absent on this workstation. A local copy was extracted in ignored build/vita/tools without installing/upgrading SDKs:

```bash
mkdir -p build/vita/tools
cd build/vita/tools
apt-get download ninja-build
dpkg-deb -x ninja-build_1.13.2-1_amd64.deb ninja
```

On another machine, use its installed Ninja or substitute the actual downloaded package filename/version. Do not assume that this package version is always current.

Direct CMake alternative:

```bash
cmake -S port/vita -B build/vita -G Ninja \
  -DCMAKE_MAKE_PROGRAM="$PWD/build/vita/tools/ninja/usr/bin/ninja" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/vita --target vita_vpk -j6
python3 tools/vita_verify.py
```

Current build log: `build/vita/logs/build-a019-final-delivery.txt`; historical logs retained. Package hashes/byte sizes: `build/vita/artifacts.json` and `SHA256SUMS`. Generated build/Ninja/ELF/VPK artifacts are ignored, not committed.

## Expected outputs

```text
build/vita/eboot.bin
build/vita/HaloCE.vpk
```

Optional debug assets/log maps may also be generated under `build/vita/`.

Keep `HaloCE.elf`, `HaloCE.elf.map`, `HaloCE.elf.velf` and artifacts.json for crash analysis. Final package SHA-256:

```text
6f8c72b892f3c1c2793a8ee57b1adaeff8efad23686ca3a3498e1b489a8e8ae4  HaloCE.vpk
```

Rebuilding can change package timestamps/hash; use the verifier's fresh SHA256SUMS for the package actually installed.

### Installation correction 00.02 (A012)

Historical correction in APP_VER 00.02, retained in current 00.05: three LiveArea images use opaque indexed PNG-8/type 3 with two palette colors; RGBA/type 6 was the identified packaging defect consistent with 0x8010113D. This follows [VitaSDK samples](https://github.com/vitasdk/samples#notes-on-images). The verifier rejects old images and checks all packaged images, CRC/pixel streams and XML references. A012 changed packaging only; subsequent user logs now demonstrate install/boot.

Transfer the new VPK to Vita and retry installation normally through VitaShell. If an existing Halo CE Vita bubble prompts for replacement, accept the update. No map/data removal, database rebuild, firmware change or runtime-compiler reinstall is part of this packaging correction. Confirm whether installation reaches completion and whether the app subsequently launches; these are separate milestones. If installation still fails, report the exact code, percentage/stage and VitaShell version, and verify that the transferred package matches the hash above. The original rejected VPK/evidence is retained in ignored build/vita/attempts/a012-original-rgba/.

### Graphics correction 00.03 (A013)

The prior executable banner said00.01 even in package 00.02, because it was hardcoded; since00.03 the CMake version supplies both SFO and log banner. The earlier user logs prove core initialization and valid ui/a10 headers, but `graphics=0` suppressed our frames. This was our interpretation error: [vitaGL 6e7fe40](https://github.com/Rinnegatamante/vitaGL/blob/6e7fe40/source/vgl.c) returns a resolution-fallback flag, normally zero at 960x544, not success/failure. Waiting longer cannot exit that code path.

00.03 accepts normal/fallback returns, checks version/viewport, and logs memory without changing pool sizes. It submits an initial diagnostic frame with GLSL TEST PENDING before GPU-copy/shader probes. A014 confirms this frame becomes visible, GL error0, viewport960x544 and GPU-copy PASS. The next crash is in shader linking, before011. 010 remains deliberately absent. Original00.02 artifacts are preserved in build/vita/attempts/a013-original-00.02/; evidence is in ignored docs/runtime/.

Host regression (actual init function, SDK mocked; does not prove GXM):

```bash
python3 tools/vita_init_regression.py
```

### Shader-link recovery 00.04 (A014)

The 00.03 dump localizes data abort to SceGxm, called by glLinkProgram's vertex-parameter query. Deferred compilation can leave a NULL program; it reports success before compilation and does not recheck that program after compilation. The specific compiler rejection is not yet observable in the old log. 00.04 uses the documented vertex-then-fragment SHADER_PAIR contract, checks both real statuses before attach/link, and continues the diagnostic on failure. It captures VitaShaRK diagnostics directly in debug.txt and saves retained rejected source as vertex_probe.cg/fragment_probe.cg. Original GLSL is still saved. NV2A translators and memory pools are unchanged.

Nine host cases exercise the actual probe with a mocked SDK: success, either/both compile failures, absent compiler, either shader allocation failure, program allocation failure and link rejection. Failures before successful compilation never attach/link; objects are released and failed source lengths are checked. This does not prove console recovery or compiler compatibility.

```bash
python3 tools/vita_shader_regression.py
```

Matching 00.03 ELF/eboot/VPK/map/hashes/dump are preserved in build/vita/attempts/a014-original-00.03/. Metadata-only symbolization is recorded in logs/crash-a014-analysis.json; it requires the exact matching ELF and treats stack words as candidates, not an unwound backtrace. Never symbolize the old dump against another build's ELF.

### Vertex qualifier correction 00.05 (A015)

The 00.04 hardware log proves recovery: vertex status0/pixel status1, attach/link skipped,011, repeated input/header checks and Start exit at21.5s. The compiler's line177 error identifies `invariant gl_Position;` in the exact saved Cg. 00.05 excludes this unsupported declaration only on Vita and logs its omission; arithmetic/varyings/helpers stay intact. Desktop/Android retain it. GLSL's cross-program invariance guarantee is not established on Vita; a real multipass alignment test remains required (KI-015/D009).

Host comparisons compile the actual pre/post generators and string builder for114 cases per Vita/desktop/Android backend. Vita output changes only by this declaration; desktop/Android output is identical. The baseline synthetic probe exactly matches supplied hardware GLSL. Saved generated C snapshots/outputs are in build/vita/tests/vertex-a015/; result in logs/vertex-regression-a015.txt. This checks source generation, not ShaccCg acceptance.00.04 package/symbols/source are archived in build/vita/attempts/a015-original-00.04/.

### Hardware acceptance 00.05 (A016)

The new user log and visible result verify both compiles (status1, GL error0), program link1, runtime result1 and011 shaders1; the triangle is visible. Header/input rechecks continue until Start clean exit at34.3s. Runtime shader sources are byte-identical to the intended correction: vertex loses only the invariant line, fragment unchanged. Evidence copies are docs/runtime/2026-09-29-00.05-*. The00.05 hashes in A016 were unchanged when that result was recorded. Broader shader/state/texture coverage, real menu and multipass invariance remain pending.

## Libraries actually used

Existing SDK static libraries: vitaGL/VitaShaRK/ShaccCgExt, SDL3 3.4.12, pthread, mathneon, C++/newlib and native Sce stubs. The SDK SDL3 archive contains the Vita audio bootstrap; upstream dsound_sdl.c compiles natively. The diagnostic audio device opens paused, with no retail audio playback yet.

- VitaSDK libc/newlib and kernel/user libraries;
- vitaGL;
- VitaShaRK / runtime shader compiler requirements;
- `libshacccg.suprx` requirement on the console;
- Installed vitaGL archive demonstrably includes GLSL translation and VitaShaRK runtime compiler calls. For the generated shader test, install console `libshacccg.suprx` correctly at `ur0:data/libshacccg.suprx` (or vitaGL's fallback `ur0:data/external/libshacccg.suprx`). Do not put it in this repo/VPK.
- HENkaku-compatible homebrew setup/VitaShell; this SELF is built UNSAFE, so enable unsafe homebrew. No Android loader, kubridge or guest image is used.

## Game data

No retail Halo data belongs in the build tree.

The imported engine is the Xbox reconstruction: its real verifier requires **Xbox cache version 5**. PC Halo/Custom Edition caches/version7/609 and PC-style separate bitmaps.map/sounds.map are not accepted substitutes. Put Xbox `ui.map`, `a10.map` and the other map files from your own game in the target directory. ui/a10 names are found case-insensitively. The current milestone verifies headers only; it does not yet decompress/rebase/load their tags.

## Current00.07 Main Menu tag checkpoint (A019)

Install current HaloCE.vpk, verify banner00.07 and keep the matching ELF/map. Same Xbox-v5 ui.map; no conversion. Startup reads ui only, avoiding another57s a10 read. Existing diagnostic remains visible. Expect MENU TAGS PASS - ENGINE PENDING after these completed operations:

- 017: typed relocation committed,3362 pointers for exact user ui.map; rule counts1025 tag_block.address/866 tag_data.address/1471 tag_reference.name.
- 018: original tag_loaded resolves ui\shell\main_menu\main_menu and9 reachable widgets.
- 019: original bitmap_group_get_bitmap_from_sequence, unicode_string_list_get_string and font_get_character_by_ascii_code complete.
- 020: detach restores original CRCe22586e4;016 reports UI checkpoint1/1.

Cross repeats the changed UI contract; Start cancels/exits. Return debug.txt, or matching dump on crash. This VPK does not initialize UI widget state or draw the menu;010 is still withheld. New00.07 remains LINKS until tested. Host commands: python3 tools/vita_menu_regression.py /path/to/ui.map /path/to/a10.map and python3 tools/vita_program_regression.py. Campaign a10 safely rejects this menu-only plan because its main-menu tag is absent; A018 campaign read stays BOOTS. Journal/hash1.5MiB + at most32KiB graph state are transient within existing heap; no pool increase, full-map copy or retail data in outputs.

## Historical 00.06 integration test (A017/A018)

1. Install the current build/vita/HaloCE.vpk (banner/SFO00.06, SHA above). Keep the matching ELF/map/manifest for crashes. Use the same Xbox-v5 maps already under ux0:data/HaloCE/maps/; no map conversion or replacement is required.
2. Expect the existing diagnostic/triangle, then `READING REAL MAP TAGS`, then `TAG INDEX PASS - ENGINE PENDING` if both directory checkpoints pass. This still is a diagnostic, not Halo's menu. Streaming compressed a10 traverses274MiB logically; completed progress logs appear approximately once per second, frames/control polling continue, and Start cancels with cleanup.
3. Log expectations:013 reserves96MiB;014 original memory/state functions PASS;015 reads each map; `Halo original tag_iterator/tag_get/tag_index_is_group PASS` appears for both maps, then016 checkpoint2/2. Expected ui983 tags/a10 3357; section CRCs e22586e4/1c90dc0f for the exact host-tested files. These CRCs identify tag sections, not a retail cache-integrity guarantee. A different legitimate build/map may differ. Allocation failures or invalid/corrupt/missing data log and leave a diagnostic; nested tags/BSP/GPU resources remain inactive and010 stays withheld.
4. Cross rescans headers and repeats tag reads; Start exits, including during streaming. Test without maps as well. Return debug.txt and the visible result; retain the matching dump if there is a crash. A018 confirms00.06 BOOTS with013/014 PASS, both original tag API checks and016=2/2. ui4.82s/a10 57.12s, tag CRCs match. Supplied log does not demonstrate cancellation/recheck/exit or repeated stability.

Host verification of actual allocator/reader (no ARM/kernel/GXM execution):

```bash
python3 tools/vita_memory_regression.py
VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_cache_regression.py /path/to/ui.map /path/to/a10.map
```

The second command reads maps in place and prints metadata only; omit paths to run25 synthetic cases. The host harness uses host libz.so.1, with portable SDK headers if host development headers are absent. It never links the Vita ARM zlib archive into a host test.

## Historical 00.05 console test

1. Copy/install build/vita/HaloCE.vpk using VitaShell (TITLE_ID HCEV00001). Preserve the matching ELF/map/hashes on the PC.
2. Launch once even without maps. Logging/core probes should still execute; missing maps should be reported, not cause a deliberate exit.
3. Expected diagnostic screen: dark background, `HALO CE VITA`, `NATIVE HALO CORE PROBE`, CORE/MAPS/GLSL status text; a colored triangle if the upstream NV2A-generated shader pair compiles. **This is not Halo's main menu or retail geometry.**
4. Read `ux0:data/HaloCE/debug.txt`. Confirm new banner 00.05. Expected: 001/002/003/004;007 before real core probes;008 on root discovery;009 only when ui/a10 headers verify;005/006 around vitaGL. Then012 for the initial diagnostic frame, GPU-copy/shader begin/result logs and011 after completion. Milestone010 is intentionally absent. A submitted frame/log alone does not prove visible rendering.
5. Look for `Halo ... PASS`, `pthread ... value=42`, paused SDL3 audio stream result and GPU-copy PASS. Expect individual `NV2A vertex/pixel compile status` plus compiler stage/line messages. On rejection: `attach/link skipped; diagnostic continues`,011 and GLSL BLOCKED SEE LOG. On both successful statuses: attach/link begin and result, then triangle if rendering succeeds. Collect vertex_probe.glsl/fragment_probe.glsl and failed vertex_probe.cg/fragment_probe.cg if present. A recoverable shader rejection is useful evidence, not GLSL PASS.
6. Copy your Xbox maps to `ux0:data/HaloCE/maps/`. Press Cross to recheck headers. Test Circle/Square/Triangle/L/R/D-pad/Select/sticks; input log shows Vita buttons and mapped Xbox values. Start exits cleanly. White/Black and stick-click mapping is intentionally undecided.
7. Relaunch several times and leave it running for 30 seconds. Report visible results, missing milestones and whether Start exits reliably. Do not label STABLE/FPS/gameplay until those tests exist.

## What to return after a crash

- Entire `ux0:data/HaloCE/debug.txt` (append-only; preserve prior runs).
- `vertex_probe.glsl`, `fragment_probe.glsl`, plus rejected `vertex_probe.cg` / `fragment_probe.cg` if created; logs identify which dump belongs to the latest compilation.
- Vita error code and crash PC/LR/SP, screenshot or saved crash dump (for example the file under ux0:data if the console reports one).
- SHA-256/version of the VPK actually installed, real Vita vs Vita3K, firmware/homebrew/runtime-compiler setup, and last visible frame/action. No retail maps need be sent.

Decode an address against the matching ELF:

```bash
arm-vita-eabi-addr2line -a -f -C -e build/vita/HaloCE.elf 0xCRASH_ADDRESS
```

If the crash report uses a module offset or the module was relocated, supply its load base; compute the ELF address from the offset/base before symbolizing. Do not use an ELF from a different build.

For a gzip/ELF Vita crash dump, use the local metadata-only parser, with the ELF archived for that exact package:

```bash
VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_crash_audit.py \
  build/vita/attempts/a014-original-00.03/crash.psp2dmp \
  --elf build/vita/attempts/a014-original-00.03/HaloCE.elf \
  --output build/vita/logs/crash-a014-analysis.json
```

The tool records input/ELF digests, applies module segment relocation and does not export dump memory or system-info payloads. Keep full dumps ignored; they may contain loaded game/process memory.

## Compile/ABI/graphics evidence

```bash
python3 tools/vita_compile_audit.py --jobs 6 --output build/vita/audit-abi
python3 tools/vita_link_audit.py build/vita/audit-abi
python3 tools/vita_gl_audit.py
```

466/466 configured game C units compile; relocatable aggregation retains 584 CRT/platform imports and is not a final full-game link. dsound_sdl.c, d3d8_gl.c and xbox_textures.c compile in a separate platform audit against upstream GL declarations. Native Vita headers/archive compatibility is a separate matrix in GRAPHICS_COMPATIBILITY.md. Memory/address contract, full XAPI/renderer and remaining cross-unit ABI issues prevent full main/menu. ABI boundary/warnings: ABI_VITA.md.

Runtime data target:

```text
ux0:data/HaloCE/maps/
```

The VPK should create/use writable save/log/config directories without bundling the maps.
