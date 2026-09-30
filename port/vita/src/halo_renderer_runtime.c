/* Vita entry point for the original Halo rasterizer.
 * Preserve Halo's shell initialization order after the Vita platform and
 * physical-memory arena are ready: game state first, then the public
 * rasterizer wrapper. Device/cache/shader setup remains in original code. */
#include <xtl.h>
#include "cseries.h"
#include "saved games/game_state.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/xbox/rasterizer_xbox.h"
#include "vita_runtime.h"

static boolean vita_game_state_ready;
static boolean vita_renderer_ready;

int halo_vita_renderer_initialize(void)
{
    if (vita_renderer_ready) return 1;

    if (!vita_game_state_ready) {
        vita_log("[VITA 033] original game-state initialization begin");
        game_state_initialize();
        vita_game_state_ready = TRUE;
        vita_log("[VITA 034] original game-state arena ready");
    }

    vita_log("[VITA 035] original Xbox rasterizer initialization begin");
    if (!rasterizer_initialize()) {
        vita_log("MAIN MENU BLOCKED: original Xbox rasterizer initialization failed");
        return 0;
    }
    if (!global_d3d_device) {
        vita_log("MAIN MENU BLOCKED: rasterizer returned success without a D3D8 device");
        return 0;
    }

    vita_renderer_ready = TRUE;
    vita_log("[VITA 036] original Xbox rasterizer ready; D3D8 device=%p", global_d3d_device);
    return 1;
}
