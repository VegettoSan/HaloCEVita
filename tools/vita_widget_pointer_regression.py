#!/usr/bin/env python3
"""Execute original widget recursion/initialization with the real Vita resolver.

Host projections test traversal, offsets, focus and handler ownership, not the
ARM ABI or real callbacks/GPU. The native verifier inspects shipping ARM calls.
"""
from pathlib import Path
import re
import subprocess
import tempfile
from vita_original_cache_owner_regression import definition
from vita_text_pointer_regression import definition as pointer_definition

ROOT = Path(__file__).resolve().parents[1]


def main():
    source = (ROOT / 'source/interface/ui_widget.c').read_text()
    start = source.index('#ifdef HALO_VITA\n#define UI_WIDGET_BLOCK_ELEMENT')
    macro = source[start:source.index('/* ---------- structures */', start)]
    keyboard = (ROOT / 'source/interface/virtual_keyboard.c').read_text()
    start = keyboard.index('#ifdef HALO_VITA\n#define virtual_keyboard_key_get')
    key_macro = keyboard[start:keyboard.index('/* ---------- structures */', start)]
    key_function = pointer_definition(keyboard, 'virtual_keyboard_get_character')
    legacy_key = key_function.replace('virtual_keyboard_get_character', 'legacy_keyboard_character')
    legacy_key = ('\n#undef HALO_VITA\n#undef virtual_keyboard_key_get\n' + key_macro
                  + legacy_key + '\n#define HALO_VITA 1\n')
    # All five typed widget-block families cross the boundary, not only root children.
    assert not re.search(r'definition->\w+\.address', source)
    assert source.count('UI_WIDGET_BLOCK_ELEMENT(') == 12  # two definitions, ten consumers
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
typedef int boolean;
typedef unsigned char byte;
typedef uint16_t word;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define HALO_XBOX_MEMORY_BASE 0x80000000u
#define HALO_XBOX_TAG_BASE 0x803a6000u
#define HALO_VITA_TAG_CAPACITY 0x01600000u
#define HALO_VITA_ARENA_SIZE (96u*1024u*1024u)
#define MAXIMUM_STRUCTURE_BSPS_PER_SCENARIO 16
#define MAXIMUM_GAMEPADS 4
#define match_assert(f,l,c) assert(c)
#define match_vassert(f,l,c,m) assert(c)
#define TEST_FLAG(v,b) ((v)&(1L<<(b)))
#define MAX(a,b) ((a)>(b)?(a):(b))
struct block_definition {long element_size;const char *name;};
struct tag_block {long count;void *address;struct block_definition *definition;};
struct scenario {struct tag_block structure_bsp_references;};
struct scenario_structure_bsp_reference {void *structure_bsp,*base_address;long file_size;};
struct structure_bsp {byte bytes[64];};
static struct scenario *global_scenario;
static byte *image;
static size_t active_size=0x191f04;
static jmp_buf rejected;
static void vita_log(const char *format,...) {(void)format;}
static void vita_fatal(const char *message) {(void)message;longjmp(rejected,1);}
static size_t halo_vita_cache_direct_tag_size(void) {return active_size;}
static uintptr_t halo_vita_memory_base(void) {return (uintptr_t)image-0x3a6000u;}
static uintptr_t halo_vita_memory_address(uintptr_t xbox) {return (uintptr_t)image+xbox-HALO_XBOX_TAG_BASE;}
#define TAG_BLOCK_GET_ELEMENT(b,i,t) ((t *)tag_block_get_element_with_size(b,i,sizeof(t)))
struct ref {long index;};
struct ui_widget_child_reference {struct ref widget_tag;long flags;short custom_controller_index,horizontal_offset,vertical_offset;};
struct ui_widget_event_handler_reference {short event_type,function;};
struct ui_widget_definition {char *name;short type;long flags,list_flags,milliseconds_to_auto_close,auto_close_fade_time;
 struct ref background_bitmap,text_label_string_list,extended_description_widget;
 struct tag_block child_widgets,event_handlers;};
