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
