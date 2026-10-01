/* Vita entry point for the original Halo rasterizer.
 * Preserve Halo's shell initialization order after the Vita platform and
 * physical-memory arena are ready. Menu frames use the same rasterizer frame /
 * window / UI / present sequence as render_frame_pregame; this file only
 * supplies the pregame camera that main_pregame_render normally builds. */
#include <xtl.h>
#include "cseries.h"
#include "cache/texture_cache.h"
#include "effects/decals.h"
#include "game/game.h"
#include "game/players.h"
#include "cutscene/cinematics.h"
#include "math/real_math.h"
#include "main/main.h"
#include "render/render.h"
#include "render/render_cameras.h"
#include "interface/ui_widget.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/xbox/rasterizer_xbox.h"
#include "vita_runtime.h"
#include "halo_ui_pointer.h"

/* Keep vitaGL headers out of Halo's MSVC-semantics translation unit. The
 * diagnostic uses only ABI-stable GLES scalar entry points and constants. */
#ifdef HALO_VITA
extern void glGetIntegerv(unsigned int pname, int *data);
extern void glReadPixels(int x, int y, int width, int height,
    unsigned int format, unsigned int type, void *data);
extern unsigned int glGetError(void);
#define VITA_GL_VIEWPORT 0x0BA2u
#define VITA_GL_RGBA 0x1908u
#define VITA_GL_UNSIGNED_BYTE 0x1401u
#endif

/* cache_files.c owns the map-open sequencing and intentionally keeps this
 * per-map entry point out of texture_cache.h. Vita mounts ui.map through its
 * checked logical-range backend instead of scenario_tags_load(), so restore
 * that per-map transition after rasterizer_initialize() has created the
 * process-lifetime Xbox texture cache. */
void texture_cache_open(void);
void texture_cache_close(void);
/* January declares these original disposal helpers privately in game.c. */
void players_dispose_from_old_map(void);
void players_dispose(void);

/* Validate the already prepared compiled-cache records before the first draw.
 * scenario_tags_load does not repeat the tag-building bitmap postprocess. */
int halo_vita_menu_bitmap_resources_activate(void);

/* January keeps this helper private to main.c; the pregame path uses it to
 * derive the same full-screen and title-safe rectangles used by retail. */
void compute_window_bounds(long player_index, long num_players,
    rectangle2d *pixel_bounds, rectangle2d *safe_frame_bounds);
/* Original render.c also declares this main.c helper locally. */
short main_get_window_count(void);

static boolean vita_renderer_ready;
static boolean vita_rasterizer_initialized;
static boolean vita_texture_cache_opened;
static boolean vita_bitmap_resources_ready;
static boolean vita_decals_ready;
static boolean vita_shell_state_ready;
static boolean vita_first_menu_frame = TRUE;
static unsigned long vita_menu_frame_count;

#ifdef HALO_VITA
/* Sample nine small blocks from the real currently-bound Halo target. This is
 * readback-only diagnostics: it neither clears nor replaces the retail frame.
 * A nonzero RGB count proves rasterization reached the 320x240 source before
 * the D3D8 Present upscale; zero isolates the problem before presentation. */
