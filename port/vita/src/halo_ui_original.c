/* Native input boundary only. Halo owns the complete UI frame. */
#include "../../../source/interface/ui_widget.c"
void halo_vita_ui_post_button(short index);

int halo_vita_ui_activate_main_menu_state(void)
{
    if (!widget_globals.initialized || !widget_globals.active_widgets[0])
        return FALSE;
    main_menu_active(TRUE);
    return main_menu_is_active();
}

int halo_vita_ui_process_menu_action(short action)
{
    short button;
    switch (action) {
    case 1: button = _gamepad_analog_button_a; break;
    case 2: button = _gamepad_analog_button_x; break;
    case 11: button = _gamepad_analog_button_y; break;
    case 3: button = _widget_event_dpad_up; break;
    case 4: button = _widget_event_dpad_down; break;
    case 5: button = _widget_event_dpad_left; break;
    case 6: button = _widget_event_dpad_right; break;
    case 7: button = _widget_event_b_button; break;
    case 8: button = _gamepad_binary_button_start; break;
    case 9: button = _gamepad_analog_button_left_trigger; break;
    case 10: button = _gamepad_analog_button_right_trigger; break;
    default: return FALSE;
    }
    halo_vita_ui_post_button(button);
    return TRUE;
}

void halo_vita_ui_process_shell_frame(void)
{
    process_ui_widgets();
}
