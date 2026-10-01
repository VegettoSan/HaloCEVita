#!/usr/bin/env python3
"""Exercise actual compiled-bitmap validation and renderer pair cache on host.

The GPU/tag accessors are mocked: this checks ownership, immutability, pair
ordering, cache identity and failure cleanup, not shader acceptance or rendering.
"""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/vita/tests/renderer-contracts'
OUT.mkdir(parents=True, exist_ok=True)


def run(name, prefix, body, suffix):
    source = OUT / (name + '.c')
    executable = OUT / name
    source.write_text(prefix + body + suffix)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                    str(source), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)


COMMON = '''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TRUE 1
#define FALSE 0
#define NONE (-1L)
typedef int boolean;
static void vita_log(const char *format, ...) { (void)format; }
'''

bitmap = (ROOT / 'port/vita/src/halo_menu_tags.c').read_text()
bitmap = bitmap[bitmap.index('int halo_vita_menu_bitmap_resources_activate(void)'):
                bitmap.index('int halo_vita_menu_root_checkpoint(void)')]
reader = (ROOT / 'port/vita/src/vita_cache_read.c').read_text()
range_body = reader[reader.index('int vita_cache_resource_range_valid('):
                    reader.index('int vita_cache_resource_read(')]
run('bitmap', COMMON + '''
#define BITMAP_GROUP_TAG 0x6269746d
#define CACHE_HEADER_SIZE 2048
#define TEST_FLAG(flags, bit) ((flags) & (1U << (bit)))
enum { _vita_bitmap_cached_bit = 7 };
struct tag_iterator { int visited; };
struct bitmap_data {
    unsigned flags;
    long tag_index, cache_block_index, pixels_offset, pixels_size;
    void *base_address, *hardware_format;
    short format, width, height, depth;
};
struct block { long count; struct bitmap_data *address; };
struct bitmap_group { struct block bitmaps; };
static struct bitmap_data record;
static struct bitmap_group group;
static int scenario, globals;
static void *vita_ui_scenario = &scenario, *vita_ui_game_globals = &globals;
static boolean vita_menu_bitmap_resources_activated;
static FILE *resource_file = (FILE *)1;
static uint32_t resource_map_logical_size = 32768;
static const long owner = 0xe1780004;
static void tag_iterator_new(struct tag_iterator *i, long tag) {
    assert(tag == BITMAP_GROUP_TAG); i->visited = 0;
}
static long tag_iterator_next(struct tag_iterator *i) { return i->visited++ ? NONE : owner; }
static struct bitmap_group *bitmap_group_get(long i) { assert(i == owner); return &group; }
static const char *tag_get_name(long i) { assert(i == owner); return "synthetic"; }
#define TAG_BLOCK_GET_ELEMENT(b, i, type) ((b)->address + (i))
static int bitmap_verify(struct bitmap_data *b, int importing) {
    assert(b == &record && !importing); return b->width > 0;
}
''' + range_body, bitmap, '''
static void fixture(void) {
    memset(&record, 0, sizeof(record));
    record.flags = 0x80; record.tag_index = owner; record.cache_block_index = NONE;
    record.pixels_offset = 8192; record.pixels_size = 128; record.width = 4;
    group.bitmaps.count = 1; group.bitmaps.address = &record;
    vita_menu_bitmap_resources_activated = FALSE;
}
int main(void) {
    struct bitmap_data saved; int i;
    fixture(); saved = record;
    assert(halo_vita_menu_bitmap_resources_activate());
    assert(halo_vita_menu_bitmap_resources_activate()); /* idempotent */
    assert(!memcmp(&saved, &record, sizeof(record))); /* no double offset or handle mutation */
    for (i = 0; i < 8; ++i) {
        fixture();
        switch (i) {
        case 0: record.flags = 0; break;
        case 1: record.tag_index++; break;
        case 2: record.cache_block_index = 0; break;
        case 3: record.pixels_offset = -1; break;
        case 4: record.pixels_size = 0; break;
        case 5: record.pixels_offset = 32700; break;
        case 6: record.width = 0; break;
        case 7: record.pixels_offset = 2047; break;
        }
        saved = record;
        assert(!halo_vita_menu_bitmap_resources_activate());
        assert(!vita_menu_bitmap_resources_activated);
        assert(!memcmp(&saved, &record, sizeof(record)));
    }
    fixture(); record.flags = 0x83; record.base_address = (void *)(uintptr_t)0x024f0040;
    record.hardware_format = (void *)(uintptr_t)0xdead0040;
    record.pixels_offset = 4096; record.pixels_size = 2816;
    saved = record;
    assert(halo_vita_menu_bitmap_resources_activate());
    assert(!memcmp(&saved, &record, sizeof(record))); /* serialized pointers stay opaque */
    fixture(); record.pixels_offset = 32768 - 128;
    assert(halo_vita_menu_bitmap_resources_activate());
    puts("PASS: actual compiled-bitmap validator preserves records; 8 invalid state/range cases, serialized Xbox pointers and exact EOF");
}
''')

