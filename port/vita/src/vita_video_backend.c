/* D3D8 platform-video contract for the native Vita backend.
 * vita_graphics_initialize() owns the single vitaGL context and runs before
 * Halo mounts ui.map. Direct3D_CreateDevice must reuse that context rather
 * than attempting a second vglInitWithCustomSizes(). */
#include "vita_runtime.h"
#include "halo_vita_graphics.h"

int platform_video_initialize(unsigned long width, unsigned long height)
{
    static int logged;
    int ready = vita_graphics_initialize();

    if (!logged) {
        logged = 1;
        vita_log("[VITA D3D8] platform video request logical=%lux%lu context=%s internal=%dx%d display=%dx%d",
            width, height, ready ? "ready" : "FAILED",
            HALO_VITA_RENDER_WIDTH, HALO_VITA_RENDER_HEIGHT,
            HALO_VITA_DISPLAY_WIDTH, HALO_VITA_DISPLAY_HEIGHT);
    }
    return ready;
}
