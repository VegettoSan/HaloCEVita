# Decomp / existing Vita port / HaloCEVita platform audit

2026-10-03. Source authority: HaloCEVita main9439745 before this work;
`cybersecurity/halo-ce-universal`601a4c1f506662dcd93d79105e3fe69410397a87;
`BirchWoodGod/halo-ce-vita`309b9deeb8f4e5b5155ca1e187c81e4ae207d1f2.
The imported Halo baseline remains unchanged; these references are not a merge.

## First important divergence and evidence

**HECHO COMPROBADO:** upstream shell_xbox enters shell_initialize/main_loop.
The donor invokes that original entry on a dedicated16MiB thread. Our native
entry used the SDK startup thread and a limited outer menu loop, although map,
game-state, widget, sound and rasterizer owners are original. The last supplied
hardware evidence is00.41's serialized child-list abort (A129), addressed by
00.42. There is no fresh00.42 console log that proves another fault.

The reconstructed original code expects compiled pointers to be directly
readable. The donor performs a schema-guided relocation before original tags/BSP
consumers. HaloCEVita has a checked movable arena and typed accessor translation;
it already adapts original text/font/widget/keyboard consumers. Remaining direct
mouth-data access is confirmed in original sound_definitions.c. A host protected
serialized page reproduces its fault; the native typed accessor preserves the
original byte selection. This is source/host evidence, not a new console crash.

**HECHO COMPROBADO:** our menu loop called process_ui_widgets before its clock
update and input_frame_end before render/present. Original main_loop collects
input/events, updates time, processes widgets, renders/presents, then closes the
input frame.00.43 restores that ordering in the existing original menu owners.
It does not replace the widgets, events or draw implementation.

**HIPÓTESIS:** the corrected order may improve UI effects and input timing.
No white-panel, placement, FPS or complete Main Menu fix is claimed from it.
The dedicated stack avoids depending on the unconfigured startup stack; this
is not a diagnosis of any prior menu data-abort.

## A/B/C comparison and integration choices

| System | Original / donor implementation | HaloCEVita action and remaining boundary |
| --- | --- | --- |
| Process/bootstrap | Original shell/main; donor large engine thread | Reuse checked16MiB thread pattern, GPL attribution. Same thread owns engine, GL and cleanup; create/start/join/delete failures are explicit. Extra16MiB user memory, unrestricted affinity. Native CI286 passed; console budget pending. |
| Shell/game initialization | cseries, platform, errors, tags, math, game state, rasterizer, input, sound; main_loop initializes game | Existing native adapters retain those owners but split startup; game_initialize occurs before main_menu_load. Full original shell/main_loop/teardown remains an explicit next boundary. Do not call main_loop after game_initialize without moving that owner. |
| Map/cache/decompression | Original six Z: slots, cache-copy worker, queued reads; donor retains originals | Already restored in00.40. Keep original workers/native I/O; do not import another cache manager. Actual original worker reproduces supplied ui and bloodgulch logical bytes exactly. |
| Tags/endian/alignment | Xbox little-endian32-bit fields; donor generated typed relocation includes raw model/BSP pointers | Keep current checked tag/block/data/reference accessor model and ARM layout gates. Add mouth-data accessor; external audio sample addresses remain opaque. A whole donor schema import is not required by this fix and would change the currently accepted pointer contract. Model/BSP raw-pointer coverage remains open for world activation. |
| Game-state/allocators | Original placed/top-down physical allocations; donor112MiB movable arena with save-location policy | Existing96MiB checked Xbox-offset arena already boots. Retain it and original game-state allocation. Do not copy donor memory sizes or save-address pinning without a world/save budget and migration test. |
| Threads/events/APCs | Original Linux pthread handles/TLS completion plus donor native host threads | Existing event/mutex/suspended-thread/issuing-thread completion contracts retained. Add dedicated engine owner only; no cache-worker scheduling substitute. |
| Timers/synchronization | Performance counter and monotonic deadlines, native relative delay | Existing checked native deadline and original frame-clock functions retained. Restore clock-before-widgets and input-end-after-present. Actual C frame transaction and lost-root cleanup PASS. |
| Filesystem/storage | Xbox drive mapping, POSIX files, saves, enumeration/case folding | Existing general Vita/Xbox API adapter and POSIX filesystem retained; original cache owns preallocation/publication. No proprietary files copied or packaged. |
| Input | Original XInput/input abstraction/events; donor remaps D-pad in gameplay and exposes settings | Existing original controller/menu event path retained. Donor gameplay ergonomics/settings are not imported into authored menu behavior. Gameplay control mapping remains a later hardware task. |
| Audio/streaming | Original sound/cache manager with shared DirectSound/SDL backend; donor native host services | Existing original queued resource/mixer/consumer completion retained. Inline mouth envelope now uses tag_data_get_pointer. No external sample rebasing or new audio engine. Spatial/world and sustained audio acceptance remain open. |
| Network/modules/ARM CRT | Donor SceNet module/socket boundary and ARM floating-point environment | Already attributed/integrated from donor5ceb8e8. Preserve these source owners. Latest donor additional settings/network tracing are not dependencies of first menu draw; LAN/world acceptance remains open. |
| Movies/configuration | Donor AvPlayer MP4 adapter and settings/env-file panel | MP4 playback is separable but original Bink is currently an explicit unavailable capability. Converting user movies or importing a settings UI is outside first-menu initialization. Defaults that alter authored widescreen/layout/quality are not adopted. |
| Dirty-memory/GPU resource lifetime | Donor explicit-write page generations for its GXM renderer | Do not replace vitaGL's bounded exact CPU snapshots with donor's incomplete game-write tracking. It would miss dynamic font/CPU edits. |
| Renderer/draw | Donor direct GXM/Cg; our original D3D8/NV2A -> GL -> vitaGL | Preserve d3d8_gl/resources/translators, authored coordinates/alpha/topology, texture state and single original Present owner. No donor renderer, binary or assets copied. |
| Logging/error handling | Donor asynchronous ring/previous-session log/optional watchdog | Existing bounded milestone logging and explicit failure remain; no deliberate watchdog crash or per-frame log flood added. Matching final ELF/map are required for console dumps. |

## Validation and what the new test package establishes

A132: actual bootstrap C host contract plus CI286 native ELF/SELF/VPK and
publication PASS on cdd23ce. A133: all33 required host commands PASS, including
actual original frame transaction and five negative serialized text/font/mouth
consumers; supplied maps are unchanged and original decompressor output hashes
match the00.40 audit. Final combined native CI and hardware acceptance must be
recorded separately. No Vita or Vita3K session is attached here.

**PROPUESTA / next hardware gate:** install the exact final00.43 package;
observe engine-thread memory log, original cache open/BSP/language, Main Menu root,
first original draw/present, navigation/profile/keyboard and music. Return that
binary's logs/core before identifying another runtime cause. A screenshot or
bitmap-only output does not accept the complete original menu or gameplay.
