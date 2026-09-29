# Roadmap

The project advances by observable milestones. Do not skip directly to optimization or feature completeness.

## M0 — Reproducible source baseline

- Import current upstream source into this independent repository.
- Record exact upstream SHA.
- Confirm original Linux build still has a documented path outside Vita work if possible.
- Keep Vita work isolated under `port/vita/` and build scripts.

Exit criteria: source baseline recorded and repository builds/configures enough for Vita work to begin.

## M1 — Native Vita compilation

- Add Vita compiler/toolchain detection.
- Compile game units for 32-bit ARM.
- Resolve compiler ABI/layout errors deliberately rather than hiding them.
- Produce a linked Vita ELF/`eboot.bin`, initially with a minimal/null renderer if necessary.

Exit criteria: `build/vita/eboot.bin` links reproducibly.

## M2 — First installable boot

- Package `HaloCE.vpk`.
- Initialize Vita runtime.
- Initialize filesystem and logging.
- Launch on real Vita and/or Vita3K far enough to produce a startup log.

Exit criteria: installable VPK boots and writes deterministic milestones to `debug.txt`.

## M3 — Data discovery and Halo initialization

- Find `ux0:data/HaloCE/maps/`.
- Initialize Halo memory/game systems.
- Load enough retail data to reach UI initialization without graphics.

Exit criteria: logs prove game data is recognized and initialization progresses into UI/render startup.

## M4 — Graphics bootstrap

- Initialize vitaGL.
- Clear/present frame.
- Adapt minimal GL state path.
- Compile one known generated shader.

Exit criteria: visible frame and a successful Halo-originated draw path.

## M5 — Main menu

- Textures, shaders, blend/depth/stencil, vertex/index buffers.
- UI/HUD/menu rendering.
- Vita controls in menus.
- Basic audio if feasible.

Exit criteria: usable Halo CE main menu.

## M6 — First campaign level

- Load `a10`.
- BSP rendering.
- Models/animations.
- Player movement/look/actions.
- AI/gameplay loop.

Exit criteria: playable segment of Pillar of Autumn.

## M7 — Fidelity and stability

- Cubemaps/reflections.
- Transparent/multipass effects.
- Lightmaps.
- Lens flares/queries.
- Framebuffer effects.
- Saves.
- Audio completeness.

Exit criteria: repeatable campaign play without major rendering/gameplay blockers.

## M8 — Performance

- 30 FPS target.
- Memory/VRAM budgeting.
- Buffer sizing.
- Shader caching.
- Draw/state optimization.

Exit criteria: measured stable performance targets on representative campaign scenes.

## M9 — Optional systems

- Multiplayer/system link.
- VitaTV.
- Gyro.
- Quality-of-life settings.
