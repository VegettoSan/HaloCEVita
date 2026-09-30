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
static char resource_map_path[4] = "map";
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
    for (i = 0; i < 10; ++i) {
        fixture();
        switch (i) {
        case 0: record.flags = 0; break;
        case 1: record.tag_index++; break;
        case 2: record.cache_block_index = 0; break;
        case 3: record.base_address = &record; break;
        case 4: record.hardware_format = &record; break;
        case 5: record.pixels_offset = -1; break;
        case 6: record.pixels_size = 0; break;
        case 7: record.pixels_offset = 32700; break;
        case 8: record.width = 0; break;
        case 9: record.pixels_offset = 2047; break;
        }
        saved = record;
        assert(!halo_vita_menu_bitmap_resources_activate());
        assert(!vita_menu_bitmap_resources_activated);
        assert(!memcmp(&saved, &record, sizeof(record)));
    }
    fixture(); record.pixels_offset = 32768 - 128;
    assert(halo_vita_menu_bitmap_resources_activate());
    puts("PASS: actual compiled-bitmap validator preserves records; 10 invalid state/range cases and exact EOF");
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
