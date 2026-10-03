#!/usr/bin/env python3
"""Execute the original event dispatcher against explicit allocator boundaries."""
import os
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
s = (ROOT/'source/interface/ui_widget.c').read_text()
a=s.index('static void event_handler_dispatch(',s.index('static void event_handler_dispatch(')+1)
b=s.index('static boolean ui_widget_load_children_recursive(',a)
block_macro=s[s.index('#ifdef HALO_VITA\n#define UI_WIDGET_BLOCK_ELEMENT'):s.index('/* ---------- structures */',s.index('#ifdef HALO_VITA\n#define UI_WIDGET_BLOCK_ELEMENT'))]
actual=block_macro+s[a:b]
code=r'''
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define HALO_VITA_MENU_BRINGUP 1
#define MAXIMUM_NUMBER_OF_LOCAL_PLAYERS 4
#define TEST_FLAG(v,b) ((v)&(1L<<(b)))
#define match_assert(f,l,c) assert(c)
enum {_event_handler_close_current_widget_bit,_event_handler_close_other_widget_bit,
_event_handler_close_all_widgets_bit,_event_handler_open_widget_bit,_event_handler_reload_self_bit,
_event_handler_reload_widget_bit,_event_handler_give_focus_to_widget_bit,_event_handler_run_function_bit,
_event_handler_replace_with_other_widget_bit,_event_handler_go_back_to_previous_widget_bit,
_event_handler_run_scenario_script_bit,_event_handler_look_for_conditional_widget_on_failure_bit};
enum {_conditional_widget_load_if_event_handler_function_fails_bit=0};
enum {_error_silent,_ui_audio_feedback_none=-1,_ui_audio_feedback_cursor=0,_ui_audio_feedback_forward=1,_ui_audio_feedback_back=2};
struct ref {long index;};
struct block {long count;void *address;};
struct ui_widget_conditional_reference {struct ref widget_tag;long flags;};
struct ui_widget_definition {struct block conditional_widgets;};
struct ui_widget_event_handler_reference {long flags;short function;struct ref widget_tag,sound_effect;char script[32];};
struct event_record {int unused;};
struct widget_instance {long definition_tag_index;short local_player_index;
struct widget_instance *parent,*next,*previous,*child,*focused_child;short horizontal_offset,vertical_offset;};
struct widget_stack_data {int unused;};
static struct {struct widget_instance *active_widgets[4];void *widget_stack[4];} widget_globals;
static int vita_menu_dispatch_blocked, fn_result=1, fn_delete, launches, backs, frees, sounds, last_audio, reloads;
static struct widget_instance *replacement;
static void vita_log(const char *f,...) {(void)f;}
static void error(int p,const char *f,...) {(void)p;(void)f;}
static void ui_widget_delete(struct widget_instance *w) {frees++;free(w);}
static int ui_widget_event_handler_function_invoke(struct widget_instance *w,struct event_record *e,short f,boolean *d) {
(void)e;(void)f;if(fn_delete){ui_widget_delete(w);*d=1;}return fn_result;}
static struct widget_instance *widget_instance_get_topmost_parent(struct widget_instance *w) {while(w->parent)w=w->parent;return w;}
static struct widget_instance *ui_widget_launch_widget(struct widget_instance *w,long t) {(void)t;launches++;ui_widget_delete(widget_instance_get_topmost_parent(w));return (void *)1;}
static void widget_instance_give_focus_by_tag(struct widget_instance *w,long t,short p) {(void)w;(void)t;(void)p;}
static void widget_instance_reload_recursive(struct widget_instance *w) {(void)w;reloads++;}
static void ui_widget_reload_by_tag(long t) {(void)t;reloads++;}
static struct widget_instance *widget_instance_find_by_tag_index(long t) {(void)t;return NULL;}
static struct widget_instance *ui_widget_load_by_name_or_tag(void *n,long t,struct widget_instance *w,short p,long i,long f,short c) {
(void)n;(void)t;(void)p;(void)i;(void)f;(void)c;replacement=calloc(1,sizeof(*replacement));replacement->parent=w;
replacement->horizontal_offset=2;replacement->vertical_offset=3;widget_globals.active_widgets[1]=replacement;return replacement;}
static void widget_instance_go_back_to_previous(struct widget_instance *w) {backs++;ui_widget_delete(w);}
static void unspatialized_impulse_sound_new(long t,float v) {(void)t;assert(v==1);sounds++;}
static void pop_widget(void **s,struct widget_stack_data *d) {(void)d;*s=NULL;}
static void ui_play_audio_feedback_sound(long a) {last_audio=a;}
''' + actual + r'''
static struct widget_instance *new_widget(void) {return calloc(1,sizeof(struct widget_instance));}
int main(void) {
struct event_record e={0};struct ui_widget_definition d={0};boolean deleted;
struct ui_widget_event_handler_reference h={.widget_tag={7},.sound_effect={9}};
struct widget_instance *w=new_widget();h.flags=1L<<_event_handler_open_widget_bit;
event_handler_dispatch(w,&d,&e,&h,&deleted);assert(deleted&&launches==1&&sounds==1&&last_audio==1);
w=new_widget();h.flags=1L<<_event_handler_go_back_to_previous_widget_bit;
event_handler_dispatch(w,&d,&e,&h,&deleted);assert(deleted&&backs==1&&last_audio==2);
/* Real replacement relinks siblings, parent focus and offsets before deleting caller. */
struct widget_instance parent={0},prev={0},next={0};w=new_widget();w->parent=&parent;w->previous=&prev;w->next=&next;
w->horizontal_offset=11;w->vertical_offset=12;parent.child=parent.focused_child=w;prev.next=w;next.previous=w;
h.flags=1L<<_event_handler_replace_with_other_widget_bit;
event_handler_dispatch(w,&d,&e,&h,&deleted);assert(deleted&&parent.child==replacement&&parent.focused_child==replacement);
assert(prev.next==replacement&&next.previous==replacement&&replacement->parent==&parent);
assert(replacement->horizontal_offset==13&&replacement->vertical_offset==15&&!widget_globals.active_widgets[1]);free(replacement);
/* False predicates take the authored conditional branch without touching freed callers. */
struct ui_widget_conditional_reference c={.widget_tag={8},.flags=1};d.conditional_widgets=(struct block){1,&c};
fn_result=0;w=new_widget();h.flags=(1L<<_event_handler_run_function_bit)|(1L<<_event_handler_look_for_conditional_widget_on_failure_bit);
event_handler_dispatch(w,&d,&e,&h,&deleted);assert(deleted&&launches==2);
fn_result=1;fn_delete=1;w=new_widget();h.flags=1L<<_event_handler_run_function_bit;
event_handler_dispatch(w,&d,&e,&h,&deleted);assert(deleted);fn_delete=0;
w=new_widget();h.flags=1L<<_event_handler_reload_self_bit;event_handler_dispatch(w,&d,&e,&h,&deleted);assert(!deleted&&reloads==1);free(w);
/* A world script does not run or silently fall through to an open effect. */
w=new_widget();h.flags=(1L<<_event_handler_run_scenario_script_bit)|(1L<<_event_handler_open_widget_bit);h.script[0]='x';
event_handler_dispatch(w,&d,&e,&h,&deleted);assert(!deleted&&vita_menu_dispatch_blocked&&launches==2);free(w);
puts("PASS original event effects: root open/back, sibling replacement, conditional failure, deletion, reload and script gate");
return 0;
}
'''
with tempfile.TemporaryDirectory() as td:
    p=Path(td);(p/'test.c').write_text(code)
    subprocess.run(['cc','-std=gnu11','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g',str(p/'test.c'),'-o',str(p/'test')],check=True)
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
    subprocess.run([str(p/'test')],env=env,check=True)