struct string_list {struct tag_block strings;};
struct widget_instance {char *name;short type,local_player_index,horizontal_offset,vertical_offset;
 long definition_tag_index,creation_time,milliseconds_to_auto_close,auto_close_fade_time;
 int visible,render_regardless_of_controller_index,pause_game_time;float alpha_modifier;
 struct widget_instance *parent,*next,*previous,*child,*focused_child;
 struct {struct {long string_list_index;} text_box;struct {long number_of_items,selected_index,last_list_tab_direction;struct widget_instance *extended_description;} list;} parameters;
 struct {long number_of_sprite_frames;} animation;};
struct bitmap_group_sequence {long bitmap_count;};
struct bitmap_group {struct tag_block sequences;};
struct event_record {short controller_index;};
enum {_list_items_generated_from_string_list_tag=1,
 _child_widget_use_custom_controller_index_bit=0,
 _widget_dont_focus_a_specific_child_bit=0,
 _widget_pass_unhandled_events_to_children_bit=1,
 _widget_render_regardless_of_controller_index_bit=2,_widget_pause_game_time_bit=3,
 _ui_widget_type_text_box=1,_ui_widget_type_spinner_list=2,_ui_widget_type_column_list=3,
 _widget_event_created=0,_error_silent=0};
static struct {int dont_load_children_recursive,sound_paused,pause_game_time_count;long current_system_milliseconds;} widget_globals;
static int we_are_at_the_main_menu=1,errors,paused,loads,events,fail_tag=NONE;
static short controllers[16],functions[16];
static long tags[16];
static struct ui_widget_definition *defs[5];
static struct widget_instance instances[16];
static struct string_list strings;
static struct string_list *unicode_string_list_definition_get(long tag) {return &strings;}
static struct bitmap_group *bitmap_group_get(long tag) {assert(!"unexpected bitmap");return NULL;}
static void error(int level,const char *format,...) {++errors;}
static int game_time_get_paused(void) {return paused;}
static void game_time_set_paused(int value) {paused=value;}
static void sound_pause(int value) {(void)value;}
static int widget_instance_can_handle_events(struct widget_instance *w) {return TRUE;}
static void widget_instance_give_focus_directly(struct widget_instance *w,struct widget_instance *c) {w->focused_child=c;}
static void event_handler_dispatch(struct widget_instance *w,struct ui_widget_definition *d,
 struct event_record *e,struct ui_widget_event_handler_reference *h,boolean *deleted) {
 assert(e->controller_index==w->local_player_index);functions[events++]=h->function;*deleted=FALSE;
}
static struct widget_instance *ui_widget_load_by_name_or_tag(const char *,long,struct widget_instance *,short,long,long,short);
static void widget_instance_initialize(struct widget_instance *,struct widget_instance *,struct ui_widget_definition *,long,short,short);
#define NUMBER_OF_CONFIGURABLE_VIRTUAL_KEYS 2
struct virtual_keyboard_key {short keycode;wchar_t character,shift_character,caps_character,symbols_character,shift_caps_character,shift_symbols_character,caps_symbols_character;};
struct virtual_keyboard_definition {struct tag_block keys;};
static struct {struct virtual_keyboard_definition *keyboard;int shift_active,caps_active,symbols_active;} virtual_keyboard_globals;
'''
    resolver = (ROOT / 'port/vita/src/vita_tag_pointer.c').read_text()
    resolver = re.sub(r'^#include .*\n', '', resolver, flags=re.M)
    accessor = pointer_definition((ROOT / 'source/tag_files/tag_groups.c').read_text(),
                                  'tag_block_get_element_with_size')
    child = definition(source, 'ui_widget_load_children_recursive')
    functions = definition(source, 'ui_widget_add_child') + child + definition(source, 'widget_instance_initialize')
    legacy = child.replace('ui_widget_load_children_recursive', 'legacy_load_children')
    legacy = '\n#undef HALO_VITA\n#undef UI_WIDGET_BLOCK_ELEMENT\n' + macro + legacy + '\n#define HALO_VITA 1\n'
    test = r'''
