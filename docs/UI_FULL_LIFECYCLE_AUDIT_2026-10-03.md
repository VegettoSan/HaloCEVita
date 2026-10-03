# ui.map full original lifecycle audit — 2026-10-03

## Trigger

Hardware build 00.38 (`ade165ab4fe849667a9e40c7fb2a887765e401d3`) still renders the Main Menu with the same misplaced elements/text and white rectangular regions reported in earlier staged builds. The user also reproduced the established D-pad behavior where extended keyboard/menu navigation can make the white regions visually settle/disappear.

The supplied 00.38 log proves that the new NV2A mobile `clip_position` reconstruction is running, while the visible result is unchanged. Therefore the precise mobile clip-position omission was a real backend divergence but is **not** the root cause of these UI symptoms on hardware.

## 00.38 evidence retained

- Startup identifies 00.38.
- `ui.map` is rebuilt/reused through the upstream-style `cache002.map` contract.
- The logical cache is 33,582,080 bytes, 983 tags, tag CRC `e22586e4`.
- The original Main Menu root becomes active and `main_pregame_render` produces real D3D8/NV2A draws.
- The first sampled DXT3 mask has authored transparent texels and reaches the shader with alpha 0 at the corner and alpha 118 at the sampled centre; native blending is `SRC_ALPHA / ONE_MINUS_SRC_ALPHA`.
- The 4x4 DXT1 resource drawn immediately before one UI mask is the intentional retail tag `ui\shell\bitmaps\white`, not corrupt filler.
- The newly generated vertex shaders contain `clip_position`, `clip_captured`, the pre-`rcc(r12.w)` capture and c[-38]/c[-37] reconstruction. No hardware visual change follows.

This keeps cache corruption, missing DXT alpha and the old clip-position fallback out of the active root-cause list for the reported rectangles/placement symptom.

## Source audit: 00.38 was still not using Halo's complete ui.map lifecycle

Primary authority inspected directly:

```
cybersecurity/halo-ce-universal
commit 80d30410c8db28f4008b92f4e012a1b046ece14e
```

The retail/decomp ownership chain is:

```
main_menu_load()
  -> main_load_ui_scenario(FALSE)
       -> game_precache_new_map("levels\\ui\\ui", TRUE)
       -> game_dispose_from_old_map()
       -> game_unload()
       -> game_engine_dispose()
       -> main_new_map(&options)
            -> input_flush()
            -> game_load(options)
                 -> scenario_load(options->map_name)
                      -> scenario_tags_load()
                      -> scenario_switch_structure_bsp(0)
            -> game_initialize_for_new_map()
            -> create_local_players()
            -> game_time_start()
            -> game_initial_pulse()
  -> main_screen_shell_load()
  -> main_menu_precache_resources()
  -> update-server/game-time/HS menu reset
```

`game_initialize_for_new_map()` is not a small UI helper. It initializes the complete per-map owner graph, including rasterizer, game-state map data, game time, interface, allegiances, players, scenario, objects, render, structures, breakable surfaces, decals, director, observer, effects, particles, sound, physics, game engine, player control, rumble, AI, console, editor, cinematics, HS and recorded animations. It finally marks the game active, places objects and calls `ui_widgets_safe_to_load(TRUE)`.

### Divergence in 00.38

00.38 did **not** call that ownership chain for ui.map. It instead:

1. prepared/read `cache002.map` through `vita_cache_probe()`;
2. mounted/relocated a selected UI-facing subset;
3. manually initialized selected per-map systems in `halo_vita_renderer_initialize()`;
4. called `main_screen_shell_load()` through `halo_vita_menu_root_checkpoint()`;
5. rendered through the original pregame renderer.

This staged path was useful for bring-up, but it is not equivalent to Halo's normal ui scenario lifecycle. In particular, the runtime log explicitly reported the staged-world boundary.

## Why the complete original path is now technically possible

