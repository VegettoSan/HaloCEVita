#!/usr/bin/env python3
"""Reject a return to the staged ui.map lifecycle in the original-runtime build."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / "port/vita/src/main.c").read_text(encoding="utf-8")
ui = (root / "port/vita/src/halo_ui_original.c").read_text(encoding="utf-8")
game = (root / "source/game/game.c").read_text(encoding="utf-8")
original_main = (root / "source/main/main.c").read_text(encoding="utf-8")

required_ui = [
    "#undef HALO_VITA_MENU_BRINGUP",
    "main_menu_load();",
    "main_pregame_render();",
    "render_frame_present(NULL, NULL);",
    "tag_files_open();",
]
for token in required_ui:
    assert token in ui, f"original UI owner missing {token!r}"

original_branch = main.split("#ifdef HALO_VITA_ORIGINAL_RUNTIME", 2)[-1]
assert "halo_vita_original_main_menu_load()" in main
assert "halo_vita_original_render_menu_frame()" in main
assert "handing ui.map to original main_menu_load" in main

# The retail ownership chain must still exist in the imported decomp source.
assert "main_load_ui_scenario(FALSE);" in original_main
assert "game_initialize_for_new_map();" in original_main
for token in (
    "rasterizer_initialize_for_new_map();",
    "interface_initialize_for_new_map();",
    "scenario_initialize_for_new_map();",
    "objects_initialize_for_new_map();",
    "render_initialize_for_new_map();",
    "structures_initialize_for_new_map();",
    "director_initialize_for_new_map();",
    "observer_initialize_for_new_map();",
    "sound_initialize_for_new_map();",
    "cinematic_initialize_for_new_map();",
    "hs_initialize_for_new_map();",
    "ui_widgets_safe_to_load(TRUE);",
):
    assert token in game, f"decomp new-map lifecycle missing {token!r}"

# In the native original-runtime branch the old staged mount/partial renderer
# initialization must be below the #else recovery path, not the shipping path.
start = main.index("#ifdef HALO_VITA_ORIGINAL_RUNTIME\n\t/* The retail Xbox path")
end = main.index("#else", start)
shipping = main[start:end]
for forbidden in (
    "vita_cache_probe(",
    "halo_vita_renderer_initialize(",
    "halo_vita_menu_root_checkpoint(",
    "halo_vita_ui_activate_main_menu_state(",
):
    assert forbidden not in shipping, f"staged owner leaked into original runtime: {forbidden}"

print("PASS: original-runtime ui.map uses Halo main_menu_load/full new-map lifecycle; staged owners remain recovery-only")