static void vita_probe_real_menu_target(void)
{
    unsigned char pixels[8 * 8 * 4];
    int viewport[4] = {0, 0, 0, 0};
    unsigned long nonblack = 0, samples = 0;
    unsigned long r = 0, g = 0, b = 0, a = 0;
    int gx, gy, x, y, i;
    unsigned int error;

    glGetIntegerv(VITA_GL_VIEWPORT, viewport);
    if (viewport[2] < 8 || viewport[3] < 8) {
        vita_log("[VITA PIXEL] source probe skipped viewport=%d,%d,%d,%d",
            viewport[0], viewport[1], viewport[2], viewport[3]);
        return;
    }

    for (gy = 1; gy <= 3; gy++) {
        for (gx = 1; gx <= 3; gx++) {
            x = viewport[0] + (viewport[2] * gx) / 4 - 4;
            y = viewport[1] + (viewport[3] * gy) / 4 - 4;
            glReadPixels(x, y, 8, 8, VITA_GL_RGBA, VITA_GL_UNSIGNED_BYTE, pixels);
            for (i = 0; i < 8 * 8; i++) {
                unsigned char *p = &pixels[i * 4];
                r += p[0]; g += p[1]; b += p[2]; a += p[3];
                if (p[0] || p[1] || p[2]) nonblack++;
                samples++;
            }
        }
    }
    error = glGetError();
    vita_log("[VITA PIXEL] real Halo target viewport=%d,%d,%d,%d samples=%lu nonblack=%lu rgba_sum=%lu,%lu,%lu,%lu gl_error=0x%x",
        viewport[0], viewport[1], viewport[2], viewport[3], samples, nonblack,
        r, g, b, a, error);
}
#endif

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
    vita_rasterizer_initialized = TRUE;

    /* Restore only the per-map transition that the Vita ui.map mount skipped.
     * No pixels or menu assets are supplied here; bitmap reads still flow
     * through Halo's original texture cache and cache_file_read backend. */
    if (!vita_texture_cache_opened) {
        vita_log("[VITA 036T] original Xbox texture cache per-map open begin");
        texture_cache_open();
        vita_texture_cache_opened = TRUE;
        vita_log("[VITA 036T] original Xbox texture cache per-map open PASS");
    }

    /* Keep scenario_tags_load's compiled-cache contract: no bitmap_new or
     * offset adjustment. Pixel bytes are loaded lazily by the original Xbox
     * texture cache through cache_file_read, using the stored absolute offsets. */
    if (!vita_bitmap_resources_ready) {
        vita_log("[VITA 039] mounted ui.map bitmap runtime activation begin");
        if (!halo_vita_menu_bitmap_resources_activate()) {
            vita_log("MAIN MENU BLOCKED: mounted ui.map bitmaps could not enter original texture cache");
            return 0;
        }
        vita_bitmap_resources_ready = TRUE;
    }

    /* window_end -> main_get_window_count queries both cinematic and player
     * globals even for one pregame window. Preserve game_initialize's owners
     * and each subsystem's new-map reset, rather than replacing those queries
     * with a constant or allowing a NULL game-state pointer at first present.
     * The clock remains inactive and no local players/input are created. */
    if (!vita_shell_state_ready) {
        vita_log("[VITA 036S] original pregame clock/player/cinematic state begin");
        game_time_initialize();
        game_time_initialize_for_new_map();
        players_initialize();
        players_initialize_for_new_map();
        cinematic_initialize();
        cinematic_initialize_for_new_map();
        vita_shell_state_ready = TRUE;
        vita_log("[VITA 036S] original pregame state PASS; windows=%d", main_get_window_count());
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

#ifdef HALO_VITA_MENU_AUDIO
    if (!halo_vita_menu_audio_initialize()) return 0;
#endif
    vita_log("[VITA WORLD] staged UI renderer; scenario BSP/models/HS camera and spatial game audio remain pending");
    vita_renderer_ready = TRUE;
    vita_log("[VITA 036] original Xbox rasterizer ready; D3D8 device=%p", global_d3d_device);
    return 1;
}

/* Call only before root activation, while tags/arena/context remain owned.
 * Track the completed rasterizer separately from overall readiness: a bitmap
 * validation failure happens after its heap allocations but before ready. */
void halo_vita_renderer_dispose_before_root(void)
{
    if (!vita_rasterizer_initialized) return;
#ifdef HALO_VITA_MENU_AUDIO
    halo_vita_menu_audio_dispose();
#endif
    vita_log("[VITA 040] original pre-root renderer disposal begin");
    if (vita_texture_cache_opened) {
        texture_cache_close();
        vita_texture_cache_opened = FALSE;
    }
    if (vita_shell_state_ready) {
        cinematic_dispose();
        players_dispose_from_old_map();
        players_dispose();
        game_time_dispose_from_old_map();
        game_time_dispose();
        vita_shell_state_ready = FALSE;
    }
    if (vita_decals_ready) {
        decals_dispose_from_old_map();
        decals_dispose();
        vita_decals_ready = FALSE;
    }
    rasterizer_dispose();
    vita_rasterizer_initialized = FALSE;
    vita_renderer_ready = FALSE;
    vita_bitmap_resources_ready = FALSE;
    vita_log("[VITA 040] original pre-root renderer disposal PASS");
}

