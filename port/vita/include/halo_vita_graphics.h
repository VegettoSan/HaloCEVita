#ifndef HALO_VITA_GRAPHICS_H
#define HALO_VITA_GRAPHICS_H
/* Keep original Xbox/UI coordinates. Only screen-sized GL targets use the
 * half-resolution scale; final presentation remains native and letterboxed. */
#define HALO_VITA_GAME_WIDTH 640
#define HALO_VITA_GAME_HEIGHT 480
#define HALO_VITA_RENDER_WIDTH 320
#define HALO_VITA_RENDER_HEIGHT 240
#define HALO_VITA_DISPLAY_WIDTH 960
#define HALO_VITA_DISPLAY_HEIGHT 544

/* Initial diagnostic budgets. Full renderer stream flushing is still pending. */
#define HALO_VITA_GL_RAM_SIZE (16 * 1024 * 1024)
#define HALO_VITA_GL_CDRAM_SIZE (24 * 1024 * 1024)
#define HALO_VITA_LEGACY_SIZE (2 * 1024 * 1024)
#define HALO_VITA_STREAM_SIZE (2 * 1024 * 1024)
#define HALO_VITA_INDEX_SIZE (256 * 1024)
#define HALO_VITA_STREAM_RING 1
#endif