renderer = (ROOT / 'port/linux/src/d3d8_gl.c').read_text()
pair = renderer[renderer.index('static struct program_entry *vita_program_pair_get('):]
pair = pair[:pair.index('\n#endif')]
run('pair', COMMON.replace('static void vita_log(const char *format, ...) { (void)format; }', '') + '''
typedef unsigned GLuint;
typedef int BOOL;
#define GL_VERTEX_SHADER 1
#define GL_FRAGMENT_SHADER 2
#define FRAGMENT_BUCKETS 16
struct nv2a_pixel_shader_key { uint32_t words[8]; };
struct vertex_shader_object { unsigned long id, packed_mask, instruction_count; void *instructions; };
struct program_entry { int present; };
struct fragment_entry {
    struct fragment_entry *next;
    unsigned long hash, vertex_program_id, packed_mask;
    BOOL immediate;
    struct nv2a_pixel_shader_key key;
    struct program_entry *paired_program;
};
static struct fragment_entry *fragment_buckets[FRAGMENT_BUCKETS];
static struct { struct vertex_shader_object *vertex_shader; } device;
static unsigned compiles, links, deletes, failure, stage, generated_packed;
static struct program_entry linked;
static unsigned long hash_words(const void *bytes, unsigned long count) {
    const unsigned char *p = bytes; unsigned long hash = 0;
    while (count--) hash = hash * 37 + *p++;
    return hash;
}
static char *source_copy(void) { char *p = malloc(2); assert(p); memcpy(p,"x",2); return p; }
static char *nv2a_vertex_shader_to_glsl(void *instructions, unsigned long count, unsigned long packed) {
    assert(instructions && count); generated_packed = (unsigned)packed; return source_copy();
}
static char *nv2a_pixel_shader_to_glsl(const struct nv2a_pixel_shader_key *key) {
    assert(key); return source_copy();
}
static GLuint compile_shader(unsigned type, const char *source, const char *name) {
    assert(source && name && type == ++stage); ++compiles;
    if (type == GL_FRAGMENT_SHADER) stage = 0; /* translator semantic-pool cycle */
    return failure == type ? 0 : type;
}
static struct program_entry *program_get(GLuint vertex, GLuint pixel) {
    assert(vertex == 1 && pixel == 2 && !failure && stage == 0); ++links; return &linked;
}
static void glDeleteShader(GLuint shader) { assert(shader == 1 || shader == 2); ++deletes; }
''', pair, '''
int main(void) {
    struct vertex_shader_object program = { 10, 0, 1, &linked }, other = { 11, 0, 1, &linked };
    struct nv2a_pixel_shader_key key = {{0}};
    device.vertex_shader = &program;
    assert(vita_program_pair_get(&program,FALSE,&key) == &linked);
    assert(compiles == 2 && links == 1 && deletes == 2);
    assert(vita_program_pair_get(&program,FALSE,&key) == &linked && compiles == 2);
    key.words[4] = 99; /* new PS with cached VS still compiles a complete pair */
    assert(vita_program_pair_get(&program,FALSE,&key) == &linked && compiles == 4);
    assert(vita_program_pair_get(&other,FALSE,&key) == &linked && compiles == 6);
    program.packed_mask = 4;
    assert(vita_program_pair_get(&program,FALSE,&key) == &linked && compiles == 8 && generated_packed == 4);
    assert(vita_program_pair_get(&program,TRUE,&key) == &linked && compiles == 10 && generated_packed == 0);
    failure = GL_VERTEX_SHADER; key.words[0] = 1;
    assert(!vita_program_pair_get(&program,FALSE,&key));
    assert(compiles == 12 && links == 5 && deletes == 11 && stage == 0);
    assert(!vita_program_pair_get(&program,FALSE,&key) && compiles == 12);
    failure = GL_FRAGMENT_SHADER; key.words[0] = 2;
    assert(!vita_program_pair_get(&program,FALSE,&key));
    assert(compiles == 14 && links == 5 && deletes == 12 && stage == 0);
    puts("PASS: actual NV2A pair cache orders both stages, distinguishes VS/PS/declaration/immediate and caches safe failures");
}
''')

