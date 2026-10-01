/* Preserve the original queue and keyboard event contract. Only native button
 * translation crosses this boundary; no Xbox event layouts cross into SDK code. */
#include "../../../source/interface/event_manager.c"
void halo_vita_ui_post_button(short index)
{
    struct event_record event = {0};
    if (!event_manager_globals.state.initialized) return;
    event.type = _event_type_button;
    event.data.button.index = (byte)index;
    event.data.button.value = 1;
    queue_event(&event, 0);
}
