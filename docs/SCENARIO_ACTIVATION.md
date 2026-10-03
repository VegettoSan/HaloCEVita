# Native scenario activation on PS Vita

Status: groundwork in progress (2026-10-02 Bogotá)

This document records the boundary for reconnecting Halo's original map/scenario
lifetime after the V5 original-UI reintegration. It does not define a replacement
map loader or renderer.

## Required original flow

Keep the decomp call sequence:

`game_precache_new_map -> game_load -> scenario_load -> scenario_tags_load -> scenario_switch_structure_bsp -> game_initialize_for_new_map`

and later the normal `main_loop` update/render path. Vita adaptations belong at
platform/cache/address boundaries only.

## Verified facts

- `source/main/main.c` already owns Campaign/deferred map transitions. The
  current Vita test shell processes original UI frames but does not yet execute
  those private deferred-map handlers; duplicating them in a second router is
  not the target architecture.
- `scenario_tags_load` reads the complete compiled tag image through
  `cache_file_read`, then registers the compiled vertex/index descriptors.
- `scenario_structure_bsp_load` reads the referenced BSP payload through the
  same `cache_file_read` API and writes it at the Xbox-authored
  `scenario_structure_bsp_reference.base_address`.
- Retail `cache_files_windows.c` first copies DVD maps into fixed
  `z:\cacheNNN.map` files. This is not appropriate for Vita's supplied
  compressed Xbox-v5 maps.
- Vita already has a validated logical-range backend that can expose compressed
  maps without changing source map bytes.
- Real supplied retail maps use build `01.10.12.2276`; the January executable's
  build string is `01.01.14.2342`. The v5 format/header/range contract, not this
  informational build string, is the native compatibility gate.
- The supplied `ui.map` and `bloodgulch.map` both carry one BSP reference. Their
  BSP destinations are Xbox arena addresses outside the retained tag-data
  length, so they must be translated through the placed Vita arena and must not
  be treated as pointers inside the tag-data blob.

## Implemented groundwork

### Direct original cache API on Vita

`port/vita/src/halo_cache_windows_vita.c` now keeps the original public cache
API but renames the retail HDD/DVD implementation internally. On Vita:

- `levels\\a10\\a10` resolves to `ux0:data/HaloCE/maps/a10.map`;
- precache availability checks validate the direct file instead of copying it;
- `cache_file_open` binds the validated map to the existing logical resource
  stream;
- public `cache_file_read` remains owned by `vita_cache_bridge.c`;
- `cache_file_close` releases the logical stream;
- original source map bytes are never rewritten.

### Scenario/BSP typed preflight

When the original `scenario_tags_load` path performs its first tag-image read,
`vita_cache_bridge.c` now validates before completion is published:

- the Xbox-v5 tag directory and scenario datum;
- the scenario root span;
- the typed `structure_bsp_references` block;
- each BSP logical file range against the currently bound map;
- each BSP Xbox destination against the 96 MiB placed Vita arena;
- the expected `sbsp` reference group.

This stage is validation only; it deliberately does not mutate pointers yet.

## Next implementation boundary

The next code change must perform a transactional typed relocation for the
original scenario load. At minimum it must cover, before Halo dereferences them:

1. tag-header directory, vertex-buffer and index-buffer pointers;
2. tag-directory name/root pointers;
3. scenario top-level `tag_block`, `tag_data` and `tag_reference` fields using
   actual structure definitions/`offsetof`, not arbitrary 32-bit scanning;
4. `scenario_structure_bsp_reference.base_address` through
   `halo_vita_memory_address`;
5. the BSP cache header and `struct structure_bsp` top-level block/data fields;
6. nested structures only as their original consumers require them, with bounds
   checks and a rollback/fail-loud path.

Do not enable Campaign/new-map handoff until this relocation reaches the
original `game_initialize_for_new_map` consumers safely. A successful link or
preflight is not a gameplay claim.

## Explicit non-goals

- no custom map viewer;
- no hardcoded `a10` renderer;
- no arbitrary aligned-word rebasing;
- no asset conversion replacing `.map` semantics;
- no second UI/game state machine;
- no replacement of the original D3D8/NV2A renderer.
