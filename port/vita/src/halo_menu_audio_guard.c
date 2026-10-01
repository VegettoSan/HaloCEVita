/* Explicit unavailable-world guards for the UI-only 2D audio link closure.
 * Never return fake spatial/audio results. Remove these link wraps when the
 * original scenario/objects/observer/debug owners are initialized. */
#include "cseries.h"
#include "cseries_windows.h"
#include <xtl.h>
#include "vita_runtime.h"
#include "sound/game_sound.h"
#include "camera/observer.h"
#include "scenario/scenario.h"
#include "effects/player_effects.h"
#include "render/render_debug.h"
#include "sound/sound_manager.h"

/* Compiler-check wrap signatures against their original owner declarations. */
__typeof__(compute_sound_obstruction) __wrap_compute_sound_obstruction;
__typeof__(track_object_impulse_sound) __wrap_track_object_impulse_sound;
__typeof__(game_sound_set_mouth_aperture) __wrap_game_sound_set_mouth_aperture;
__typeof__(player_effect_continuous_refresh) __wrap_player_effect_continuous_refresh;
__typeof__(observer_get_camera) __wrap_observer_get_camera;
__typeof__(scenario_location_underwater) __wrap_scenario_location_underwater;
__typeof__(render_debug_sphere) __wrap_render_debug_sphere;
__typeof__(render_debug_string_at_point) __wrap_render_debug_string_at_point;
__typeof__(render_debug_string) __wrap_render_debug_string;
__typeof__(sound_render_time) __wrap_sound_render_time;
__typeof__(sound_idle) __wrap_sound_idle;
long __real_sound_render_time(void);
void __real_sound_idle(void);

/* Texture waits can occur before sound's map lifecycle has completed. Keep
 * the previous deferred-service contract until the real cache/manager exist. */
long __wrap_sound_render_time(void)
{ return halo_vita_menu_audio_ready() ? __real_sound_render_time() : (long)system_milliseconds(); }
void __wrap_sound_idle(void)
{ if (halo_vita_menu_audio_ready()) __real_sound_idle(); else SwitchToThread(); }

void __wrap_compute_sound_obstruction(short player, struct sound_source *source, real distance)
{ (void)player; (void)source; (void)distance; vita_fatal("3D sound obstruction requires original scenario audio owners"); }
boolean __wrap_track_object_impulse_sound(long object, void const *data, struct sound_source *source)
{ (void)object; (void)data; (void)source; vita_fatal("object sound tracking unavailable in staged 2D menu"); }
void __wrap_game_sound_set_mouth_aperture(long object, real aperture)
{ (void)object; (void)aperture; vita_fatal("object speech animation unavailable in staged 2D menu"); }
void __wrap_player_effect_continuous_refresh(long effect, real_point3d const *point)
{ (void)effect; (void)point; vita_fatal("looping sound damage effect requires original game owners"); }
struct observer_result const *__wrap_observer_get_camera(short player)
{ (void)player; vita_fatal("3D sound listener requires original observer initialization"); }
boolean __wrap_scenario_location_underwater(struct location const *location, real_point3d const *point, short *weather)
{ (void)location; (void)point; (void)weather; vita_fatal("3D sound water test requires original BSP initialization"); }
void __wrap_render_debug_sphere(boolean immediate, real_point3d const *point, real radius, real_argb_color const *color)
{ (void)immediate; (void)point; (void)radius; (void)color; vita_fatal("3D sound debug geometry unavailable in staged menu"); }
void __wrap_render_debug_string_at_point(boolean immediate, real_point3d const *point, const char *text, real_argb_color const *color)
{ (void)immediate; (void)point; (void)text; (void)color; vita_fatal("3D sound debug labels unavailable in staged menu"); }
void __wrap_render_debug_string(boolean immediate, const char *text)
{ (void)immediate; (void)text; vita_fatal("sound debug overlay requires original debug renderer owners"); }
