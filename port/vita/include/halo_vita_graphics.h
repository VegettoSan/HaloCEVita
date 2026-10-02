#ifndef HALO_VITA_GRAPHICS_H
#define HALO_VITA_GRAPHICS_H
/* Keep Halo's original Xbox/UI 640x480 coordinate space and rasterize the
 * screen-sized targets 1:1 at 640x480 for this UI comparison build. The
 * authored widget coordinates therefore reach the render target without the
 * half-resolution 320x240 viewport/scissor scaling step. The original D3D8
 * Present blit letterboxes it on the native 960x544 Vita panel. */
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
