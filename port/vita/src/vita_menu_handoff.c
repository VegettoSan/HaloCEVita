/* Transfer ownership from the special ui.map bring-up to Halo's normal
 * scenario_tags_load/game_load lifetime. The staged menu initialized only a
 * narrow subset of per-map systems, so calling game_dispose_from_old_map()
 * here would dispose owners that never received initialize_for_new_map().
 * Retire exactly the staged owners, keep process-lifetime game/rasterizer
 * systems alive, then release the manual tag/resource mount before a10 is read.
 */
#include "cseries/cseries.h"
#include "cache/cache_files.h"
#include "cache/texture_cache.h"
#include "effects/decals.h"
#include "cutscene/cinematics.h"
#include "game/game.h"
#include "interface/ui_widget.h"
#include "main/main.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "halo_vita_cache.h"
#include "vita_runtime.h"

void texture_cache_close(void);
void players_dispose_from_old_map(void);

/* main.c keeps this retail ABI structure private. The Vita transition boundary
 * needs the same public game_options_new/game_load contract without copying
 * main_new_map or inventing gameplay state. Keep the layout assertion beside
 * the local declaration so divergence is a build failure. */
struct vita_game_options
{
    unsigned long flags;
    short code_version;
    short difficulty;
    unsigned long random_seed;
    char map_name[256];
};
typedef char vita_game_options_size_assert[
    sizeof(struct vita_game_options) == 0x10C ? 1 : -1];

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

int halo_vita_main_pump_deferred_map_change(void)
{
    static char missing_map[256];
    const char *requested = main_get_map_name();
    struct vita_game_options options;
    struct scenario *scenario;

    /* The staged shell starts with main_globals.soloplayer_map_name empty.
     * Original UI handlers populate it through main_set_map_name only when a
     * real solo map has been selected. Do not reinterpret arbitrary strings as
     * a transition request. */
    if (game_in_progress() || !requested || !requested[0] ||
        main_get_current_solo_level() == NONE)
        return 0;

    /* Validation is intentionally one-shot per rejected map name. The staged
     * shell pumps this function every frame; repeating full source/cache
     * diagnostics at 30 Hz would hide the first useful failure in debug.txt. */
    if (!csstrcmp(missing_map, requested))
        return 0;

    /* Retail does not open the compressed DVD/source map as the active cache.
     * First ensure the scenario has a committed z:\\cacheNNN equivalent. Vita's
     * precache is synchronous, but preserves slot selection, source identity,
     * full zlib validation and header-last publication. Only then may the menu
     * be released and scenario_tags_load consume the logical cache. */
    if (!cache_files_precache_map_loaded(requested)) {
        vita_log("[VITA MAP] Campaign source selected; precache required before handoff: %s", requested);
        if (!cache_files_precache_map_begin(requested, TRUE) ||
            !cache_files_precache_map_loaded(requested)) {
            csstrncpy(missing_map, requested, NUMBEROF(missing_map) - 1);
            missing_map[NUMBEROF(missing_map) - 1] = 0;
            vita_log("[VITA MAP] Campaign precache FAILED/not committed: %s", requested);
            return 0;
        }
        vita_log("[VITA MAP] Campaign precache committed and validated: %s", requested);
    }
    missing_map[0] = 0;

    game_options_new((struct game_options *)&options);
    csstrncpy(options.map_name, requested, NUMBEROF(options.map_name) - 1);
    options.map_name[NUMBEROF(options.map_name) - 1] = 0;
    options.difficulty = main_get_difficulty();
    vita_log("[VITA MAP] original solo scenario activation begin: map=%s difficulty=%d",
        options.map_name, (int)options.difficulty);

    if (!halo_vita_ui_runtime_release_staged_map())
        return -1;

    game_connection_set(_game_connection_local);
    game_precache_new_map(options.map_name, TRUE);
    game_unload();

    /* This checkpoint intentionally stops at the original game_load boundary:
     * scenario_load -> scenario_tags_load -> first structure BSP. The next
     * milestone will call game_initialize_for_new_map and create the local
     * player through main_new_map-equivalent original ownership. */
    if (!game_load((struct game_options *)&options)) {
        vita_log("[VITA MAP] original game_load FAILED for %s", options.map_name);
        return -1;
    }

    scenario = global_scenario_try_and_get();
    if (!scenario || global_structure_bsp_index == NONE ||
        scenario->structure_bsp_references.count <= 0) {
        vita_log("[VITA MAP] game_load returned without active scenario/BSP: scenario=%p bsp=%d refs=%ld",
            scenario, (int)global_structure_bsp_index,
            scenario ? scenario->structure_bsp_references.count : -1L);
        return -1;
    }

    vita_log("[VITA MAP] ORIGINAL SCENARIO ACTIVE: datum=%08lx type=%d BSP=%d/%ld map=%s",
        (unsigned long)global_scenario_index, (int)scenario->type,
        (int)global_structure_bsp_index, scenario->structure_bsp_references.count,
        options.map_name);
    return 1;
}
