/* Vita entry point for the original Halo rasterizer.
 * Preserve Halo's shell initialization order after the Vita platform and
 * physical-memory arena are ready. Menu frames use the same rasterizer frame /
 * window / UI / present sequence as render_frame_pregame; this file only
 * supplies the pregame camera that main_pregame_render normally builds. */
#include <xtl.h>
#include "cseries.h"
#include "cache/texture_cache.h"
#include "effects/decals.h"
#include "math/real_math.h"
#include "main/main.h"
#include "render/render.h"
#include "render/render_cameras.h"
#include "interface/ui_widget.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/xbox/rasterizer_xbox.h"
#include "vita_runtime.h"

/* cache_files.c owns the map-open sequencing and intentionally keeps this
 * per-map entry point out of texture_cache.h. Vita mounts ui.map through its
 * checked logical-range backend instead of scenario_tags_load(), so restore
 * that per-map transition after rasterizer_initialize() has created the
 * process-lifetime Xbox texture cache. */
void texture_cache_open(void);

/* January keeps this helper private to main.c; the pregame path uses it to
 * derive the same full-screen and title-safe rectangles used by retail. */
void compute_window_bounds(long player_index, long num_players,
    rectangle2d *pixel_bounds, rectangle2d *safe_frame_bounds);

static boolean vita_renderer_ready;
static boolean vita_texture_cache_opened;
static boolean vita_decals_ready;
static boolean vita_first_menu_frame = TRUE;

int halo_vita_renderer_initialize(void)
{
    if (vita_renderer_ready) return 1;

    /* Physical/game-state ownership belongs to halo_vita_memory_initialize().
     * The Vita bring-up already allocates Halo's game-state buffer before the
     * UI cache is mounted. Calling game_state_initialize() here allocates that
     * buffer a second time and correctly trips January's buffer_allocated
     * assertion. Keep renderer initialization limited to the renderer-side
     * subsystems skipped by the special Vita ui.map mount. */
    vita_log("[VITA 033] original game-state already owned by memory bring-up");

    /* The original lifecycle is split across two owners:
     *   rasterizer_initialize() -> texture_cache_new()
     *   scenario_tags_load()    -> texture_cache_open()
     * The special Vita ui.map mount bypasses scenario_tags_load(), but it must
     * not pre-create the texture cache because rasterizer_initialize() creates
     * it itself. */
    vita_log("[VITA 035] original Xbox rasterizer initialization begin");
    if (!rasterizer_initialize()) {
        vita_log("MAIN MENU BLOCKED: original Xbox rasterizer initialization failed");
        return 0;
    }
    if (!global_d3d_device) {
        vita_log("MAIN MENU BLOCKED: rasterizer returned success without a D3D8 device");
        return 0;
    }

    /* Restore only the per-map transition that the Vita ui.map mount skipped.
     * No pixels or menu assets are supplied here; bitmap reads still flow
     * through Halo's original texture cache and cache_file_read backend. */
    if (!vita_texture_cache_opened) {
        vita_log("[VITA 036T] original Xbox texture cache per-map open begin");
        texture_cache_open();
        vita_texture_cache_opened = TRUE;
        vita_log("[VITA 036T] original Xbox texture cache per-map open PASS");
    }

    /* game_initialize() owns decals_initialize(), which in turn creates the
     * rasterizer's decal vertex LRU via rasterizer_decals_initialize(). The
     * staged Vita Main Menu path does not run full game_initialize(), so its
     * first rasterizer_frame_begin() previously reached
     * rasterizer_decal_vertices_begin_update() with local_vertex_cache == NULL.
     * The matching per-map phase is also required because decals_update() runs
     * in that same frame when environment decals are enabled. Restore both
     * original subsystem transitions instead of fabricating an LRUV cache. */
    if (!vita_decals_ready) {
        vita_log("[VITA 036D] original decals initialization begin");
        decals_initialize();
        decals_initialize_for_new_map();
        vita_decals_ready = TRUE;
        vita_log("[VITA 036D] original decals initialization/new-map PASS");
    }

    vita_renderer_ready = TRUE;
    vita_log("[VITA 036] original Xbox rasterizer ready; D3D8 device=%p", global_d3d_device);
    return 1;
}

int halo_vita_renderer_render_menu_frame(void)
{
    struct rasterizer_frame_begin_parameters frame_parameters;
    struct rasterizer_window_begin_parameters window_parameters;
    real_point3d position = { 0.0f, 0.0f, 0.0f };
    real_vector3d forward = { 0.0f, 0.0f, 1.0f };
    real_vector3d up = { 0.0f, 1.0f, 0.0f };

    if (!vita_renderer_ready || !global_d3d_device) {
        vita_log("MAIN MENU BLOCKED: render frame requested before rasterizer initialization");
        return 0;
    }

    csmemset(&frame_parameters, 0, sizeof(frame_parameters));
    csmemset(&window_parameters, 0, sizeof(window_parameters));

    /* This is the camera construction from main_pregame_render(). */
    window_parameters.camera.position = position;
    window_parameters.camera.forward = forward;
    window_parameters.camera.up = up;
    window_parameters.camera.mirrored = FALSE;
    window_parameters.camera.vertical_field_of_view =
        2.0f * arctangent(
            0.75f * render_camera_get_adjusted_field_of_view_tangent(
                DEGREES_TO_RADIANS(80.0f)),
            1.0f);
    compute_window_bounds(
        0,
        1,
        &window_parameters.camera.viewport_bounds,
        &window_parameters.camera.window_bounds);
    window_parameters.camera.z_near = 0.01f;
    window_parameters.camera.z_far = 1.0f;
    window_parameters.rasterizer_target = 0;
    window_parameters.window_index = NONE;

    /* render_frame_pregame() builds both render and rasterizer frusta before
     * entering the window. Keep the same state without pulling unrelated
     * sound/loading-screen work into the Vita Main Menu milestone. */
    render.frame_index++;
    render.camera = window_parameters.camera;
    render_camera_build_frustum(&render.camera, NULL, &render.frustum, TRUE);
    render_camera_build_frustum(
        &window_parameters.camera, NULL, &window_parameters.frustum, TRUE);

    if (vita_first_menu_frame)
        vita_log("[VITA 037] original Main Menu frame begin");

    rasterizer_frame_begin(&frame_parameters);
    if (vita_first_menu_frame)
        vita_log("[VITA 037F] rasterizer_frame_begin PASS");

    rasterizer_windows_begin();
    if (vita_first_menu_frame)
        vita_log("[VITA 037W0] rasterizer_windows_begin PASS");

    rasterizer_window_begin(&window_parameters);
    if (vita_first_menu_frame)
        vita_log("[VITA 037W1] rasterizer_window_begin PASS");

    if (vita_first_menu_frame)
        vita_log("[VITA 037UI] render_ui_widgets begin");
    render_ui_widgets(0, &window_parameters.camera.viewport_bounds);
    if (vita_first_menu_frame)
        vita_log("[VITA 037UI] render_ui_widgets PASS");

    rasterizer_window_end();
    if (vita_first_menu_frame)
        vita_log("[VITA 037W2] rasterizer_window_end PASS");

    rasterizer_windows_end();
    if (vita_first_menu_frame)
        vita_log("[VITA 037W3] rasterizer_windows_end PASS");

    rasterizer_frame_end();
    if (vita_first_menu_frame)
        vita_log("[VITA 037E] rasterizer_frame_end PASS");

    rasterizer_present(NULL, NULL);

    if (vita_first_menu_frame) {
        vita_log("[VITA 038] original Main Menu frame presented");
        vita_first_menu_frame = FALSE;
    }
    return 1;
}