The Vita backend has advanced beyond the reason the staged mount originally existed:

- `cache_files_precache_*` now preserves Halo's retail map/cache-slot contract while adapting storage to `ux0:data/HaloCE`.
- public `cache_file_open/read` use the committed uncompressed cache slot.
- `cache_file_read()` already detects the original `scenario_tags_load()` read into the Vita Xbox arena and calls the Vita compiled-pointer activation boundary.
- that activation rebases the tag header/directory, tag names/root pointers, scenario top-level metadata and BSP ownership required by original scenario loading.
- structure BSP payload reads have their own Vita relocation/compiled-GPU registration boundary.

Therefore the platform no longer needs to bypass `scenario_tags_load()` merely to reach the menu.

## Applied change

### 1. Halo owns menu loading

The original-runtime branch in `port/vita/src/main.c` no longer calls the staged `vita_cache_probe()`/partial renderer initialization path. It now calls `halo_vita_original_main_menu_load()`, whose game-ABI implementation calls the real `main_menu_load()`.

Expected shipping chain is now:

```
Vita platform/context/arena
  -> original game_initialize()
  -> original main_menu_load()
  -> original ui scenario game_load/new-map initialization
  -> original main_screen_shell_load()
  -> original process_ui_widgets()
  -> original main_pregame_render()
  -> original render_frame_present()
```

Vita remains responsible only for filesystem/cache-slot adaptation, ARM pointer rebasing, Xbox resource-address adaptation, D3D8/NV2A translation and vitaGL presentation.

### 2. Staged UI compile branch removed from original ui_widget.c

`HALO_VITA_MENU_BRINGUP` belongs to the native test harness. Allowing it to remain defined while including original `ui_widget.c` selected old staged substitutions inside Halo itself (for example the special staged Main Menu audio path).

`halo_ui_original.c` now undefines `HALO_VITA_MENU_BRINGUP` before including original `ui_widget.c` when `HALO_VITA_ORIGINAL_RUNTIME` is enabled. This restores the original retail branches in the UI implementation without rewriting those functions.

The recovery/staged implementation remains in the tree but is no longer the owner of the original-runtime startup path.

### 3. Rendering owner

The original-runtime loop directly calls the original frame owner:

```
main_pregame_render();
render_frame_present(NULL, NULL);
```

It does not call `halo_vita_renderer_initialize()` after `main_menu_load()`, because doing so would duplicate per-map subsystem initialization already performed by `game_initialize_for_new_map()`.

### 4. Transition boundary

The old special Campaign handoff is staged-menu-specific. It is deliberately not executed from the new full-original UI loop. Campaign transition must next be connected to the original main-loop map-change owner rather than releasing a fake staged ui.map lifetime.

## Regression policy

`tools/vita_original_ui_lifecycle_regression.py` requires:

- original `main_menu_load()` ownership;
- original `main_pregame_render`/present ownership;
- the decomp's `game_initialize_for_new_map()` subsystem chain;
- no staged `vita_cache_probe`, `halo_vita_renderer_initialize`, `halo_vita_menu_root_checkpoint` or manual Main Menu state activation inside the shipping original-runtime branch.

The Vita workflow runs this check with the existing renderer/cache/audio regressions.

## Acceptance status

Source/CI success will prove only that the full original lifecycle compiles/links/packages. It does **not** prove the hardware UI is visually fixed.

The next Vita run must establish the first runtime boundary reached:

1. original cache precache/open;
2. original scenario tag-image activation;
3. original BSP activation;
4. `game_initialize_for_new_map()` completion;
5. `main_screen_shell_load()` root creation;
6. first original render/present;
7. visual placement/white-region comparison.

If the full original lifecycle reaches the same visible corruption, investigation returns below the widget layer to the exact pixel-combiner/render-state result. If it fixes or materially changes the UI, the staged partial lifecycle was the demonstrated cause/contributor.
