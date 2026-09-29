# Vita build notes

Current milestone: **LINKS**, native ARM32 real-core bring-up `00.02`. This is a diagnostic package with actual Halo code; full Halo main/menu/campaign has not run. The user reports 00.01 failed VitaShell installation with 0x8010113D; 00.02 corrects its LiveArea PNG color format. Corrected installation/boot remain unverified.

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

Build logs: `build/vita/logs/final-build.txt`; package hashes/byte sizes: `build/vita/artifacts.json` and `SHA256SUMS`. `git diff --check` is clean. Generated build/Ninja/ELF/VPK artifacts are ignored, not committed.

## Expected outputs

```text
build/vita/eboot.bin
build/vita/HaloCE.vpk
```

Optional debug assets/log maps may also be generated under `build/vita/`.

Keep `HaloCE.elf`, `HaloCE.elf.map`, `HaloCE.elf.velf` and artifacts.json for crash analysis. Final package SHA-256:

```text
a3801dd4e0e39b14d732e6ec51a87f5ad37f09986136e0c57b9987b7a93320c8  HaloCE.vpk
```

Rebuilding can change package timestamps/hash; use the verifier's fresh SHA256SUMS for the package actually installed.

### Installation correction 00.02 (A012)

Use the current HaloCE.vpk, APP_VER 00.02, rather than the old 00.01 copy. Three LiveArea images now use opaque indexed PNG-8/type 3 with two palette colors; generic RGBA/type 6 was the identified packaging defect consistent with the reported 0x8010113D. This format follows [VitaSDK samples](https://github.com/vitasdk/samples#notes-on-images). The new verifier rejects every old image and checks all packaged images, CRC/pixel streams and XML references. Decoded artwork and ELF/map/eboot bytes are identical to the prior build.

Transfer the new VPK to Vita and retry installation normally through VitaShell. If an existing Halo CE Vita bubble prompts for replacement, accept the update. No map/data removal, database rebuild, firmware change or runtime-compiler reinstall is part of this packaging correction. Confirm whether installation reaches completion and whether the app subsequently launches; these are separate milestones. If installation still fails, report the exact code, percentage/stage and VitaShell version, and verify that the transferred package matches the hash above. The original rejected VPK/evidence is retained in ignored build/vita/attempts/a012-original-rgba/.

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

## Exact console test

1. Copy/install build/vita/HaloCE.vpk using VitaShell (TITLE_ID HCEV00001). Preserve the matching ELF/map/hashes on the PC.
2. Launch once even without maps. Logging/core probes should still execute; missing maps should be reported, not cause a deliberate exit.
3. Expected diagnostic screen: dark background, `HALO CE VITA`, `NATIVE HALO CORE PROBE`, CORE/MAPS/GLSL status text; a colored triangle if the upstream NV2A-generated shader pair compiles. **This is not Halo's main menu or retail geometry.**
4. Read `ux0:data/HaloCE/debug.txt`. Expected: milestones 001/002/003/004;007 before real cseries/heap/profile/data/pool/CRC probes;008 on root discovery;009 only when ui/a10 headers verify;005/006 around vitaGL. Then 011 (bring-up complete) and012 (diagnostic frame submitted). Milestone010 is intentionally not emitted. A submitted frame/log alone does not prove visible rendering.
5. Look for `Halo ... PASS`, `pthread ... value=42`, paused SDL3 audio stream result, `GPU framebuffer/blit copy probe: PASS`, and vertex/pixel/program compile logs. Shader failures are recorded and sources saved to vertex_probe.glsl/fragment_probe.glsl; compiler absence is logged.
6. Copy your Xbox maps to `ux0:data/HaloCE/maps/`. Press Cross to recheck headers. Test Circle/Square/Triangle/L/R/D-pad/Select/sticks; input log shows Vita buttons and mapped Xbox values. Start exits cleanly. White/Black and stick-click mapping is intentionally undecided.
7. Relaunch several times and leave it running for 30 seconds. Report visible results, missing milestones and whether Start exits reliably. Do not label STABLE/FPS/gameplay until those tests exist.

## What to return after a crash

- Entire `ux0:data/HaloCE/debug.txt` (append-only; preserve prior runs).
- `vertex_probe.glsl`, `fragment_probe.glsl` if created.
- Vita error code and crash PC/LR/SP, screenshot or saved crash dump (for example the file under ux0:data if the console reports one).
- SHA-256/version of the VPK actually installed, real Vita vs Vita3K, firmware/homebrew/runtime-compiler setup, and last visible frame/action. No retail maps need be sent.

Decode an address against the matching ELF:

```bash
arm-vita-eabi-addr2line -a -f -C -e build/vita/HaloCE.elf 0xCRASH_ADDRESS
```

If the crash report uses a module offset or the module was relocated, supply its load base; compute the ELF address from the offset/base before symbolizing. Do not use an ELF from a different build.

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
