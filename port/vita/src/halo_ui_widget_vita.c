/* Vita-only translation-unit wrapper around January's original ui_widget.c.
 *
 * Keep the original implementation as the owner of widget state, focus,
 * event handlers and stack transitions.  The staged Vita shell deliberately
 * does not link process_ui_widgets(): that routine also owns pause/attract/
 * filesystem/world update work which is outside the verified Main Menu
 * closure.  This wrapper reaches only the original focus/list primitives and
 * the already-staged event dispatcher, without guessing runtime layouts.
 */
#include "../../../source/interface/ui_widget.c"

#ifdef HALO_VITA
static struct widget_instance *halo_vita_menu_root(short controller_index)
{
    long widget_index;

    for (widget_index = 0;
        widget_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
        widget_index++)
    {
        struct widget_instance *widget = widget_globals.active_widgets[widget_index];

        if (widget &&
            (widget->local_player_index == NONE ||
             widget->local_player_index == controller_index))
            return widget;
    }

    return NULL;
}

static struct widget_instance *halo_vita_deepest_focus(struct widget_instance *root)
{
    struct widget_instance *widget = root;

    while (widget && widget->focused_child)
        widget = widget->focused_child;

    return widget;
}

static int halo_vita_move_menu_focus(
    struct widget_instance *root,
    short button_index,
    struct event_record *event)
{
    struct widget_instance *widget;
    struct widget_instance *target = NULL;
    struct widget_instance *before;
    struct widget_instance *after;
    boolean deleted = FALSE;
    boolean moved = FALSE;

    /* January's Main Menu select list is a column list with the
     * _widget_dpad_updown_tabs_thru_list_items_bit.  Search the active focus
     * chain rather than hard-coding that tag so the same bridge remains valid
     * for later menus that use the original list contract. */
    for (widget = root; widget; widget = widget->focused_child)
    {
        struct ui_widget_definition *definition =
            ui_widget_definition_get(widget->definition_tag_index);

        if ((widget->type == _ui_widget_type_spinner_list ||
             widget->type == _ui_widget_type_column_list) &&
            ((button_index == _widget_event_dpad_up ||
              button_index == _widget_event_dpad_down) &&
             TEST_FLAG(definition->flags,
                 _widget_dpad_updown_tabs_thru_list_items_bit)))
        {
            target = widget;
        }
    }

    if (!target)
        return FALSE;

    before = target->focused_child;
    if (button_index == _widget_event_dpad_up)
        moved = widget_event_function_list_widget_goto_previous_item(
            target, event, &deleted);
    else if (button_index == _widget_event_dpad_down)
        moved = widget_event_function_list_widget_goto_next_item(
            target, event, &deleted);

    after = target->focused_child;
    if (moved)
        ui_play_audio_feedback_sound(_ui_audio_feedback_cursor);

    vita_log("[VITA UI INPUT] focus button=%d list=%08lx before=%08lx after=%08lx moved=%d deleted=%d",
        (int)button_index,
        (unsigned long)target->definition_tag_index,
        before ? (unsigned long)before->definition_tag_index : (unsigned long)NONE,
        after ? (unsigned long)after->definition_tag_index : (unsigned long)NONE,
        (int)moved,
        (int)deleted);

    return moved && !deleted;
}

static int halo_vita_dispatch_focused_button(
    struct widget_instance *root,
    short button_index,
    struct event_record *event)
{
    struct widget_instance *widget = halo_vita_deepest_focus(root);

    /* Match January's focused-child propagation direction, but only dispatch
     * an explicitly matching handler.  This avoids the unrelated
     * input_has_gamepad/pause/update-server branches in the full recursive
     * processor while preserving the original handler implementation. */
    while (widget)
    {
        struct ui_widget_definition *definition =
            ui_widget_definition_get(widget->definition_tag_index);
        long handler_index;

        for (handler_index = 0;
            handler_index < definition->event_handlers.count;
            handler_index++)
        {
            struct ui_widget_event_handler_reference *handler =
                (struct ui_widget_event_handler_reference *)
                    definition->event_handlers.address + handler_index;

            if (handler->event_type == button_index)
            {
                boolean deleted = FALSE;

                event_handler_dispatch(
                    widget,
                    definition,
                    event,
                    handler,
                    &deleted);
                vita_log("[VITA UI INPUT] dispatch button=%d widget=%08lx function=%d flags=%08lx deleted=%d blocked=%d",
                    (int)button_index,
                    (unsigned long)widget->definition_tag_index,
                    (int)handler->function,
                    (unsigned long)handler->flags,
                    (int)deleted,
                    (int)vita_menu_dispatch_blocked);
                return deleted ? FALSE : TRUE;
            }
        }

        widget = widget->parent;
    }

    vita_log("[VITA UI INPUT] button=%d ignored: focused chain has no matching handler",
        (int)button_index);
    return FALSE;
}

/* Action values are the small platform commands produced by the Vita shell:
 * 1 accept, 3/4/5/6 dpad up/down/left/right, 7 back. */
int halo_vita_ui_process_menu_action(short action)
{
    struct widget_instance *root;
    struct event_record event = {0};
    short button_index;

    if (!widget_globals.initialized || !we_are_at_the_main_menu)
        return FALSE;

    root = halo_vita_menu_root(0);
    if (!root)
    {
        vita_log("[VITA UI INPUT] action=%d ignored: no compatible active root",
            (int)action);
        return FALSE;
    }

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

    if (button_index == _widget_event_dpad_up ||
        button_index == _widget_event_dpad_down)
        return halo_vita_move_menu_focus(root, button_index, &event);

    /* Left/right are intentionally not remapped to up/down.  Menus that own
     * horizontal lists will get their original list contract when staged. */
    if (button_index == _widget_event_dpad_left ||
        button_index == _widget_event_dpad_right)
        return FALSE;

    return halo_vita_dispatch_focused_button(root, button_index, &event);
}
#endif
