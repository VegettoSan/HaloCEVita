/* Vita-only translation-unit wrapper around January's original ui_widget.c.
 *
 * Keep the original implementation as the owner of widget state, focus,
 * event handlers and stack transitions.  The staged Vita shell deliberately
 * does not link process_ui_widgets(): that routine also owns pause/attract/
 * filesystem/world update work which is outside the verified Main Menu
 * closure.  This wrapper reaches only the original focus/list primitives and
 * a narrow subset of the original event effects, without guessing runtime
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

/* ui_widget_launch_widget() normally asks ui_widget_load_by_name_or_tag() to
 * replace an existing root.  The bring-up guard inside the shared loader still
 * rejects that broad lifecycle.  Perform only the already-original pieces in
 * their normal order: remember the invoking/focus tags, delete the original
 * root with ui_widget_delete(), then let the original loader push history and
 * construct the requested ui.map root. */
static struct widget_instance *halo_vita_open_original_root(
    struct widget_instance *calling_widget,
    long target_tag_index)
{
    struct widget_instance *top;
    struct widget_instance *opened;
    long invoking_widget_tag;
    long focused_child_parent_widget_tag;
    short focused_child_index;
    short local_player_index;

    if (!calling_widget || target_tag_index == NONE)
        return NULL;

    top = widget_instance_get_topmost_parent(calling_widget);
    invoking_widget_tag = top->definition_tag_index;
    focused_child_parent_widget_tag = calling_widget->parent
        ? calling_widget->parent->definition_tag_index
        : NONE;
    focused_child_index = (short)widget_instance_get_child_index_from_parent(calling_widget);
    local_player_index = calling_widget->local_player_index;

    vita_log("[VITA UI TRANSITION] open begin from=%08lx target=%08lx focus_parent=%08lx focus_index=%d",
        (unsigned long)invoking_widget_tag,
        (unsigned long)target_tag_index,
        (unsigned long)focused_child_parent_widget_tag,
        (int)focused_child_index);

    ui_widget_delete(top);
    opened = ui_widget_load_by_name_or_tag(
        NULL,
        target_tag_index,
        NULL,
        local_player_index,
        invoking_widget_tag,
        focused_child_parent_widget_tag,
        focused_child_index);

    if (opened)
    {
        halo_vita_sync_focus_chain_visuals(opened);
        vita_log("[VITA UI TRANSITION] open PASS root=%08lx focused=%08lx",
            (unsigned long)opened->definition_tag_index,
            opened->focused_child
                ? (unsigned long)opened->focused_child->definition_tag_index
                : (unsigned long)NONE);
    }
    else
    {
        vita_log("MAIN MENU BLOCKED: original target root %08lx failed to load",
            (unsigned long)target_tag_index);
    }

    return opened;
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
    list_tag = target->definition_tag_index;
    before_tag = before ? before->definition_tag_index : NONE;
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

/* Execute only the generic original effects needed to traverse ui.map roots.
 * Function handlers remain owned by ui_widget_event_handler_function_invoke()
 * and therefore keep its explicit Vita allow-list.  Scenario scripts stay
 * blocked: the staged shell has not initialized the HS/world closure. */
static int halo_vita_dispatch_original_menu_handler(
    struct widget_instance *widget,
    struct ui_widget_definition *definition,
    struct event_record *event,
    struct ui_widget_event_handler_reference *handler)
{
    boolean widget_deleted = FALSE;
    boolean function_failed = FALSE;
    long audio_feedback = _ui_audio_feedback_none;

    if (TEST_FLAG(handler->flags, _event_handler_run_scenario_script_bit) &&
        handler->script[0])
    {
        vita_log("MAIN MENU BLOCKED: UI script '%s' needs original HS/world runtime",
            handler->script);
        return FALSE;
    }

    if (TEST_FLAG(handler->flags, _event_handler_run_function_bit))
    {
        halo_vita_ui_event_reset();
        if (!ui_widget_event_handler_function_invoke(
            widget, event, handler->function, &widget_deleted))
            function_failed = TRUE;
        if (widget_deleted)
            return TRUE;
    }

    if (!function_failed &&
        TEST_FLAG(handler->flags, _event_handler_give_focus_to_widget_bit))
    {
        if (handler->widget_tag.index == NONE)
            return FALSE;
        widget_instance_give_focus_by_tag(
            widget, handler->widget_tag.index, widget->local_player_index);
        audio_feedback = _ui_audio_feedback_cursor;
    }

    if (!function_failed &&
        TEST_FLAG(handler->flags, _event_handler_reload_self_bit))
        widget_instance_reload_recursive(widget);

    if (!function_failed &&
        TEST_FLAG(handler->flags, _event_handler_reload_widget_bit))
    {
        if (handler->widget_tag.index == NONE)
            return FALSE;
        ui_widget_reload_by_tag(handler->widget_tag.index);
    }

    if (!function_failed &&
        TEST_FLAG(handler->flags, _event_handler_open_widget_bit))
    {
        if (!halo_vita_open_original_root(widget, handler->widget_tag.index))
            return FALSE;
        ui_play_audio_feedback_sound(
            audio_feedback == _ui_audio_feedback_none
                ? _ui_audio_feedback_forward
                : audio_feedback);
        return TRUE;
    }

    if (!function_failed &&
        TEST_FLAG(handler->flags, _event_handler_go_back_to_previous_widget_bit))
    {
        if (!halo_vita_go_back_original(widget_instance_get_topmost_parent(widget)))
            return FALSE;
        ui_play_audio_feedback_sound(_ui_audio_feedback_back);
        return TRUE;
    }

    /* These deletion-only effects are safe after function/focus work because
     * they use the original widget allocator and deletion recursion. */
    if (!function_failed &&
        TEST_FLAG(handler->flags, _event_handler_close_other_widget_bit) &&
        handler->widget_tag.index != NONE)
    {
        struct widget_instance *other =
            widget_instance_find_by_tag_index(handler->widget_tag.index);
        if (other && other != widget)
            ui_widget_delete(other);
    }

    if (!function_failed &&
        TEST_FLAG(handler->flags, _event_handler_close_current_widget_bit))
    {
        ui_widget_delete(widget_instance_get_topmost_parent(widget));
        ui_play_audio_feedback_sound(_ui_audio_feedback_back);
        return TRUE;
    }

    if (!function_failed &&
        TEST_FLAG(handler->flags, _event_handler_close_all_widgets_bit))
    {
        ui_widgets_close_all();
        return TRUE;
    }

    if (!function_failed &&
        TEST_FLAG(handler->flags, _event_handler_replace_with_other_widget_bit))
    {
        /* Replacement is rarer than root open and has sibling relinking
         * semantics. Keep it explicit until a reachable retail handler asks
         * for it; never approximate it as a root transition. */
        vita_log("MAIN MENU BLOCKED: replace-with-widget effect not staged target=%08lx",
            (unsigned long)handler->widget_tag.index);
        return FALSE;
    }

    if (function_failed &&
        TEST_FLAG(handler->flags,
            _event_handler_look_for_conditional_widget_on_failure_bit))
    {
        long conditional_index;

        for (conditional_index = 0;
            conditional_index < definition->conditional_widgets.count;
            conditional_index++)
        {
            struct ui_widget_conditional_reference *conditional =
                (struct ui_widget_conditional_reference *)
                    definition->conditional_widgets.address + conditional_index;

            if (TEST_FLAG(conditional->flags,
                _conditional_widget_load_if_event_handler_function_fails_bit) &&
                conditional->widget_tag.index != NONE)
            {
                if (!halo_vita_open_original_root(
                    widget, conditional->widget_tag.index))
                    return FALSE;
                ui_play_audio_feedback_sound(_ui_audio_feedback_forward);
                return TRUE;
            }
        }
    }

    if (function_failed || halo_vita_ui_event_failed())
        return FALSE;

    if (audio_feedback != _ui_audio_feedback_none)
        ui_play_audio_feedback_sound((short)audio_feedback);
    return TRUE;
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
    /* A rejected creation handler belongs to its own event. Pure open/back
     * handlers must not inherit a failure from an earlier screen. */
    halo_vita_ui_event_reset();
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
