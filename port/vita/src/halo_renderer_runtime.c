/* Vita entry point for the original Halo rasterizer.
 * Keep device/cache/shader setup inside the decompiled Xbox rasterizer; this
 * file only sequences that original initialization after vitaGL is ready. */
#include <xtl.h>
#include "cseries.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/xbox/rasterizer_xbox.h"
#include "vita_runtime.h"

boolean _rasterizer_initialize(void);

static boolean vita_renderer_ready;

int halo_vita_renderer_initialize(void)
{
    if (vita_renderer_ready) return 1;

    vita_log("[VITA 033] original Xbox rasterizer initialization begin");
    if (!_rasterizer_initialize()) {
        vita_log("MAIN MENU BLOCKED: original Xbox rasterizer initialization failed");
        return 0;
    }
    if (!global_d3d_device) {
        vita_log("MAIN MENU BLOCKED: rasterizer returned success without a D3D8 device");
        return 0;
    }

    vita_renderer_ready = TRUE;
    vita_log("[VITA 034] original Xbox rasterizer ready; D3D8 device=%p", global_d3d_device);
    return 1;
}
