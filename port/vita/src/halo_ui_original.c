/* Native input boundary only. Halo owns the complete UI frame. */
#include "../../../source/interface/ui_widget.c"
#include "input/input.h"
#include "input/input_abstraction.h"
#include "interface/event_manager.h"
int halo_vita_ui_original_root_ready(void)
{ return widget_globals.initialized && widget_globals.active_widgets[0] != NULL; }

int halo_vita_ui_activate_main_menu_state(void)
{
    if (!widget_globals.initialized || !widget_globals.active_widgets[0])
        return FALSE;
    main_menu_active(TRUE);
    return main_menu_is_active();
}

void halo_vita_ui_process_shell_frame(void)
{
    input_frame_begin();
    input_update();
    input_abstraction_update();
    event_manager_update();
    process_ui_widgets();
    input_frame_end();
}
