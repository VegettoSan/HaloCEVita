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

static void halo_vita_sync_list_visual_state(struct widget_instance *widget)
{
    struct ui_widget_definition *definition;

    if (!widget)
        return;

    definition = ui_widget_definition_get(widget->definition_tag_index);
    if (widget->type == _ui_widget_type_spinner_list)
        spinner_list_update(widget);
    else if (widget->type == _ui_widget_type_column_list)
        column_list_update(widget, definition);
}

static void halo_vita_sync_focus_chain_visuals(struct widget_instance *root)
{
    struct widget_instance *widget;

    /* process_ui_widgets() normally calls the original list update before it
     * processes input.  The narrow Vita bridge intentionally skips that wide
     * closure, so run only those original list visual-state owners here. */
    for (widget = root; widget; widget = widget->focused_child)
        halo_vita_sync_list_visual_state(widget);
}

/* The normal Halo Main Menu lifecycle marks this state while entering the
 * shell.  The staged Vita path creates the same original root directly, so it
 * must complete that original state transition once the root has been
 * validated.  Do not write the private flag: keep main_menu_active() as the
 * owner of we_are_at_the_main_menu. */
int halo_vita_ui_activate_main_menu_state(void)
{
    struct widget_instance *root;

    if (!widget_globals.initialized)
    {
        vita_log("MAIN MENU BLOCKED: cannot activate UI state before widget globals");
        return FALSE;
    }

    root = halo_vita_menu_root(0);
    if (!root)
    {
        vita_log("MAIN MENU BLOCKED: cannot activate UI state without original root");
        return FALSE;
    }

    main_menu_active(TRUE);
    halo_vita_sync_focus_chain_visuals(root);
    vita_log("[VITA UI STATE] original Main Menu active=%d root=%08lx focused=%08lx",
        (int)main_menu_is_active(),
        (unsigned long)root->definition_tag_index,
        root->focused_child
            ? (unsigned long)root->focused_child->definition_tag_index
            : (unsigned long)NONE);
    return main_menu_is_active();
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
    {
        vita_log("[VITA UI INPUT] focus button=%d ignored: focused chain has no vertical list",
            (int)button_index);
        return FALSE;
    }

    before = target->focused_child;
    if (button_index == _widget_event_dpad_up)
        moved = widget_event_function_list_widget_goto_previous_item(
            target, event, &deleted);
    else if (button_index == _widget_event_dpad_down)
        moved = widget_event_function_list_widget_goto_next_item(
            target, event, &deleted);

    /* In the full original update loop this runs on the following widget
     * update and selects sprite frame 1 for the focused two-frame item while
     * resetting the others to frame 0.  Keep that exact owner rather than a
     * Vita-side highlight or hard-coded Main Menu bitmap choice. */
    if (!deleted)
        halo_vita_sync_list_visual_state(target);

    after = target->focused_child;
    if (moved)
        ui_play_audio_feedback_sound(_ui_audio_feedback_cursor);

    vita_log("[VITA UI INPUT] focus button=%d list=%08lx before=%08lx after=%08lx moved=%d deleted=%d frame=%ld",
        (int)button_index,
        (unsigned long)target->definition_tag_index,
        before ? (unsigned long)before->definition_tag_index : (unsigned long)NONE,
        after ? (unsigned long)after->definition_tag_index : (unsigned long)NONE,
        (int)moved,
        (int)deleted,
        after ? (long)after->animation.current_frame_index : -1L);

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
    {
        vita_log("[VITA UI INPUT] action=%d rejected initialized=%d main_menu_active=%d",
            (int)action, (int)widget_globals.initialized,
            (int)we_are_at_the_main_menu);
        return FALSE;
    }

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