# Execute the original game-state owners and the exact window-count queries.
# Only allocation/data handles are mocked; no query returns a constant.
def original(path, signature):
    source = (ROOT / path).read_text()
    start = source.rindex(signature)
    brace = source.index('{', start)
    level = 1
    end = brace + 1
    while level:
        level += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

owners = [
    ('source/game/player_control.c', 'void player_control_initialize('),
    ('source/game/player_control.c', 'void player_control_dispose('),
    ('source/game/players.c', 'void players_initialize('),
    ('source/game/players.c', 'void players_initialize_for_new_map('),
    ('source/cutscene/cinematics.c', 'void cinematic_initialize('),
    ('source/cutscene/cinematics.c', 'void cinematic_initialize_for_new_map('),
    ('source/game/game_time.c', 'void game_time_initialize('),
    ('source/game/game_time.c', 'void game_time_initialize_for_new_map('),
    ('source/game/game_time.c', 'boolean game_time_initialized('),
    ('source/game/game_time.c', 'long game_time_get('),
    ('source/game/players.c', 'short local_player_count('),
    ('source/cutscene/cinematics.c', 'boolean cinematic_in_progress('),
    ('source/game/game_engine.c', 'boolean game_engine_force_single_screen('),
    ('source/main/main.c', 'short main_get_window_count('),
]
run('shell-state', COMMON.replace('static void vita_log(const char *format, ...) { (void)format; }', '') + '''
#define csmemset memset
#define match_assert(file, line, expression) assert(expression)
#define PIN(x, lo, hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
#define MAXIMUM_WINDOWS 4
struct player_datum { char unused[16]; };
struct data_array { int valid; };
struct players_globals {
    long local_players[4], dead_units[4], unknown0;
    int local_player_count, input_disabled, double_speed_ticks, all_dead;
    long pending_teleport_starting_location_index, respawn_failure;
};
struct cinematic_global_data {
    int in_progress; struct { long title_index, time; } queued_titles[4];
};
struct game_time_globals_struct { int initialized, active; long local_time; };
struct control_state { char unused[64]; };
static struct control_state *player_control_globals;
static struct players_globals *players_globals;
static struct cinematic_global_data *cinematic_globals;
static struct game_time_globals_struct *game_time_globals;
static struct data_array *player_data, *team_data;
static long machine_to_player_table[16];
static void *game_engine;
static struct { int postgame_state; } game_engine_globals;
static void *allocations[8]; static int allocation_count;
static void *game_state_malloc(const char *name, const char *kind, size_t bytes) {
    void *p = malloc(bytes); (void)name; (void)kind;
    assert(p && allocation_count < 8); memset(p, 0xa5, bytes);
    allocations[allocation_count++] = p; return p;
}
static struct data_array *game_state_data_new(const char *name, int count, size_t size) {
    assert(count == 16 && size > 0);
    return game_state_malloc(name, "data", sizeof(struct data_array));
}
static void data_make_valid(struct data_array *p) { p->valid = 1; }
''', ''.join(original(path, sig) for path, sig in owners), '''
int main(void) {
    int i;
    game_time_initialize(); game_time_initialize_for_new_map();
    players_initialize(); players_initialize_for_new_map();
    cinematic_initialize(); cinematic_initialize_for_new_map();
    assert(game_time_initialized() && game_time_get() == 0 && !game_time_globals->active);
    assert(player_data->valid && team_data->valid && local_player_count() == 0);
    assert(player_control_globals && !cinematic_in_progress());
    assert(main_get_window_count() == 1); /* exact first window_end query */
    for (i = 0; i < 4; ++i) {
        assert(players_globals->local_players[i] == NONE);
        assert(cinematic_globals->queued_titles[i].title_index == NONE);
    }
    players_globals->local_player_count = 2;
    assert(main_get_window_count() == 2);
    cinematic_globals->in_progress = TRUE;
    assert(main_get_window_count() == 1);
    cinematic_initialize_for_new_map();
    assert(main_get_window_count() == 2);
    players_initialize_for_new_map();
    assert(main_get_window_count() == 1);
    for (i = 0; i < allocation_count; ++i) free(allocations[i]);
    puts("PASS: original shell state owns valid inactive clock/player/cinematic globals; exact window queries and resets");
    return 0;
}
''')


