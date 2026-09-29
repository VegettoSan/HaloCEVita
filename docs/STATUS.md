# Project status

Last updated: 2026-09-28

Status vocabulary: `NOT STARTED`, `INVESTIGATING`, `COMPILES`, `LINKS`, `BOOTS`, `RENDERS`, `PLAYABLE`, `STABLE`, `BLOCKED`.

| Area | Status | Evidence / next requirement |
|---|---|---|
| Independent Vita repository | NOT STARTED | Repository scaffold/documentation exists; no Vita code has compiled yet. |
| Upstream source import | NOT STARTED | Import `cybersecurity/halo-ce-universal` without its `.git` history, then record exact SHA in `docs/UPSTREAM.md`. |
| Vita build target | NOT STARTED | Add Vita target to build system; generate `eboot.bin`. |
| Vita VPK packaging | NOT STARTED | Produce installable `HaloCE.vpk`. |
| 32-bit ABI compatibility | INVESTIGATING | Vita is 32-bit ARM, favorable for Halo's 32-bit layouts; compiler/layout assumptions still need validation. |
| C runtime / libc boundary | NOT STARTED | Audit Linux POSIX layer vs Vita libc/newlib availability. |
| Filesystem | NOT STARTED | Target `ux0:data/HaloCE/`; case-insensitive behavior may require compatibility layer. |
| Logging | NOT STARTED | Create `ux0:data/HaloCE/debug.txt` early. |
| Threading | NOT STARTED | Map pthread/platform assumptions to Vita. |
| Timing | NOT STARTED | Map SDL/platform timing calls. |
| Input | NOT STARTED | Native Vita controls -> Xbox control semantics. |
| Audio | NOT STARTED | Evaluate SDL3 Vita support vs direct Vita audio path. |
| Networking | NOT STARTED | Defer until single-player boot/render path works. |
| vitaGL initialization | NOT STARTED | Create context and clear/present screen. |
| D3D8 -> GL renderer | INVESTIGATING | Upstream renderer is reusable conceptually; Vita compatibility audit required. |
| Dynamic vertex shaders | INVESTIGATING | Prefer upstream NV2A->GLSL translator + vitaGL shader path. |
| Dynamic pixel shaders | INVESTIGATING | Same strategy; validate generated GLSL subset. |
| Texture 2D | NOT STARTED | Audit formats/swizzles/compression. |
| Cubemaps | NOT STARTED | Audit vitaGL support and Halo usage. |
| 3D textures | BLOCKED | Known risk area until actual Halo usage and vitaGL behavior are tested. |
| Framebuffers/render-to-texture | INVESTIGATING | vitaGL exposes framebuffer/blit APIs; verify semantics and performance. |
| Visibility queries/lens flares | NOT STARTED | May need fallback depending on query support. |
| Main menu | NOT STARTED | First meaningful graphics milestone. |
| Campaign `a10` | NOT STARTED | First gameplay milestone after menu. |
| Saves | NOT STARTED | Preserve Xbox-style layout while mapping paths to Vita. |
| 30 FPS target | NOT STARTED | Optimize only after correct rendering/gameplay. |
