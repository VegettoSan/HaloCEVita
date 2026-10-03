/* Vita-owned platform hooks consumed by the shared D3D8 presentation path.
 *
 * The desktop backend pumps SDL window events here and optionally enables a
 * presentation-time interpolation layer. Vita has neither an SDL window event
 * queue nor that interpolation system during the native Main Menu bring-up:
 * controller/lifecycle input is polled by vita_platform.c and Halo advances on
 * its original simulation cadence. Keep these as platform adapters rather
 * than introducing substitute renderer or menu behavior.
 */

void platform_pump_events(void)
{
	/* No host window event queue on the native Vita backend. */
}

int halo_interpolation_enabled(void)
{
	return 0;
}

#ifdef HALO_VITA_ORIGINAL_RUNTIME
#include <string.h>
#include <stdint.h>
typedef int BOOL;
#ifndef FALSE
#define FALSE 0
#endif
struct platform_ui_pointer;
void platform_video_window_size(int *width, int *height)
{
    *width = 960; *height = 544;
}
/* Vita has no desktop mouse. Native controller input stays in XInput. */
void platform_ui_pointer_set_active(BOOL active) { (void)active; }
BOOL platform_ui_pointer_read(struct platform_ui_pointer *pointer)
{
    (void)pointer;
    return FALSE;
}
#endif
