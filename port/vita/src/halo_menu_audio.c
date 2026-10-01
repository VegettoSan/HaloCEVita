/* Restore original sound cache/manager lifecycle skipped by the staged menu.
 * Samples stay in ui.map and are loaded by Xbox sound-cache/cache_file_read;
 * no authored music, WAV replacement or independent mixer is supplied. */
#include "cseries.h"
#include "vita_runtime.h"
#include "sound/sound_manager.h"
#include "sound/sound_definitions.h"
#include "sound/sound_classes.h"
#include "cache/sound_cache.h"
#include "cache/cache_files.h"
#include "game/game.h"
#include "interface/ui_widget.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"

/* Public original implementation has no declarations for these in its header. */
boolean sound_is_active(void);
boolean halo_vita_sound_menu_refresh(long definition_index, short refresh_state);
static boolean ready;
static long music_index = NONE;
static boolean music_started;
int halo_vita_menu_audio_ready(void) { return ready; }

int halo_vita_menu_audio_initialize(void)
{
    struct tag_iterator iterator;
    long index, permutations = 0;
    if (ready) return 1;
    if (game_in_progress()) vita_fatal("staged 2D menu audio cannot own running-game sound");
    /* Compiled caches already contain runtime metadata: never add file offsets
     * again or reinterpret external sample address fields as tag pointers. */
    tag_iterator_new(&iterator, SOUND_DEFINITION_TAG);
    while ((index = tag_iterator_next(&iterator)) != NONE) {
        struct sound_definition *definition = sound_definition_get(index);
        long r;
        for (r = 0; r < definition->pitch_ranges.count; ++r) {
            struct sound_pitch_range *range = TAG_BLOCK_GET_ELEMENT(&definition->pitch_ranges, r, struct sound_pitch_range);
            long n;
            if (range->actual_permutation_count < 1 || range->actual_permutation_count > range->permutations.count || range->actual_permutation_count > 32)
                vita_fatal("compiled menu sound permutation range invalid");
            for (n = 0; n < range->permutations.count; ++n) {
                struct sound_permutation *p = TAG_BLOCK_GET_ELEMENT(&range->permutations, n, struct sound_permutation);
                if (p->unknown0 != NONE || p->unknown1 != 0 || p->samples.size < 0 || p->samples.size > MAXIMUM_SOUND_DATA_SIZE || p->samples.file_offset < 0) {
                    vita_log("[VITA AUDIO] invalid cold permutation: tag=%08lx range=%ld permutation=%ld block=%08lx base=%08lx bytes=%ld offset=%ld", (unsigned long)index,r,n,(unsigned long)p->unknown0,p->unknown1,p->samples.size,p->samples.file_offset);
                    vita_fatal("compiled sound cache cannot be activated with stale runtime metadata");
                }
                sound_cache_sound_new(index, p);
                ++permutations;
            }
        }
    }
    vita_log("[VITA AUDIO] original sound classes/cache/manager initialization begin: permutations=%ld", permutations);
    sound_classes_initialize();
    sound_classes_initialize_for_new_map();
    sound_initialize();
    if (!sound_is_active()) vita_fatal("original sound manager initialization failed");
    sound_cache_open();
    sound_initialize_for_new_map();
    ready = TRUE;
    vita_log("[VITA AUDIO] original 2D sound manager/cache ready; audibility requires console test");
    return 1;
}
int halo_vita_menu_audio_start(uint32_t index)
{
    if (!ready) { vita_log("MENU AUDIO DEFERRED: original sound manager not ready"); return 0; }
    if (music_index == (long)index) return 1;
    if (music_index != NONE) halo_vita_menu_audio_stop();
    music_index = (long)index;
    music_started = FALSE;
    vita_log("[VITA AUDIO] original menu loop armed for completed render frame: tag=%08lx path=%s", (unsigned long)music_index, tag_get_name(music_index));
    return 1;
}
void halo_vita_menu_audio_stop(void)
{
    if (ready && music_started && music_index != NONE) halo_vita_sound_menu_refresh(music_index, _looping_sound_refresh_stop);
    music_index = NONE;
    music_started = FALSE;
}
void halo_vita_menu_audio_frame(void)
{
    if (!ready) return;
    if (game_in_progress()) vita_fatal("staged 2D menu sound must hand off to original game audio");
    if (music_index != NONE) {
        halo_vita_sound_menu_refresh(music_index, music_started ?
            _looping_sound_refresh_loop : _looping_sound_refresh_start);
        music_started = TRUE;
    }
    sound_render();
}
void halo_vita_menu_audio_feedback_probe(void)
{
    static short feedback = 1; /* original private enum: cursor/forward/back */
    if (!ready) return;
    vita_log("[VITA AUDIO] Square diagnostic: original UI feedback=%d; widget navigation still pending", feedback);
    ui_play_audio_feedback_sound(feedback);
    feedback = feedback == 3 ? 1 : feedback + 1;
}
void halo_vita_menu_audio_dispose(void)
{
    if (!ready) return;
    halo_vita_menu_audio_stop();
    sound_stop_all();
    sound_dispose_from_old_map();
    sound_cache_close();
    sound_dispose();
    sound_classes_dispose();
    ready = FALSE;
    halo_vita_audio_mixer_shutdown();
    vita_log("[VITA AUDIO] original manager/cache disposed before tag release");
}
