/* D3D8 platform-video contract for the native Vita backend.
 * vita_graphics_initialize() owns the single vitaGL context and runs before
 * Halo mounts ui.map. Direct3D_CreateDevice must reuse that context rather
 * than attempting a second vglInitWithCustomSizes(). */
#include "vita_runtime.h"

int platform_video_initialize(unsigned long width, unsigned long height)
{
    static int logged;
    int ready = vita_graphics_initialize();

    if (!logged) {
        logged = 1;
        vita_log("[VITA D3D8] platform video request logical=%lux%lu context=%s physical=960x544",
            width, height, ready ? "ready" : "FAILED");
    }
    return ready;
}
