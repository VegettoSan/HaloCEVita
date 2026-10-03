# Native scenario activation on PS Vita

Status: **scenario/BSP checkpoint wired; hardware verification pending** (2026-10-02/03 Bogotá)

This document records the boundary for reconnecting Halo's original map/scenario
lifetime after the V5 original-UI reintegration. It does not define a replacement
map loader, game state machine or renderer.

## Required original flow

Keep the decomp call sequence:

`game_precache_new_map -> game_load -> scenario_load -> scenario_tags_load -> scenario_switch_structure_bsp -> game_initialize_for_new_map`

and later the normal `main_loop` update/render path. Vita adaptations belong at
platform/cache/address boundaries only.

The current Vita checkpoint intentionally stops after the original `game_load`
has completed `scenario_load` and switched the first BSP. It does **not** yet
claim `game_initialize_for_new_map`, local-player creation or gameplay.

## Verified facts

- `source/main/main.c` owns Campaign/deferred map transitions in the decomp.
- The current staged Vita menu was mounted outside `game_load`; therefore its
  `game_globals` do not claim a loaded/active map and the retail private
  `main_change_map_name()` cannot be entered blindly.
- `scenario_tags_load` reads the complete compiled tag image through
  `cache_file_read`, then registers compiled vertex/index descriptors.
- `scenario_structure_bsp_load` reads the referenced BSP payload through the
  same `cache_file_read` API and writes it at the Xbox-authored placed address.
- Retail `cache_files_windows.c` first copies DVD maps into fixed
  `z:\cacheNNN.map` files. This is not appropriate for Vita's supplied
  compressed Xbox-v5 maps.
- Vita already has a validated logical-range backend that can expose compressed
  maps without changing source map bytes.
- Real supplied retail maps use build `01.10.12.2276`; the January executable's
  build string is `01.01.14.2342`. The v5 format/header/range contract, not this
  informational build string, is the native compatibility gate.
- Real map inspection confirms BSP destinations are Xbox placed-arena addresses,
  not offsets inside the retained tag blob. `bloodgulch.map`, for example,
  places its BSP at Xbox VA `0x81869800` and keeps its BSP vertex/lightmap
  descriptor directories inside that payload.
- Compiled vertex-buffer `Data` values are Xbox virtual addresses but the shared
  D3D8 backend stores/consumes registered vertex data as contiguous-memory
  physical offsets. Compiled index-buffer `Data` is consumed by `SetIndices` as
  a virtual `WORD *`. Those two descriptor classes therefore require different
  Vita address translations.

## Implemented Vita boundary

### 1. Direct original cache API

`port/vita/src/halo_cache_windows_vita.c` keeps Halo's original public cache API
but renames the retail HDD/DVD implementation internally. On Vita:

- `levels\\a10\\a10` resolves to `ux0:data/HaloCE/maps/a10.map`;
- precache availability validates the direct map instead of copying it;
- `cache_file_open` binds that map to the existing checked logical resource
  stream;
- public `cache_file_read` stays owned by `vita_cache_bridge.c`;
- `cache_file_close` releases the logical stream;
- original `.map` bytes are never rewritten.

Retail `01.10.12.2276` maps are accepted only after the Xbox-v5 structural
checks pass. The January build string is normalized only in the in-memory
compatibility view required by the original executable assertion.

### 2. Transactional tag/scenario relocation

When original `scenario_tags_load` performs its tag-image read,
`vita_cache_bridge.c` validates the raw Xbox-v5 index before publishing
completion, then relocates typed fields that the original loader immediately
consumes:

- tag-header directory and vertex/index descriptor directory pointers;
- tag-directory name/root pointers;
- scenario top-level `tag_block`, `tag_data` and `tag_reference` fields;
- `scenario_structure_bsp_reference.base_address` through the placed 96 MiB
  Vita arena;
- structure-BSP reference names and logical file ranges.

No arbitrary aligned-word scan is used.

### 3. BSP payload relocation

On the original BSP `cache_file_read`, before Halo observes completion, the
bridge validates the `sbsp` cache header and relocates:

- BSP root;
- vertex and lightmap-vertex descriptor directories;
- top-level `struct structure_bsp` blocks/data/references;
- leaf-map compiled blocks.

