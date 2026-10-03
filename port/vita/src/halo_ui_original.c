/* Native input boundary only. Halo owns the complete UI frame. */
#ifdef HALO_VITA_ORIGINAL_RUNTIME
/* The original-runtime target must execute ui_widget.c's retail branches.
 * HALO_VITA_MENU_BRINGUP belongs to the native harness, not to Halo's UI
 * implementation; leaving it visible here selects the old staged audio/UI
 * substitutions even after the complete game lifetime has been linked. */
#ifdef HALO_VITA_MENU_BRINGUP
#undef HALO_VITA_MENU_BRINGUP
#endif
#endif
#include "../../../source/interface/ui_widget.c"
#include "input/input.h"
#include "input/input_abstraction.h"
#include "interface/event_manager.h"
#include "main/main.h"
#include "render/render.h"
#include "cache/cache_files.h"
#include "vita_runtime.h"

int halo_vita_ui_original_root_ready(void)
{ return widget_globals.initialized && widget_globals.active_widgets[0] != NULL; }

int halo_vita_ui_activate_main_menu_state(void)
{
    if (!widget_globals.initialized || !widget_globals.active_widgets[0])
        return FALSE;
    main_menu_active(TRUE);
    return main_menu_is_active();
}

#ifdef HALO_VITA_ORIGINAL_RUNTIME
int halo_vita_original_main_menu_load(void)
{
    static boolean tag_files_opened;

    /* shell_initialize() opens the tag/cache service before maps are loaded.
     * Vita owns the physical arena/context separately, but the storage owner is
     * still the original tag_files_open -> cache_files_initialize contract. */
    if (!tag_files_opened) {
        tag_files_open();
        tag_files_opened = TRUE;
        vita_log("[VITA ORIGINAL] tag_files_open/cache_files_initialize PASS");
    }

    vita_log("[VITA ORIGINAL] main_menu_load begin: original ui scenario/new-map lifecycle owns initialization");
    main_menu_load();

    if (!halo_vita_ui_original_root_ready() || !main_menu_is_active()) {
        vita_log("MAIN MENU BLOCKED: original main_menu_load returned without active retail root");
        return FALSE;
    }

    vita_log("[VITA ORIGINAL] main_menu_load PASS: main_load_ui_scenario -> main_new_map -> game_initialize_for_new_map -> main_screen_shell_load");
    return TRUE;
}

int halo_vita_original_render_menu_frame(void)
{
    if (!halo_vita_ui_original_root_ready())
        return FALSE;
    halo_vita_main_render_time_update();
    main_pregame_render();
    render_frame_present(NULL, NULL);
    return TRUE;
}
#endif

void halo_vita_ui_process_shell_frame(void)
{
    input_frame_begin();
    input_update();
    input_abstraction_update();
    event_manager_update();
    process_ui_widgets();
    input_frame_end();
}

/* The staged menu handoff shares ui_widget.c's real widget globals in this
 * translation unit. Keep it at the UI ownership boundary rather than teaching
 * game/scenario code about the special bring-up mount. */
#include "vita_menu_handoff.c"
