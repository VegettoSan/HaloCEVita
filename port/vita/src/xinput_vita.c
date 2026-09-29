/* Typed XDK controller entry points backed by actual native Vita input. */
#include "platform.h"
#include "vita_runtime.h"
#include <string.h>

XPP_DEVICE_TYPE XDEVICE_TYPE_GAMEPAD_TABLE;
XPP_DEVICE_TYPE XDEVICE_TYPE_MEMORY_UNIT_TABLE;
XPP_DEVICE_TYPE XDEVICE_TYPE_DEBUG_KEYBOARD_TABLE;
static struct { BOOL open; DWORD packet; XINPUT_GAMEPAD previous; } controller;
typedef char vita_gamepad_layout[sizeof(struct vita_gamepad_sample) == sizeof(XINPUT_GAMEPAD) ? 1 : -1];
typedef char vita_gamepad_axis_layout[offsetof(struct vita_gamepad_sample, lx) == offsetof(XINPUT_GAMEPAD, sThumbLX) ? 1 : -1];

HANDLE WINAPI XInputOpen(PXPP_DEVICE_TYPE type, DWORD port, DWORD slot, PXINPUT_POLLING_PARAMETERS polling)
{
	(void)polling;
	if (type != XDEVICE_TYPE_GAMEPAD || port != 0 || slot != 0) {
		vita_log("XInputOpen: unsupported device/port/slot"); return NULL;
	}
	memset(&controller, 0, sizeof(controller)); controller.open = TRUE;
	return (HANDLE)&controller;
}
VOID WINAPI XInputClose(HANDLE device)
{
	if (device == (HANDLE)&controller) controller.open = FALSE;
}
DWORD WINAPI XInputGetState(HANDLE device, PXINPUT_STATE state)
{
	struct vita_gamepad_sample sample;
	if (!state) return ERROR_INVALID_PARAMETER;
	memset(state, 0, sizeof(*state));
	if (device != (HANDLE)&controller || !controller.open || !vita_read_gamepad(&sample))
		return ERROR_DEVICE_NOT_CONNECTED;
	memcpy(&state->Gamepad, &sample, sizeof(sample));
	if (memcmp(&controller.previous, &state->Gamepad, sizeof(state->Gamepad))) {
		++controller.packet; controller.previous = state->Gamepad;
	}
	state->dwPacketNumber = controller.packet;
	return ERROR_SUCCESS;
}
