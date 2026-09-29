# Vita build notes

This file describes the intended build contract. Commands may evolve as the Vita target is implemented.

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

## Intended build command

Preferred end state:

```bash
python configure.py --release
ninja vita_vpk
```

Acceptable alternative if integrating the existing Ninja generator is impractical during the first milestone:

```bash
cmake -S port/vita -B build/vita -G Ninja
cmake --build build/vita
```

Whichever approach is chosen must become reproducible and documented here.

## Expected outputs

```text
build/vita/eboot.bin
build/vita/HaloCE.vpk
```

Optional debug assets/log maps may also be generated under `build/vita/`.

## Libraries likely involved

Evaluate, do not blindly assume:

- VitaSDK libc/newlib and kernel/user libraries;
- vitaGL;
- VitaShaRK / runtime shader compiler requirements;
- `libshacccg.suprx` requirement on the console;
- SDL3 if the current VitaSDK package is usable for the needed subsystems;
- otherwise native Vita control/audio/thread APIs.

## Game data

No retail Halo data belongs in the build tree.

Runtime data target:

```text
ux0:data/HaloCE/maps/
```

The VPK should create/use writable save/log/config directories without bundling the maps.