# Cold compiled records can contain obsolete Xbox pointer words. Run the
# original cached query/load bodies: the allocator/read and GPU registration
# boundary are mocked, but the pointer replacement/branch ordering is actual.
run('cached-load', COMMON.replace('static void vita_log(const char *format, ...) { (void)format; }', '') + r'''
#define TEST_FLAG(flags, bit) ((flags) & (1U << (bit)))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define match_assert(file, line, expression) assert(expression)
typedef unsigned char byte;
enum { _bitmap_cached_bit = 7 };
struct bitmap_data {
    unsigned flags; long cache_block_index, tag_index, pixels_offset, pixels_size;
    void *base_address, *hardware_format;
};
struct xbox_texture_cache_texture {
    struct bitmap_data *bitmap; int hardware_format, loaded, used; long read_request_handle;
};
static byte live_pixels[4096];
static struct xbox_texture_cache_texture resident;
static struct { byte *base_address; void *cache, *textures; } xbox_texture_cache_globals;
static boolean debug_texture_cache;
static unsigned long texture_cache_last_failure_time;
static void *global_real_argb_purple;
static int load_count, registration_count, read_count, touch_count;
static long rasterizer_xbox_bitmap_get_pixel_data_size(struct bitmap_data *b) {
    return b->pixels_size;
}
static long lruv_block_new(void *cache, long size) {
    assert(cache && size == 2816); ++load_count; return 7;
}
static long lruv_block_get_address(void *cache, long index) { assert(cache && index == 7); return 0; }
static long datum_new_at_index(void *data, long index) { assert(data && index == 7); return index; }
static void *datum_get(void *data, long index) { assert(data && index == 7); return &resident; }
static void texture_cache_initialize_hardware_format(struct bitmap_data *b, int *format) {
    assert(b->base_address == live_pixels && b->cache_block_index == 7);
    ++registration_count; *format = 42; /* real initializer registers the just-replaced base */
}
static long cache_file_read(long tag, long offset, long size, void *dest, int *loaded, int block) {
    assert(tag == 0xe1780004L && offset == 4096 && size == 2816);
    assert(dest == live_pixels && block); memset(dest, 0x5a, (size_t)size);
    ++read_count; *loaded = TRUE; return 13;
}
static void lruv_block_touch(void *cache, long index) { assert(cache && index == 7); ++touch_count; }
static void console_warning(const char *format, ...) { (void)format; assert(0); }
static const char *tag_get_name(long tag) { (void)tag; return "synthetic"; }
static void cache_file_promote_read(long request) { (void)request; assert(0); }
static unsigned long system_milliseconds(void) { return 0; }
static unsigned long sound_render_time(void) { return 0; }
static void sound_idle(void) { assert(0); }
static void SwitchToThread(void) { assert(0); }
static void terminal_printf(void *color, const char *format) { (void)color; (void)format; assert(0); }
#define _error_silent 0
static void error(int level, const char *message) { (void)level; (void)message; assert(0); }
static void scenario_debug_to_file(void) {}
static void texture_cache_name_block_proc(void) {}
static void lruv_debug_to_file(const char *path, const char *name, long size, void *cache,
    void (*scenario)(void), void (*block_name)(void)) {
    (void)path; (void)name; (void)size; (void)cache; (void)scenario; (void)block_name; assert(0);
}
static void *rasterizer_get_bitmap_default_hardware_format(struct bitmap_data *b) { (void)b; assert(0); return NULL; }
''', original('source/cache/xbox_texture_cache.c', 'static boolean texture_cache_start_loading_bitmap(')
     + original('source/cache/xbox_texture_cache.c', 'void *_texture_cache_bitmap_get_hardware_format('), r'''
int main(void) {
    struct bitmap_data b = {0x83, NONE, 0xe1780004L, 4096, 2816,
        (void *)(uintptr_t)0x024f0040, (void *)(uintptr_t)0xdead0040};
    struct bitmap_data saved = b;
    int placeholder;
    xbox_texture_cache_globals.base_address = live_pixels;
    xbox_texture_cache_globals.cache = xbox_texture_cache_globals.textures = &placeholder;
    assert(!_texture_cache_bitmap_get_hardware_format(&b, FALSE, FALSE));
    assert(!memcmp(&b, &saved, sizeof(b))); /* no-load never reads serialized handles */
    assert(_texture_cache_bitmap_get_hardware_format(&b, TRUE, TRUE) == &resident.hardware_format);
    assert(b.base_address == live_pixels && b.hardware_format == saved.hardware_format);
    assert(b.pixels_offset == saved.pixels_offset && b.pixels_size == saved.pixels_size && b.flags == saved.flags);
    assert(resident.bitmap == &b && resident.loaded && resident.used && live_pixels[2815] == 0x5a);
    assert(_texture_cache_bitmap_get_hardware_format(&b, TRUE, TRUE) == &resident.hardware_format);
    assert(load_count == 1 && registration_count == 1 && read_count == 1 && touch_count == 2);
    puts("PASS: actual cached query/load replaces serialized Xbox base before registration/read; ignores serialized hardware; resident reuse");
    return 0;
}
''')


