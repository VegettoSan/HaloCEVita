#!/usr/bin/env python3
"""Execute actual text/font consumers through original accessors and Vita resolver.

Host projections exercise ownership and bytes; they do not validate the ARM ABI
or GPU. No retail data is required. Legacy consumers must fault on a protected
serialized-address page; fixed consumers must use a separate native image.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def definition(source, name):
    match = re.search(r'(?m)^(?:static\s+)?(?:void|char|wchar_t|byte)\s*\*?\s*'
                      + re.escape(name) + r'\s*\([^;{}]*\)\s*\{', source)
    assert match, name
    brace = source.index('{', match.start())
    end, depth = brace + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


def main():
    header = r'''
#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/resource.h>
#define NONE -1
#define TRUE 1
#define HALO_XBOX_MEMORY_BASE 0x80000000u
#define HALO_XBOX_TAG_BASE 0x803a6000u
#define HALO_VITA_TAG_CAPACITY 0x01600000u
#define HALO_VITA_ARENA_SIZE (96u*1024u*1024u)
#define MAXIMUM_STRUCTURE_BSPS_PER_SCENARIO 16
#define match_assert(f,l,c) assert(c)
#define match_vassert(f,l,c,m) assert(c)
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
typedef unsigned char byte;
typedef uint16_t word;
typedef uint32_t pixel32;
struct tag_data { long size; void *address; };
struct sound_permutation { struct tag_data mouth_data; };
struct block_definition { long element_size; const char *name; };
struct tag_block { long count; void *address; struct block_definition *definition; };
struct string_list { struct tag_block strings; };
struct string_list_entry { struct tag_data string; };
struct scenario { struct tag_block structure_bsp_references; };
struct scenario_structure_bsp_reference { void *structure_bsp,*base_address; long file_size; };
struct structure_bsp { byte bytes[64]; };
static struct scenario *global_scenario;
static byte *image;
static size_t active_size;
static jmp_buf rejected;
static void vita_log(const char *format,...) { (void)format; }
static void vita_fatal(const char *message) { (void)message; longjmp(rejected,1); }
static size_t halo_vita_cache_direct_tag_size(void) { return active_size; }
static uintptr_t halo_vita_memory_base(void) { return (uintptr_t)image-0x3a6000u; }
static uintptr_t halo_vita_memory_address(uintptr_t xbox) { return (uintptr_t)image+xbox-HALO_XBOX_TAG_BASE; }
#define string_list_definition_get(i) ((struct string_list *)(image+0x1000))
#define unicode_string_list_definition_get(i) string_list_definition_get(i)
#define TAG_BLOCK_GET_ELEMENT(b,i,t) ((t *)tag_block_get_element_with_size(b,i,sizeof(t)))
struct font_header { struct tag_data pixels; };
struct font_character { short bitmap_width,bitmap_height,hardware_character_index,pad; long pixels_offset; };
struct parse_string_state { int unused; };
struct bitmap { short format,width,height; void *base_address; };
static struct { struct bitmap *bitmap; short encoding_shift; } draw_character_software_globals;
enum { _bitmap_format_a8=0,_bitmap_format_y8=1,_bitmap_format_ay8=2,
 _bitmap_format_r5g6b5=6,_bitmap_format_a8r8g8b8=11 };
static int bitmap_format_get_bits_per_pixel(short format) { return format==11?32:format==6?16:8; }
static void display_assert(const char *s,const char *f,int l,int c) { abort(); }
static void system_exit(int c) { exit(c); }
#define MAXIMUM_HARDWARE_CHARACTERS 8
#define HARDWARE_CHARACTER_CACHE_BITMAP_WIDTH 8
#define HARDWARE_CHARACTER_CACHE_BITMAP_HEIGHT 8
struct hardware_character { struct font_character *character; short x0,y0; };
static struct { int initialized; short x0,y0,maximum_character_height,read_index,write_index;
 struct bitmap *bitmap; struct hardware_character characters[8]; } hardware_character_cache;
static short magic_number=17;
static int changes;
static void flush_hardware_character(struct hardware_character *h) { h->character->hardware_character_index=NONE; }
static void *bitmap_2d_address(struct bitmap *b,short x,short y,short mip) { return (word *)b->base_address+y*b->width+x; }
static void rasterizer_bitmap_changed(struct bitmap *b) { ++changes; }
'''
    resolver = (ROOT / 'port/vita/src/vita_tag_pointer.c').read_text()
    resolver = re.sub(r'^#include .*\n', '', resolver, flags=re.M)
    accessors = (ROOT / 'source/tag_files/tag_groups.c').read_text()
    functions = ''.join(definition(accessors, n) for n in
                        ('tag_data_get_pointer', 'tag_block_get_element_with_size'))
    strings = (ROOT / 'source/text/text_group.c').read_text()
    text = ''.join(definition(strings, n) for n in
                   ('string_list_get_string', 'unicode_string_list_get_string'))
    software = definition((ROOT / 'source/text/draw_string.c').read_text(), 'bitmap_draw_character')
    hardware = definition((ROOT / 'source/rasterizer/rasterizer_text.c').read_text(),
                          'cache_hardware_format_character')
    mouth = definition((ROOT / "source/sound/sound_definitions.c").read_text(),
                       "sound_permutation_get_mouth_aperture")
    legacy = text + software + hardware + mouth
    for n in ('unicode_string_list_get_string', 'string_list_get_string',
              'bitmap_draw_character', 'cache_hardware_format_character',
              'sound_permutation_get_mouth_aperture'):
        legacy = re.sub(r'\b' + n + r'\b', 'legacy_' + n, legacy)
    legacy = '\n#undef HALO_VITA\n' + legacy + '\n#define HALO_VITA 1\n'
    test = r'''
static void expect_legacy_fault(int kind,struct font_header *font,struct font_character *glyph) {
 pid_t pid=fork();assert(pid>=0);
 if(!pid) {
  struct rlimit limit={0,0};setrlimit(RLIMIT_CORE,&limit);
  if(kind==0)legacy_string_list_get_string(1,0);
  if(kind==1)legacy_unicode_string_list_get_string(1,0);
  if(kind==2)legacy_bitmap_draw_character(NULL,font,glyph,0xff000000,0,0,0,0,2,2);
  if(kind==3)legacy_cache_hardware_format_character(font,glyph);
  if(kind==4)legacy_sound_permutation_get_mouth_aperture((void *)(image+0x3200),1)[0]=0;
  _exit(0);
 }
 int status;assert(waitpid(pid,&status,0)==pid);
 assert(WIFSIGNALED(status) && WTERMSIG(status)==SIGSEGV);
}
int main(void) {
 assert(sizeof(wchar_t)==2);
 /* Protect the exact serialized page from the00.40 fault. */
 void *guard=mmap((void *)0x80489000u,4096,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
 assert(guard==(void *)0x80489000u);
 active_size=0x191f04;image=calloc(1,active_size);assert(image);
 struct string_list *list=(void *)(image+0x1000);
 struct string_list_entry *entry=(void *)(image+0x2000);
 void *raw=(void *)0x80489668u;byte *native=image+0xe3668;
 list->strings=(struct tag_block){1,(void *)(HALO_XBOX_TAG_BASE+0x2000u),NULL};
 entry->string=(struct tag_data){2,raw};native[0]='0';native[1]='x';
 struct font_header *font=(void *)(image+0x3000);
 struct font_character *glyph=(void *)(image+0x3100);
 *glyph=(struct font_character){2,2,NONE,0,1};font->pixels=(struct tag_data){5,raw};
 byte dest[4]={255,255,255,255};struct bitmap software_bitmap={0,2,2,dest};
 draw_character_software_globals.bitmap=&software_bitmap;
 word atlas[64]={0};struct bitmap hardware_bitmap={6,8,8,atlas};
 hardware_character_cache.initialized=1;hardware_character_cache.bitmap=&hardware_bitmap;
 struct sound_permutation *permutation=(void *)(image+0x3200);
 permutation->mouth_data=(struct tag_data){5,raw};
 for(int kind=0;kind<5;kind++)expect_legacy_fault(kind,font,glyph);
 for(int tick=0;tick<5;tick++)assert(sound_permutation_get_mouth_aperture(permutation,tick)==native+tick);
 assert(permutation->mouth_data.address==raw);
 assert(string_list_get_string(1,0)==(char *)native && native[0]=='0' && native[1]==0);
 assert(entry->string.address==raw && list->strings.address==(void *)(HALO_XBOX_TAG_BASE+0x2000u));
 assert(!strcmp(string_list_get_string(NONE,0),"<missing string>"));
 assert(!strcmp(string_list_get_string(1,-1),"<missing string>"));
 assert(!strcmp(string_list_get_string(1,1),"<missing string>"));
 entry->string.size=0;assert(!strcmp(string_list_get_string(1,0),"<missing string>"));
 entry->string.size=6;word utf[3]={0x41,0x03a9,0x1234};memcpy(native,utf,sizeof(utf));
 wchar_t *wide=unicode_string_list_get_string(1,0);
 assert((void *)wide==native && wide[0]==0x41 && wide[1]==0x03a9 && wide[2]==0);
 assert(unicode_string_list_get_string(NONE,0)[0]=='<');
 /* The original shared resolver rejects a compiled span before a terminator write. */
 entry->string=(struct tag_data){2,(void *)(HALO_XBOX_TAG_BASE+active_size-1)};
 image[active_size-1]=0x7b;
 if(!setjmp(rejected)){string_list_get_string(1,0);assert(!"invalid span accepted");}
 assert(image[active_size-1]==0x7b);
 permutation->mouth_data=(struct tag_data){2,(void *)(HALO_XBOX_TAG_BASE+active_size-1)};
 if(!setjmp(rejected)){sound_permutation_get_mouth_aperture(permutation,1);assert(!"invalid mouth span accepted");}
 assert(image[active_size-1]==0x7b);
 byte coverage[5]={0x5a,0,16,128,255};memcpy(native,coverage,5);
 bitmap_draw_character(NULL,font,glyph,0xff000000,0,0,0,0,2,2);
 assert(dest[0]==255 && dest[1]==15 && dest[2]==127 && dest[3]==254);
 cache_hardware_format_character(font,glyph);
 assert(atlas[0]==0x0fff && atlas[1]==0x1fff && atlas[8]==0x8fff && atlas[9]==0xffff);
 assert(changes==1 && glyph->hardware_character_index==0 && hardware_character_cache.write_index==1);
 cache_hardware_format_character(font,glyph);assert(changes==1);
 assert(font->pixels.address==raw && !memcmp(native,coverage,5));
 /* Retail fonts include empty placeholders with negative width, zero height. */
 struct font_character empty={-2,0,NONE,0,0};word saved_atlas[64];memcpy(saved_atlas,atlas,sizeof(atlas));
 bitmap_draw_character(NULL,font,&empty,0xff000000,0,0,0,0,-2,0);
 cache_hardware_format_character(font,&empty);
 assert(!memcmp(saved_atlas,atlas,sizeof(atlas)) && changes==2);
 /* Recovery images and runtime heap objects already hold native pointers. */
 active_size=0;list->strings.address=entry;entry->string=(struct tag_data){2,native};
 native[0]='7';native[1]='x';assert(string_list_get_string(1,0)==(char *)native && native[1]==0);
 native[1]='x';assert(legacy_string_list_get_string(1,0)==(char *)native && native[1]==0);
 struct tag_data heap={2,native};active_size=0x191f04;
 assert(tag_data_get_pointer(&heap,0,2)==native);
 munmap(guard,4096);free(image);return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='halo-text-pointers-') as tmp:
        path = Path(tmp)
        (path / 'test.c').write_text(header + resolver + functions + text + software + hardware + mouth + legacy + test)
        subprocess.run(['gcc', '-std=c11', '-O2', '-fshort-wchar', '-DHALO_VITA',
                        '-Werror=implicit-function-declaration', str(path / 'test.c'),
                        '-o', str(path / 'test')], check=True)
        subprocess.run([str(path / 'test')], check=True, timeout=10)
    print('PASS: five legacy serialized text/font/mouth-data consumers fault; actual ASCII/UTF16 getters and software/hardware glyph consumers translate through original accessors and real Vita resolver; bytes, terminators, cache hit, bounds, recovery/runtime ownership preserved')


if __name__ == '__main__':
    main()
