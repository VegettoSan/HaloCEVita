/* Vita entry point for the original Halo rasterizer.
 * Preserve Halo's shell initialization order after the Vita platform and
 * physical-memory arena are ready. Menu frames use the same rasterizer frame /
 * window / UI / present sequence as render_frame_pregame; this file only
 * supplies the pregame camera that main_pregame_render normally builds. */
#include <xtl.h>
#include "cseries.h"
#include "math/real_math.h"
#include "main/main.h"
#include "render/render.h"
#include "render/render_cameras.h"
#include "interface/ui_widget.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/xbox/rasterizer_xbox.h"
#include "vita_runtime.h"

/* January keeps this helper private to main.c; the pregame path uses it to
 * derive the same full-screen and title-safe rectangles used by retail. */
void compute_window_bounds(long player_index, long num_players,
    rectangle2d *pixel_bounds, rectangle2d *safe_frame_bounds);

static boolean vita_renderer_ready;
static boolean vita_first_menu_frame = TRUE;

int halo_vita_renderer_initialize(void)
{
    if (vita_renderer_ready) return 1;

    /* Physical/game-state ownership belongs to halo_vita_memory_initialize().
     * The Vita bring-up already allocates Halo's game-state buffer before the
     * UI cache is mounted. Calling game_state_initialize() here allocates that
     * buffer a second time and correctly trips January's buffer_allocated
     * assertion. Keep renderer initialization limited to the rasterizer. */
    vita_log("[VITA 033] original game-state already owned by memory bring-up");
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
    rasterizer_windows_begin();
    rasterizer_window_begin(&window_parameters);
    render_ui_widgets(0, &window_parameters.camera.viewport_bounds);
    rasterizer_window_end();
    rasterizer_windows_end();
    rasterizer_frame_end();
    rasterizer_present(NULL, NULL);

    if (vita_first_menu_frame) {
        vita_log("[VITA 038] original Main Menu frame presented");
        vita_first_menu_frame = FALSE;
    }
    return 1;
}
