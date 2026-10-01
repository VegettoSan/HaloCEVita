#ifndef HALO_VITA_UI_WIDGET_PREFIX_H
#define HALO_VITA_UI_WIDGET_PREFIX_H

/*
 * halo_vita_prefix.h intentionally reuses the native Linux portability layer,
 * which defines HALO_LINUX for shared engine adaptations. ui_widget.c also
 * uses HALO_LINUX for desktop-only mouse target collection and pointer input.
 * Vita has its own controller path and no desktop pointer, so keep those
 * desktop-only branches out of this one translation unit.
 *
 * This header is force-included after halo_vita_prefix.h for ui_widget.c only.
 * The original Xbox/controller UI code remains unchanged.
 */
#ifdef HALO_LINUX
#undef HALO_LINUX
#endif

#endif
