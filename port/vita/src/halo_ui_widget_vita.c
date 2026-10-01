/* Vita-only translation-unit wrapper around January's original ui_widget.c.
 *
 * Keep the original implementation as the owner of widget state, focus,
 * event handlers and stack transitions.  The staged Vita shell deliberately
 * does not link process_ui_widgets(): that routine also owns pause/attract/
 * filesystem/world update work which is outside the verified Main Menu
 * closure.  This wrapper reaches only the original focus/list primitives and
 * the generic original event effects, without guessing runtime
 * layouts or creating a parallel Vita menu.
 */

/*
 * ui_widget_delete() resumes the world when the final pause-owning widget is
 * destroyed.  That is correct once a gameplay world exists, but during the
 * staged ui.map shell bring-up it reaches main_menu_ensure_player_queues_exist
 * and game_time_start, which in turn pull the update server/client runtime into
 * the otherwise UI-only Vita closure.  Keep January's delete/list/history code
 * intact and redirect only those four world-resume calls in this translation
 * unit.  The real functions remain untouched for the future gameplay closure.
 */
void halo_vita_menu_deferred_main_menu_ensure_player_queues_exist(void);
void halo_vita_menu_deferred_game_time_dispose_from_old_map(void);
void halo_vita_menu_deferred_game_time_initialize_for_new_map(void);
void halo_vita_menu_deferred_game_time_start(void);

#define main_menu_ensure_player_queues_exist halo_vita_menu_deferred_main_menu_ensure_player_queues_exist
#define game_time_dispose_from_old_map halo_vita_menu_deferred_game_time_dispose_from_old_map
#define game_time_initialize_for_new_map halo_vita_menu_deferred_game_time_initialize_for_new_map
#define game_time_start halo_vita_menu_deferred_game_time_start
#include "../../../source/interface/ui_widget.c"
#undef game_time_start
#undef game_time_initialize_for_new_map
#undef game_time_dispose_from_old_map
#undef main_menu_ensure_player_queues_exist

#ifdef HALO_VITA
void halo_vita_ui_post_button(short index);
static void halo_vita_log_deferred_world_resume(char const *operation)
{
    vita_log("[VITA UI TRANSITION] defer world resume operation=%s (ui.map shell has no gameplay world yet)",
        operation ? operation : "<unknown>");
}

void halo_vita_menu_deferred_main_menu_ensure_player_queues_exist(void)
{
    halo_vita_log_deferred_world_resume("main_menu_ensure_player_queues_exist");
}

void halo_vita_menu_deferred_game_time_dispose_from_old_map(void)
{
    halo_vita_log_deferred_world_resume("game_time_dispose_from_old_map");
}

void halo_vita_menu_deferred_game_time_initialize_for_new_map(void)
{
    halo_vita_log_deferred_world_resume("game_time_initialize_for_new_map");
}

void halo_vita_menu_deferred_game_time_start(void)
{
    halo_vita_log_deferred_world_resume("game_time_start");
}

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

static int halo_vita_go_back_original(struct widget_instance *root)
{
    struct widget_instance *restored;

    if (!root || !widget_globals.widget_stack[0])
        return FALSE;

    vita_log("[VITA UI TRANSITION] back begin root=%08lx",
        (unsigned long)root->definition_tag_index);
    widget_instance_go_back_to_previous(root);
    restored = halo_vita_menu_root(0);
    if (!restored)
    {
        vita_log("MAIN MENU BLOCKED: original history pop produced no root");
        return FALSE;
    }

    halo_vita_sync_focus_chain_visuals(restored);
    vita_log("[VITA UI TRANSITION] back PASS root=%08lx focused=%08lx",
        (unsigned long)restored->definition_tag_index,
        restored->focused_child
            ? (unsigned long)restored->focused_child->definition_tag_index
            : (unsigned long)NONE);
    return TRUE;
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
    long list_tag;
    long before_tag;
    boolean deleted = FALSE;
    boolean moved = FALSE;
    boolean vertical = button_index == _widget_event_dpad_up ||
        button_index == _widget_event_dpad_down;
    boolean previous = button_index == _widget_event_dpad_up ||
        button_index == _widget_event_dpad_left;
    boolean tab_children = FALSE;

    /* January's Main Menu select list is a column list with the
     * _widget_dpad_updown_tabs_thru_list_items_bit.  Search the active focus
     * chain rather than hard-coding that tag so the same bridge remains valid
     * for later menus that use the original list contract. */
    for (widget = root; widget; widget = widget->focused_child)
    {
        struct ui_widget_definition *definition =
            ui_widget_definition_get(widget->definition_tag_index);

        boolean children = widget->focused_child && TEST_FLAG(definition->flags,
            vertical ? _widget_dpad_updown_tabs_thru_children_bit :
                       _widget_dpad_leftright_tabs_thru_children_bit);
        boolean items = (widget->type == _ui_widget_type_spinner_list ||
            widget->type == _ui_widget_type_column_list) && TEST_FLAG(definition->flags,
            vertical ? _widget_dpad_updown_tabs_thru_list_items_bit :
                       _widget_dpad_leftright_tabs_thru_list_items_bit);
        if (children || items)
        {
            target = widget;
            tab_children = children;
        }
    }

    if (!target)
    {
        vita_log("[VITA UI INPUT] focus button=%d ignored: focused chain has no matching tab/list policy",
            (int)button_index);
        return FALSE;
    }

    before = target->focused_child;
    list_tag = target->definition_tag_index;
    before_tag = before ? before->definition_tag_index : NONE;
    if (tab_children)
    {
        if (previous)
            widget_instance_tab_to_previous_valid_widget(target);
        else
            widget_instance_tab_to_next_valid_widget(target);
        moved = before != target->focused_child;
    }
    else if (previous)
        moved = widget_event_function_list_widget_goto_previous_item(
            target, event, &deleted);
    else
        moved = widget_event_function_list_widget_goto_next_item(
            target, event, &deleted);

    /* In the full original update loop this runs on the following widget
     * update and selects sprite frame 1 for the focused two-frame item while
     * resetting the others to frame 0.  Keep that exact owner rather than a
     * Vita-side highlight or hard-coded Main Menu bitmap choice. */
    if (!deleted)
        halo_vita_sync_focus_chain_visuals(root);

    /* Original list callbacks may delete the calling list. */
    after = deleted ? NULL : target->focused_child;
    if (moved)
        ui_play_audio_feedback_sound(_ui_audio_feedback_cursor);

    vita_log("[VITA UI INPUT] focus button=%d list=%08lx before=%08lx after=%08lx moved=%d deleted=%d frame=%ld",
        (int)button_index,
        (unsigned long)list_tag,
        (unsigned long)before_tag,
        after ? (unsigned long)after->definition_tag_index : (unsigned long)NONE,
        (int)moved,
        (int)deleted,
        after ? (long)after->animation.current_frame_index : -1L);

    return moved && !deleted;
}