static struct widget_instance *ui_widget_load_by_name_or_tag(const char *n,long tag,struct widget_instance *parent,
 short controller,long invoking,long focused,short index) {
 assert(tag>=100 && tag<105);if(tag==fail_tag)return NULL;
 assert(loads<16);struct widget_instance *w=&instances[loads];tags[loads]=tag;controllers[loads++]=controller;
 widget_instance_initialize(w,parent,defs[tag-100],tag,controller,0);return w;
}
static void reset_runtime(void) {memset(instances,0,sizeof(instances));loads=events=errors=0;widget_globals.dont_load_children_recursive=0;fail_tag=NONE;}
static void build_def(int i) {
 defs[i]=(void *)(image+0x1000+i*0x400);memset(defs[i],0,sizeof(*defs[i]));
 defs[i]->name="fixture";defs[i]->type=_ui_widget_type_text_box;
 defs[i]->background_bitmap.index=defs[i]->text_label_string_list.index=defs[i]->extended_description_widget.index=NONE;
 struct ui_widget_event_handler_reference *h=(void *)(image+0x4000+i*0x100);
 *h=(struct ui_widget_event_handler_reference){_widget_event_created,(short)(100+i)};
 defs[i]->event_handlers=(struct tag_block){1,(void *)(uintptr_t)(HALO_XBOX_TAG_BASE+0x4000+i*0x100),NULL};
}
int main(void) {
 image=calloc(1,active_size);assert(image);for(int i=0;i<5;i++)build_def(i);
 void *guard=mmap((void *)0x80491000u,4096,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);assert(guard==(void *)0x80491000u);
 struct ui_widget_child_reference *refs=(void *)(image+0xeb7cc);
 refs[0]=(struct ui_widget_child_reference){{101},1,2,11,-7};
 refs[1]=(struct ui_widget_child_reference){{102},1,99,-2,4};
 defs[0]->child_widgets=(struct tag_block){2,(void *)0x804917ccu,NULL};
 defs[0]->flags=1L<<_widget_pass_unhandled_events_to_children_bit;
 struct ui_widget_child_reference *nested=(void *)(image+0xed000);
 *nested=(struct ui_widget_child_reference){{103},0,0,5,6};
 defs[1]->child_widgets=(struct tag_block){1,(void *)(HALO_XBOX_TAG_BASE+0xed000),NULL};
 struct widget_instance root={.local_player_index=3};
 pid_t pid=fork();assert(pid>=0);
 if(!pid){struct rlimit limit={0,0};setrlimit(RLIMIT_CORE,&limit);legacy_load_children(&root,defs[0]);_exit(0);}
 int status;assert(waitpid(pid,&status,0)==pid);assert(WIFSIGNALED(status)&&WTERMSIG(status)==SIGSEGV);
 widget_globals.current_system_milliseconds=1234;
 widget_instance_initialize(&root,NULL,defs[0],100,3,0);
 assert(loads==3 && tags[0]==101 && tags[1]==103 && tags[2]==102);
 assert(controllers[0]==2 && controllers[1]==2 && controllers[2]==3 && errors==1);
 assert(root.child==&instances[0] && root.child->next==&instances[2] && instances[2].previous==root.child);
 assert(root.focused_child==root.child && root.child->child==&instances[1]);
 assert(instances[0].horizontal_offset==11 && instances[0].vertical_offset==-7);
 assert(instances[2].horizontal_offset==-2 && instances[2].vertical_offset==4);
 assert(instances[1].horizontal_offset==5 && instances[1].vertical_offset==6);
 assert(events==4 && functions[0]==103 && functions[1]==101 && functions[2]==102 && functions[3]==100);
 assert(root.creation_time==1234 && root.visible && root.alpha_modifier==1.0f);
 assert(defs[0]->child_widgets.address==(void *)0x804917ccu && refs[0].horizontal_offset==11);
 /* Original partial-failure and absent-child semantics. */
 reset_runtime();memset(&root,0,sizeof(root));root.local_player_index=3;fail_tag=101;
 assert(!ui_widget_load_children_recursive(&root,defs[0]) && !root.child && loads==0);
 fail_tag=NONE;refs[0].widget_tag.index=NONE;refs[1].widget_tag.index=NONE;
 assert(ui_widget_load_children_recursive(&root,defs[0]) && !root.child);
 refs[0].widget_tag.index=101;refs[1].widget_tag.index=102;
 /* Generated spinner items use original recursion suppression and list/focus. */
 reset_runtime();defs[4]->type=_ui_widget_type_spinner_list;defs[4]->list_flags=1L<<_list_items_generated_from_string_list_tag;
 defs[4]->text_label_string_list.index=200;strings.strings.count=2;
 widget_instance_initialize(&root,NULL,defs[4],104,1,0);
 assert(loads==2 && root.parameters.list.number_of_items==2 && root.focused_child==root.child);
 assert(root.parameters.list.selected_index==0 && !widget_globals.dont_load_children_recursive);
 /* Column-list descriptions detach from the normal child hierarchy. */
 reset_runtime();defs[4]->list_flags=0;defs[4]->type=_ui_widget_type_text_box;
 defs[0]->type=_ui_widget_type_column_list;defs[0]->extended_description_widget.index=104;
 widget_instance_initialize(&root,NULL,defs[0],100,3,0);
 assert(loads==4 && root.parameters.list.extended_description==&instances[3] && !instances[3].parent);
 /* Invalid compiled span stops before dereference; runtime/recovery is native. */
 defs[0]->child_widgets.address=(void *)(HALO_XBOX_TAG_BASE+active_size-1);
 if(!setjmp(rejected)){ui_widget_load_children_recursive(&root,defs[0]);assert(!"bad child span accepted");}
 active_size=0;defs[0]->type=_ui_widget_type_text_box;defs[0]->extended_description_widget.index=NONE;
 defs[0]->child_widgets.address=refs;defs[1]->child_widgets.address=nested;
 for(int i=0;i<5;i++)defs[i]->event_handlers.address=image+0x4000+i*0x100;
 reset_runtime();widget_instance_initialize(&root,NULL,defs[0],100,3,0);assert(loads==3 && events==4);
 struct tag_block heap={2,refs,NULL};active_size=0x191f04;
 assert(tag_block_get_element_with_size(&heap,1,sizeof(*refs))==&refs[1]);
 /* The same compiled-block contract applies to profile keyboard key lookup. */
 assert(sizeof(wchar_t)==2);struct virtual_keyboard_definition *kbd=(void *)(image+0x9000);
 struct virtual_keyboard_key *keys=(void *)(image+0xeb7cc);
 keys[0]=(struct virtual_keyboard_key){0,'A','B','C','D','E','F','G'};memset(&keys[1],0,sizeof(keys[1]));
 kbd->keys=(struct tag_block){2,(void *)0x804917ccu,NULL};virtual_keyboard_globals.keyboard=kbd;
 pid=fork();assert(pid>=0);
 if(!pid){struct rlimit limit={0,0};setrlimit(RLIMIT_CORE,&limit);volatile wchar_t character=legacy_keyboard_character(0);(void)character;_exit(0);}
 assert(waitpid(pid,&status,0)==pid);assert(WIFSIGNALED(status)&&WTERMSIG(status)==SIGSEGV);
 const wchar_t expected[8]={'A','B','C','E','D','F','G','E'};
 for(int flags=0;flags<8;flags++){
  virtual_keyboard_globals.shift_active=flags&1;virtual_keyboard_globals.caps_active=flags&2;virtual_keyboard_globals.symbols_active=flags&4;
  assert(virtual_keyboard_get_character(0)==expected[flags]);assert(virtual_keyboard_get_character(1)==0x7f);
 }
 assert(kbd->keys.address==(void *)0x804917ccu);
 active_size=0;kbd->keys.address=keys;assert(virtual_keyboard_get_character(0)=='E');
 munmap(guard,4096);free(image);return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='halo-widget-pointers-') as tmp:
        path = Path(tmp)
        (path / 'test.c').write_text(header + resolver + accessor + macro + functions + legacy
                                   + key_macro + key_function + legacy_key + test)
        subprocess.run(['gcc', '-std=c11', '-O2', '-fshort-wchar', '-DHALO_VITA',
                        '-Werror=implicit-function-declaration', str(path / 'test.c'),
                        '-o', str(path / 'test')], check=True)
        subprocess.run([str(path / 'test')], check=True, timeout=10)
    print('PASS: legacy widget child/keyboard pointers fault; actual original recursion/initialization/add-child/key lookup use real typed resolver; recursive order, offsets, custom/fallback controllers, created handlers, focus, failure/NONE, generated spinner, description, bounds, recovery/runtime and eight UTF16 keyboard modifier combinations preserved; all five widget-block families route through typed accessors')


if __name__ == '__main__':
    main()
