# Vita port architecture

## Source baseline

The project is based on `cybersecurity/halo-ce-universal`, which already contains native Linux/Windows ports and an Android ARM port of the Halo CE Xbox decompilation/source reconstruction.

Important upstream facts for the Vita design:

- Halo data structures/cache files/saves depend on 32-bit layouts.
- The Linux port remains 32-bit specifically to preserve those layouts.
- Android uses a more complex AArch64 ILP32 guest/host arrangement because modern Android is 64-bit.
- Vita is already a 32-bit ARM platform, so the first choice is a direct native ARM build rather than Android's guest/host architecture.

## Intended Vita layers

```text
Halo game/source code
        |
        +-- XDK compatibility declarations (upstream)
        |
        +-- shared platform-compatible code
        |
        +-- port/vita/
              |
              +-- startup / main
              +-- filesystem
              +-- timing
              +-- threads
              +-- input
              +-- audio
              +-- networking (later)
              +-- renderer glue
                       |
                       v
                  upstream d3d8_gl
                  nv2a_vsh / nv2a_psh
                       |
                       v
                  Vita-compatible GL
                       |
                       v
                     vitaGL
                       |
                       v
                     SceGxm
```

## Primary reference order

When implementing a platform function:

1. inspect `port/linux/` for the clean native implementation;
2. inspect `port/android/` for ARM/ABI/GLES-specific fixes;
3. implement the Vita equivalent under `port/vita/`;
4. modify shared code only if the platform boundary cannot reasonably contain the difference.

## Renderer principle

Do not recreate Halo's materials/shaders manually. The upstream renderer already translates Xbox NV2A/D3D8 state and shaders into GL/GLSL. Preserve that translation pipeline and adapt its GL backend to Vita limitations.

## Runtime shader principle

The desired path is generated Halo GLSL -> vitaGL GLSL translator/runtime shader compiler -> Vita GPU program. If a generated construct is unsupported, simplify or transform that construct at the Halo translator boundary rather than replacing every shader by hand.

## Data layout principle

Never assume that a successful compiler conversion preserves Halo's binary layout. Validate:

- `sizeof(long)` and pointer width assumptions;
- `wchar_t` width;
- struct packing/alignment;
- unions/small-struct returns;
- floating-point behavior where lockstep/game logic depends on it;
- any pointer-as-32-bit-address behavior.

Add compile-time static assertions for critical layouts whenever a known expected size is available.