run('pre-root-dispose', COMMON + r'''
static boolean vita_rasterizer_initialized, vita_renderer_ready;
static boolean vita_texture_cache_opened, vita_shell_state_ready;
static boolean vita_decals_ready, vita_bitmap_resources_ready;
static char calls[32]; static unsigned call_count;
static boolean tags_owned = TRUE, arena_owned = TRUE, context_owned = TRUE;
static void step(char code) {
    assert(tags_owned && arena_owned && context_owned && vita_rasterizer_initialized);
    calls[call_count++] = code;
}
static void texture_cache_close(void) { step('C'); }
static void cinematic_dispose(void) { step('c'); }
static void players_dispose_from_old_map(void) { step('P'); }
static void players_dispose(void) { step('p'); }
static void game_time_dispose_from_old_map(void) { step('T'); }
static void game_time_dispose(void) { step('t'); }
static void decals_dispose_from_old_map(void) { step('D'); }
static void decals_dispose(void) { step('d'); }
static void rasterizer_dispose(void) { step('R'); }
''', original('port/vita/src/halo_renderer_runtime.c', 'void halo_vita_renderer_dispose_before_root(void)'), r'''
int main(void) {
    halo_vita_renderer_dispose_before_root(); assert(call_count == 0);
    /* Observed039 failure: rasterizer initialized, overall ready false. */
    vita_rasterizer_initialized = vita_texture_cache_opened = TRUE;
    halo_vita_renderer_dispose_before_root();
    assert(call_count == 2 && !memcmp(calls, "CR", 2));
    assert(!vita_rasterizer_initialized && !vita_texture_cache_opened && !vita_renderer_ready);
    halo_vita_renderer_dispose_before_root(); assert(call_count == 2);
    call_count = 0;
    vita_rasterizer_initialized = vita_texture_cache_opened = TRUE;
    vita_renderer_ready = vita_bitmap_resources_ready = TRUE;
    vita_shell_state_ready = vita_decals_ready = TRUE;
    halo_vita_renderer_dispose_before_root();
    assert(call_count == 9 && !memcmp(calls, "CcPpTtDdR", 9));
    assert(!vita_rasterizer_initialized && !vita_texture_cache_opened && !vita_renderer_ready);
    assert(!vita_shell_state_ready && !vita_decals_ready && !vita_bitmap_resources_ready);
    halo_vita_renderer_dispose_before_root(); assert(call_count == 9);
    puts("PASS: pre-root failure disposes completed owners before tags/arena/context release; unopened and repeated shutdown guarded");
    return 0;
}
''')
