/* Transfer ownership from the special ui.map bring-up to Halo's normal
 * scenario_tags_load/game_load lifetime. The staged menu initialized only a
 * narrow subset of per-map systems, so calling game_dispose_from_old_map()
 * here would dispose owners that never received initialize_for_new_map().
 * Retire exactly the staged owners, keep process-lifetime game/rasterizer
 * systems alive, then release the manual tag/resource mount before a10 is read.
 */
#include "cseries/cseries.h"
#include "cache/texture_cache.h"
#include "effects/decals.h"
#include "cutscene/cinematics.h"
#include "game/game.h"
#include "interface/ui_widget.h"
#include "scenario/scenario.h"
#include "halo_vita_cache.h"
#include "vita_runtime.h"

void texture_cache_close(void);
void players_dispose_from_old_map(void);

int halo_vita_ui_runtime_release_staged_map(void)
{
    int restored;

    main_menu_active(FALSE);
    ui_widgets_inhibit_processing(TRUE);
    ui_widgets_close_all();
    if (halo_vita_ui_original_root_ready()) {
        vita_log("[VITA MAP] staged ui.map release blocked: active widget root survived close_all");
        ui_widgets_inhibit_processing(FALSE);
        return 0;
    }

#ifdef HALO_VITA_MENU_AUDIO
    if (!halo_vita_menu_audio_release_for_game()) {
        ui_widgets_inhibit_processing(FALSE);
        return 0;
    }
#endif

    /* These are exactly the per-map owners established by
     * halo_vita_renderer_initialize() for the manual menu path. */
    decals_dispose_from_old_map();
    cinematic_dispose_from_old_map();
    players_dispose_from_old_map();
    game_time_dispose_from_old_map();
    texture_cache_close();

    /* The manual ui.map was never scenario_load()'s ownership, so do not call
     * scenario_unload()/scenario_tags_unload(): those would deregister GPU
     * directories that this mount never registered. Clear only the scenario
     * globals supplied by halo_menu_tags.c, then retire its typed journal. */
    global_scenario_index = NONE;
    global_structure_bsp_index = NONE;
    if (scenario_globals)
        scenario_globals->structure_bsp_index = NONE;
    global_scenario = NULL;
    global_game_globals = NULL;
    global_collision_bsp = NULL;
    global_bsp3d = NULL;

    restored = halo_vita_cache_unmount_menu();
    if (!restored) {
        /* A live compiled cache may contain legitimate runtime bitmap/sound
         * bookkeeping. The unmount has already restored/freed pointer journals
         * and detached cache_file_globals; this image is retiring and will be
         * overwritten by scenario_tags_load(), so a full-image CRC mismatch is
         * diagnostic here rather than a reason to keep stale ownership alive. */
        vita_log("[VITA MAP] retiring ui.map did not match cold CRC after runtime use; mount ownership still released");
    }
    vita_cache_resource_unbind();
    ui_widgets_inhibit_processing(FALSE);
    vita_log("[VITA MAP] staged ui.map ownership released; original scenario loader may reuse tag arena");
    return 1;
}
