#ifndef HALO_VITA_GRAPHICS_H
#define HALO_VITA_GRAPHICS_H
/* Keep Halo's original Xbox/UI 640x480 coordinate space and render it 1:1.
 * Final presentation remains native Vita 960x544 and letterboxed.  Keeping
 * the game target unscaled also removes half-resolution sampling/viewport
 * state from the UI-correctness path while the original menu is stabilized. */
#define HALO_VITA_GAME_WIDTH 640
#define HALO_VITA_GAME_HEIGHT 480
#define HALO_VITA_RENDER_WIDTH 640
#define HALO_VITA_RENDER_HEIGHT 480
#define HALO_VITA_DISPLAY_WIDTH 960
#define HALO_VITA_DISPLAY_HEIGHT 544

/* Bounded bring-up budgets. Vita stream reuse is synchronized before mapping. */
#define HALO_VITA_GL_RAM_SIZE (16 * 1024 * 1024)
#define HALO_VITA_GL_CDRAM_SIZE (24 * 1024 * 1024)
#define HALO_VITA_LEGACY_SIZE (2 * 1024 * 1024)
#define HALO_VITA_STREAM_SIZE (2 * 1024 * 1024)
#define HALO_VITA_INDEX_SIZE (256 * 1024)
#define HALO_VITA_STREAM_RING 1
/* SDK transfer completion bridge, native ABI; 0 means successful wait. */
int halo_vita_texture_transfer_finish(void);
#endif