The payload remains the original map data copied to its original placed-arena
location, translated to the movable Vita arena.

### 4. Nested compiled-pointer access

Campaign tags contain many nested blocks beyond the scenario/BSP roots. Rather
than create hand-written Vita schemas for every tag type,
`port/vita/src/vita_tag_pointer.c` is connected at Halo's own original accessors:

- `tag_block_get_element_with_size`;
- `tag_data_get_pointer`;
- `verify_tag_reference`.

Only owners physically inside the active direct-map tag/BSP images participate.
Serialized Xbox addresses are translated against validated tag/BSP spans;
ordinary runtime/heap pointers pass through unchanged. Invalid compiled spans
fail loudly. This preserves the decomp traversal logic instead of replacing it.

### 5. Compiled D3D8 buffer descriptors

`halo_cache_windows_vita.c` now adapts the resource-registration boundary:

- tag/BSP vertex-buffer `Data`: Xbox VA -> placed-arena offset, then the original
  `IDirect3DVertexBuffer8_Register` contract runs with the movable Vita arena as
  its base;
- tag index-buffer `Data`: Xbox VA -> native Vita virtual pointer, matching
  `D3DDevice_SetIndices`, which stores it directly as `WORD *`;
- BSP lightmap vertex buffers follow the same vertex registration contract.

This change is deliberately below Halo's renderer. `d3d8_gl.c`, stream setup,
index drawing and authored map descriptors retain their original semantics.

### 6. Staged `ui.map` -> original map ownership handoff

The current Main Menu is a special bring-up mount, not a `game_load`-owned map.
Before Campaign can reuse the tag arena, `vita_menu_handoff.c` retires exactly
the per-map owners that the staged path initialized:

- original UI widgets/root;
- staged per-map sound/cache/classes state;
- decals;
- cinematics;
- players;
- game time;
- texture cache;
- manual `ui.map` relocation/resource mount.

It intentionally does **not** call full `game_dispose_from_old_map()`, because
that would dispose many owners which the staged menu never initialized for a
map. Process-lifetime systems created by `game_initialize()` remain alive for
the incoming original map.

A missing/invalid Campaign map is checked before this teardown, so the working
menu is retained rather than destroyed into a black screen.

### 7. Current original scenario checkpoint

After an original UI frame has committed a valid solo map name, the Vita loop
calls the narrow transition pump. It then uses original Halo functions to:

1. build `game_options`;
2. set local connection;
3. satisfy `game_precache_new_map` through the direct Vita backend;
4. call `game_unload` for the empty staged game-map lifetime;
5. call original `game_load`;
6. therefore execute original `scenario_load` -> `scenario_tags_load` -> first
   `scenario_switch_structure_bsp`;
7. require a valid `global_scenario` and `global_structure_bsp_index` before
   logging `ORIGINAL SCENARIO ACTIVE`.

Once this checkpoint succeeds, the staged menu renderer is suspended. The
process currently retains the loaded scenario/BSP for diagnosis instead of
pretending gameplay has begun.

## Hardware acceptance evidence still required

A build/link success is **not** scenario activation evidence. On real Vita, a
Campaign selection must produce logs showing, in order:

- direct `a10.map` open/bind;
- tag-image validation/relocation PASS;
- compiled tag GPU buffers registered;
- first BSP destination/read/relocation PASS;
- compiled BSP vertex/lightmap buffers registered;
- `ORIGINAL SCENARIO ACTIVE` with the expected solo scenario and BSP index.

Any fatal before that point should be treated as the next concrete boundary;
do not paper over it with a synthetic map state.

## Next implementation boundary after hardware PASS

Only after the scenario/BSP checkpoint is proven on hardware should the port
continue the remaining original `main_new_map` lifetime:

1. `game_initialize_for_new_map`;
2. create local player(s) using the original controller-selection path;
3. `game_time_start`;
4. `game_initial_pulse`;
5. enter the original game update/director/observer/render/present frame path.

That is the transition from **scenario active** to **gameplay initialization**.
It is intentionally not marked working yet.

## Explicit non-goals

- no custom map viewer;
- no hardcoded `a10` renderer;
- no arbitrary aligned-word rebasing;
- no asset conversion replacing `.map` semantics;
- no second gameplay state machine;
- no replacement of the original D3D8/NV2A renderer.