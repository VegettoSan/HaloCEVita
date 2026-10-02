/* Vita-only translation-unit wrapper around Halo's original
 * ui_widget_event_handler_functions.c.
 *
 * Keep the original source as the sole owner of handler semantics and static
 * state. Rename only its public dispatcher while it is included, then expose
 * the normal symbol below with two additional offline-shell handlers that are
 * authored by retail ui.map and whose dependencies are part of the staged
 * single-player/menu closure.
 *
 * Do not add networking handlers here until their native transport owners are
 * implemented. System Link availability is reported truthfully by the Vita
 * platform layer and network-only UI should remain disabled meanwhile.
 */
#define ui_widget_event_handler_function_invoke halo_vita_staged_event_handler_function_invoke
#include "../../../source/interface/ui_widget_event_handler_functions.c"
#undef ui_widget_event_handler_function_invoke

#ifdef HALO_VITA
boolean ui_widget_event_handler_function_invoke(
    struct widget_instance *widget,
    struct event_record *event,
    word function_index,
    boolean *widget_deleted)
{
    boolean result;

    switch (function_index)
    {
    case 5:
        vita_log("[VITA UI HANDLER] original function index=5 initialize sp level list solo");
        result = solo_level_initialize_list_single_player(widget, event, widget_deleted);
        break;
    case 73:
        vita_log("[VITA UI HANDLER] original function index=73 main menu switch to solo game");
        result = switch_from_main_menu_to_single_player(widget, event, widget_deleted);
        break;
    default:
        return halo_vita_staged_event_handler_function_invoke(
            widget, event, function_index, widget_deleted);
    }

    if (!result)
        vita_log("MAIN MENU BLOCKED: event handler '%s' failed",
            event_handler_function_list.names[(short)function_index]);
    return result;
}
#endif