int halo_vita_renderer_render_menu_frame(void)
{
    struct rasterizer_frame_begin_parameters frame_parameters;
    struct rasterizer_window_begin_parameters window_parameters;
    real_point3d position = { 0.0f, 0.0f, 0.0f };
    real_vector3d forward = { 0.0f, 0.0f, 1.0f };
    real_vector3d up = { 0.0f, 1.0f, 0.0f };
    boolean observe_time;
    uint64_t frame_begin = 0, ui_end = 0, present_end = 0, audio_end = 0;

    if (!vita_renderer_ready || !global_d3d_device) {
        vita_log("MAIN MENU BLOCKED: render frame requested before rasterizer initialization");
        return 0;
    }

    halo_vita_main_render_time_update();
    halo_vita_ui_render_clock_update();
    vita_menu_frame_count++;
    observe_time = vita_menu_frame_count <= 3 || vita_menu_frame_count == 30 ||
        vita_menu_frame_count == 120;
    if (observe_time) frame_begin = vita_time_us();
    if (vita_menu_frame_count == 2)
        vita_log("[VITA 041] second original Main Menu frame begin");
    else if (vita_menu_frame_count == 3)
        vita_log("[VITA 041] third original Main Menu frame begin");

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
    /* render_frame_pregame zero-initializes this index. NONE means an overlay
     * on a previous scene: the Xbox rasterizer would suppress the first
     * window's pool/dynamic-geometry begin calls. This is a standalone frame. */
    window_parameters.window_index = 0;

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
    /* Native render_frame_pregame centers Halo's authored 640-column UI in
     * the selected widescreen viewport through the backend's screen_offset.
     * The same flag also centers scissor rectangles; do not offset tags. */
    halo_screen_ui_offset(TRUE);
    render_ui_widgets(0, &window_parameters.camera.viewport_bounds);
    halo_screen_ui_offset(FALSE);
    if (observe_time) ui_end = vita_time_us();
    if (vita_first_menu_frame)
        vita_log("[VITA 037UI] render_ui_widgets PASS");

    rasterizer_window_end();
    if (vita_first_menu_frame)
        vita_log("[VITA 037W2] rasterizer_window_end PASS");

    rasterizer_windows_end();
    if (vita_first_menu_frame)
        vita_log("[VITA 037W3] rasterizer_windows_end PASS");

    rasterizer_frame_end();
    if (vita_first_menu_frame) {
        vita_log("[VITA 037E] rasterizer_frame_end PASS");
#ifdef HALO_VITA
        vita_probe_real_menu_target();
#endif
    }

    rasterizer_present(NULL, NULL);
    if (observe_time) present_end = vita_time_us();
#ifdef HALO_VITA_MENU_AUDIO
    /* First-frame shader compilation can take seconds on Vita. Queue the
     * original soundtrack after that frame, so its initial packets cannot
     * drain while the main thread is still compiling the first UI shaders. */
    halo_vita_menu_audio_frame();
#endif
    if (observe_time) {
        audio_end = vita_time_us();
        vita_log("[VITA FRAME TIME] frame=%lu begin_ui_us=%llu end_present_us=%llu audio_us=%llu total_us=%llu",
            vita_menu_frame_count, (unsigned long long)(ui_end - frame_begin),
            (unsigned long long)(present_end - ui_end),
            (unsigned long long)(audio_end - present_end),
            (unsigned long long)(audio_end - frame_begin));
    }

    if (vita_first_menu_frame) {
        vita_log("[VITA 038] original Main Menu frame presented");
        vita_first_menu_frame = FALSE;
    } else if (vita_menu_frame_count == 2) {
        vita_log("[VITA 041] second original Main Menu frame presented");
    } else if (vita_menu_frame_count == 3) {
        vita_log("[VITA 041] third original Main Menu frame presented");
    } else if (vita_menu_frame_count == 30 || vita_menu_frame_count == 120) {
        vita_log("[VITA 041] original Main Menu loop alive frame=%lu", vita_menu_frame_count);
    }
    return 1;
}