/* Execute only the generic original event effects for both input and created/deleted
 * callbacks. This includes conditional roots, replacements and authored
 * sound effects. Scripts/world services still have explicit runtime gates. */
static int halo_vita_dispatch_original_menu_handler(
    struct widget_instance *widget,
    struct ui_widget_definition *definition,
    struct event_record *event,
    struct ui_widget_event_handler_reference *handler)
{
    boolean deleted = FALSE;
    vita_menu_dispatch_blocked = FALSE;
    event_handler_dispatch(widget, definition, event, handler, &deleted);
    /* The original dispatcher owns deletion/relinking; do not touch widget
     * after it returns. A matched event is consumed even for a false predicate. */
    return !vita_menu_dispatch_blocked && !halo_vita_ui_event_failed();
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
     * processor while preserving the original handler data/effects. */
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
                long widget_tag = widget->definition_tag_index;
                short handler_function = handler->function;
                long handler_flags = handler->flags;
                int handled = halo_vita_dispatch_original_menu_handler(
                    widget, definition, event, handler);
                vita_log("[VITA UI INPUT] dispatch button=%d widget=%08lx function=%d flags=%08lx handled=%d failed=%d",
                    (int)button_index,
                    (unsigned long)widget_tag,
                    (int)handler_function,
                    (unsigned long)handler_flags,
                    handled,
                    (int)halo_vita_ui_event_failed());
                return handled;
            }
        }

        widget = widget->parent;
    }

    /* Halo's root history is the natural B-button fallback when a screen has
     * no explicit B handler. Keep that stack behavior rather than inventing
     * per-screen Vita mappings. */
    if (button_index == _widget_event_b_button &&
        widget_globals.widget_stack[0])
        return halo_vita_go_back_original(root);

    vita_log("[VITA UI INPUT] button=%d ignored: focused chain has no matching handler",
        (int)button_index);
    return FALSE;
}

/* Action values are the small platform commands produced by the Vita shell:
 * 1 accept, 2 X, 3/4/5/6 dpad, 7 back, 8 Start, 9/10 triggers. */
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
    case 2:
        button_index = _gamepad_analog_button_x;
        break;
    case 8:
        button_index = _gamepad_binary_button_start;
        break;
    case 9:
        button_index = _gamepad_analog_button_left_trigger;
        break;
    case 10:
        button_index = _gamepad_analog_button_right_trigger;
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
    /* A rejected creation handler belongs to its own event. Pure open/back
     * handlers must not inherit a failure from an earlier screen. */
    halo_vita_ui_event_reset();
    event.type = _event_type_button;
    event.controller_index = 0;
    event.data.button.index = (byte)button_index;
    event.data.button.value = 1;

    if (virtual_keyboard_active()) {
        halo_vita_ui_post_button(button_index);
        virtual_keyboard_process();
        event_manager_flush();
        return TRUE;
    }

    if (button_index >= _widget_event_dpad_up &&
        button_index <= _widget_event_dpad_right)
        return halo_vita_move_menu_focus(root, button_index, &event);

    return halo_vita_dispatch_focused_button(root, button_index, &event);
}
/* The original empty-event update runs animations, timeouts and authored
 * keyboard completion callbacks without entering pause/attract/world update. */
void halo_vita_ui_process_shell_frame(void)
{
    long index;
    if (!widget_globals.initialized || !we_are_at_the_main_menu) return;
    widget_globals.current_system_milliseconds = system_milliseconds();
    if (virtual_keyboard_active()) {
        virtual_keyboard_process();
        event_manager_flush();
        return;
    }
    for (index = 0; index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS; ++index) {
        struct widget_instance *widget = widget_globals.active_widgets[index];
        if (widget) {
            struct event_record event = {0};
            boolean deleted = FALSE;
            event.controller_index = widget->local_player_index;
            widget_instance_process_one_event_recursive(widget,
                ui_widget_definition_get(widget->definition_tag_index),
                &event, &deleted);
        }
        if (!widget_globals.active_widgets[index] && widget_globals.widget_stack[index]) {
            struct widget_stack_data data;
            pop_widget(&widget_globals.widget_stack[index], &data);
            if (data.previous_widget_tag != NONE) {
                struct widget_instance *restored = ui_widget_load_by_name_or_tag(
                    NULL, data.previous_widget_tag, NULL, data.local_player_index,
                    NONE, NONE, NONE);
                if (restored)
                    widget_instance_set_focused_child_by_index(
                        data.focused_child_parent_widget_tag, restored, data.focused_child_index);
            }
        }
    }
}
#endif
