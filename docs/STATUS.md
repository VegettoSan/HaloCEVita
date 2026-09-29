# Project status

Last updated: 2026-09-28

Maximum demonstrated state: **LINKS**. No Vita or Vita3K runtime test has been performed.

Hardware installation feedback: the user reports 00.01 rejected at the end of VitaShell installation with `0x8010113D`. A012 corrects RGBA LiveArea images to opaque indexed PNG-8 in package **00.02**; negative/positive local verification passes. Corrected installation and boot remain unverified.

Status vocabulary: `COMPILES`, `LINKS`, `BOOTS`, `RENDERS`, `PLAYABLE`, `STABLE`, `BLOCKED`.

| Area | Status | Evidence / next requirement |
|---|---|---|
| Upstream source / independence | COMPILES | Existing independent import verified: 21714ac0860e9b9ca08fbdc8a1d620f1b8a03797; no reimport/history changes. |
| Entire configured game C source | COMPILES | 466/466 original configured C units compile for native ARM32 hard-float (A009); aggregate object retains 584 CRT/platform imports. Not a complete executable link. |
| Vita executable / packaging | LINKS | build/vita/HaloCE.elf, symbols/map, eboot.bin, HaloCE.vpk 00.02; root `python3 configure.py --release; ninja vita; ninja vita_vpk`. Verifier checks ELF/SELF/SFO/ZIP, actual core symbols and all indexed LiveArea PNGs. A012 hardware retry pending after 00.01 install rejection. |
| ABI / MSVC semantics | COMPILES | pointer32/long32/wchar16/enum32, signed char, 64-bit alignment8; native SDK ABI separated. Existing upstream asserts pass; cross-unit/function-pointer audit still incomplete (KI-009). |
| Real Halo core initialization | LINKS | Calls upstream cseries/debug heap/profile/data arrays/datum iterator/compacting memory pool/CRC. Runtime probes and assertions included; PASS not yet observed on Vita. |
| Full Xbox memory/cache arena | BLOCKED | No Vita placed XPhysicalAlloc or cache/resource rebasing implementation; KI-007. |
| Filesystem / maps | LINKS | Native writable root/log/save directory; case-insensitive ui.map/a10.map detection invokes actual Xbox v5 header verifier. Tags/decompression/full Xbox filesystem/saves pending. |
| Logging | LINKS | Appends ux0:data/HaloCE/debug.txt; stable milestones 001-009,011-012; 010 withheld until full main can run. No console log received yet. |
| Timing | LINKS | Native microsecond process clock backs QueryPerformanceCounter/Frequency/GetTickCount and Halo profiler. Hardware timing unverified. |
| Threads / synchronization | LINKS | Native SDK pthread create/join/mutex probe linked; game XDK handles/APCs/thread services not integrated. |
| Input | LINKS | Native controls and typed XInputOpen/GetState/Close bridge; sticks, A/B/X/Y, L/R triggers, D-pad, Start/Back mapping. White/Black/L3/R3 intentionally unassigned. Hardware ergonomics pending. |
| Audio | COMPILES | Installed SDL3 3.4.12 has VITAAUD driver; upstream dsound_sdl.c compiles. VPK links a paused audio-stream availability probe. Game mixing/playback not linked or heard. |
| vitaGL / diagnostic display | LINKS | 960x544 clear/authored diagnostic text and NV2A-translated synthetic triangle test; no graphics observed yet. |
| Dynamic vertex / pixel translators | LINKS | Original operations/combiner translation retained; guarded GLSL1.20 dialect. Shader sources/status/log dumps on console; actual compile/link acceptance pending. |
| Full D3D8 GL backend / textures | COMPILES | d3d8_gl.c and xbox_textures.c compile with native GCC against upstream desktop GL declarations, not a completed Vita binding. 108 APIs inventoried; full renderer not linked. |
| Framebuffer copy fallback | LINKS | GPU FBO/blit path adapted from Android; tiny known-color readback probe included. Runtime PASS not observed. |
| Cubemaps / 2D retail textures | BLOCKED | Header APIs present; formats/swizzling/sampling need real renderer/tag integration and tests. |
| 3D textures | BLOCKED | No installed 3D upload API; sampler3D generation explicitly rejects. A011: campaign c10/c20 contain six volume bitmaps each, ui five; runtime sampling/a10 usage unknown. |
| Visibility / lens flares | BLOCKED | Query APIs exported; exact semantics untested. Android atomics not copied. |
| Main menu / campaign a10 | BLOCKED | Full main not called; memory, XAPI and renderer integration pending. No retail draw/tag load/gameplay. |
| Saves / networking | BLOCKED | Sources preserved; full platform integration deferred until single-player bring-up. |
| 30 FPS / stability | BLOCKED | Probe has 33,333us frame budget only; no gameplay or measured FPS/stability evidence. |

Console test instructions, final package SHA-256 and crash-symbol commands: [BUILD.md](BUILD.md). Attempt history is append-only in [ATTEMPTS.md](ATTEMPTS.md).
