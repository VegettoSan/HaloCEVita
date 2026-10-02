/* Vita-only translation-unit wrapper around Halo's original
 * ui_widget_event_handler_functions.c.
 *
 * Keep the original source as the sole owner of handler semantics and static
 * state. Rename only its public dispatcher while it is included, then expose
 * the normal symbol below with additional offline-shell handlers authored by
 * retail ui.map. Every case below calls the original Halo function; this file
 * does not recreate menu behavior.
 *
 * System Link availability is reported truthfully by the Vita platform layer.
 * The split-screen entry path is allowed to prove exactly which original local
 * network-game owners are required; socket/system-link handlers stay blocked.
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
    case 14:
        vita_log("[VITA UI HANDLER] original function index=14 clear multiplayer player joins");
        result = clear_multiplayer_player_joins(widget, event, widget_deleted);
        break;
    case 15:
        vita_log("[VITA UI HANDLER] original function index=15 join local multiplayer player");
        result = player_wants_to_join_multiplayer_game(widget, event, widget_deleted);
        break;
    case 18:
        vita_log("[VITA UI HANDLER] original function index=18 dispose server list");
        result = network_server_list_dispose(widget, event, widget_deleted);
        break;
    case 21:
        vita_log("[VITA UI HANDLER] original function index=21 initialize split-screen local game");
        result = split_screen_game_initialize(widget, event, widget_deleted);
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
