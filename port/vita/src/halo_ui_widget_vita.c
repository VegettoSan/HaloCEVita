/* Vita-only translation-unit wrapper around January's original ui_widget.c.
 *
 * Keep the original implementation byte-for-byte as the owner of widget
 * state, focus, event handlers and stack transitions.  The staged Vita shell
 * does not yet link the complete process_ui_widgets closure (A081: that pulls
 * 158 unrelated world/storage/runtime owners), but it still needs a safe way
 * to feed controller button edges into the exact original recursive widget
 * event processor.  Including the source here instead of compiling it as a
 * second translation unit lets this narrow bridge reach the file-private
 * widget state/functions without exporting layouts or guessing ABI offsets.
 */
#include "../../../source/interface/ui_widget.c"

#ifdef HALO_VITA
/* Action values are the small platform commands returned by vita_controls_poll:
 * 1 accept, 3/4/5/6 dpad up/down/left/right, 7 back.  They are translated here
 * to January's private widget/gamepad event constants; no tag-specific menu
 * index or focus pointer is modified by the platform layer. */
boolean halo_vita_ui_process_menu_action(short action)
{
    struct widget_instance *widget;
    struct ui_widget_definition *definition;
    struct event_record event = {0};
    boolean widget_deleted = FALSE;
    short button_index;
    long widget_index;

    if (!widget_globals.initialized || !we_are_at_the_main_menu)
        return FALSE;

    switch (action)
    {
    case 1:
        button_index = _gamepad_analog_button_a;
        break;
    case 3:
        button_index = _widget_event_dpad_up;
        break;
    case 4:
        button_index = _widget_event_dpad_down;
        break;
    case 5:
        button_index = _widget_event_dpad_left;
        break;
    case 6:
        button_index = _widget_event_dpad_right;
        break;
    case 7:
        button_index = _widget_event_b_button;
        break;
    default:
        return FALSE;
    }

    widget_globals.current_system_milliseconds = system_milliseconds();
    event.type = _event_type_button;
    event.controller_index = 0;
    event.data.button.index = (byte)button_index;
    event.data.button.value = 1;

    /* Mirror the event-dispatch and stack-restoration parts of
     * process_ui_widgets(), but deliberately do not pull its pause/attract/
     * filesystem/world update closure into this staged Main Menu runtime. */
    for (widget_index = 0;
        widget_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
        widget_index++)
    {
        widget = widget_globals.active_widgets[widget_index];
        if (!widget)
            continue;
        if (widget->local_player_index != NONE &&
            widget->local_player_index != event.controller_index)
            continue;

        definition = ui_widget_definition_get(widget->definition_tag_index);
        widget_instance_process_one_event_recursive(
            widget,
            definition,
            &event,
            &widget_deleted);

        if (!widget_globals.active_widgets[widget_index] &&
            widget_globals.widget_stack[widget_index])
        {
            struct widget_stack_data data;
            pop_widget(&widget_globals.widget_stack[widget_index], &data);
            if (data.previous_widget_tag != NONE)
            {
                struct widget_instance *new_widget = ui_widget_load_by_name_or_tag(
                    NULL,
                    data.previous_widget_tag,
                    NULL,
                    data.local_player_index,
                    NONE,
                    NONE,
                    NONE);
                if (new_widget)
                {
                    widget_instance_set_focused_child_by_index(
                        data.focused_child_parent_widget_tag,
                        new_widget,
                        data.focused_child_index);
                }
            }
        }

        vita_log("[VITA UI INPUT] action=%d button=%d root=%p focused=%p deleted=%d",
            (int)action, (int)button_index,
            widget_globals.active_widgets[widget_index],
            widget_globals.active_widgets[widget_index]
                ? widget_globals.active_widgets[widget_index]->focused_child
                : NULL,
            (int)widget_deleted);
        return TRUE;
    }

    vita_log("[VITA UI INPUT] action=%d ignored: no compatible active widget", (int)action);
    return FALSE;
}
#endif
