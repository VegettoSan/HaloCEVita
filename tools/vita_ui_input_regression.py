#!/usr/bin/env python3
"""Execute the staged input bridge's real focus/dispatch functions under ASan.

Original list/tab callbacks are modeled at their ownership boundary. This
checks routing and deleted-widget lifetime, not complete engine/hardware UI.
"""
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[1]
s = (ROOT / 'port/vita/src/halo_ui_widget_vita.c').read_text()
focus = s[s.index('static int halo_vita_move_menu_focus('):s.index('/* Execute only the generic')]
dispatch = s[s.index('static int halo_vita_dispatch_focused_button('):s.index('/* Action values')]
code = r'''
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define TEST_FLAG(v,b) ((v)&(1L<<(b)))
enum { _widget_event_b_button=1, _widget_event_dpad_up=8,
 _widget_event_dpad_down, _widget_event_dpad_left, _widget_event_dpad_right };
enum { _ui_widget_type_spinner_list=2,_ui_widget_type_column_list=3 };
enum { _widget_dpad_updown_tabs_thru_children_bit=3,
 _widget_dpad_leftright_tabs_thru_children_bit,
 _widget_dpad_updown_tabs_thru_list_items_bit,
 _widget_dpad_leftright_tabs_thru_list_items_bit };
enum { _ui_audio_feedback_cursor=1 };
struct event_record { int unused; };
struct ui_widget_event_handler_reference {long flags;short event_type,function;};
struct ui_widget_definition {long flags;struct {long count;void *address;} event_handlers;};
struct widget_instance {long definition_tag_index;int type;
 struct widget_instance *focused_child,*parent;
 struct {long current_frame_index;} animation;};
static struct {void *widget_stack[1];} widget_globals;
static struct ui_widget_definition defs[4];
static struct widget_instance a={.definition_tag_index=1},b={.definition_tag_index=2};
static int list_calls,tab_calls,syncs,sounds,delete_on_list,backs;
static void vita_log(const char *f,...) {(void)f;}
static struct ui_widget_definition *ui_widget_definition_get(long i) {assert(i>=0&&i<4);return &defs[i];}
static void halo_vita_sync_focus_chain_visuals(struct widget_instance *w) {assert(w);syncs++;}
static void ui_play_audio_feedback_sound(int i) {assert(i==1);sounds++;}
static int move_list(struct widget_instance *w,boolean *deleted,int previous) {
 list_calls++; if(delete_on_list){*deleted=TRUE;free(w);return TRUE;}
 w->focused_child=previous?&a:&b;return TRUE;
}
static int widget_event_function_list_widget_goto_previous_item(struct widget_instance *w,struct event_record *e,boolean *d) {(void)e;return move_list(w,d,1);}
static int widget_event_function_list_widget_goto_next_item(struct widget_instance *w,struct event_record *e,boolean *d) {(void)e;return move_list(w,d,0);}
static void widget_instance_tab_to_previous_valid_widget(struct widget_instance *w) {tab_calls++;w->focused_child=&a;}
static void widget_instance_tab_to_next_valid_widget(struct widget_instance *w) {tab_calls++;w->focused_child=&b;}
static struct widget_instance *halo_vita_deepest_focus(struct widget_instance *w) {while(w&&w->focused_child)w=w->focused_child;return w;}
static int halo_vita_ui_event_failed(void) {return FALSE;}
static int halo_vita_go_back_original(struct widget_instance *w) {(void)w;backs++;return TRUE;}
static int halo_vita_dispatch_original_menu_handler(struct widget_instance *w,
 struct ui_widget_definition *d,struct event_record *e,struct ui_widget_event_handler_reference *h) {
 (void)d;(void)e;assert(h->function==23);free(w);return TRUE;
}
''' + focus + dispatch + r'''
int main(void) {
 struct event_record event={0};
 struct widget_instance root={.definition_tag_index=0,.type=3,.focused_child=&a};
 for(int axis=0;axis<2;axis++) {
  defs[0].flags=1L<<(axis?6:5);root.focused_child=&a;
  assert(halo_vita_move_menu_focus(&root,axis?11:9,&event));assert(root.focused_child==&b);
  assert(halo_vita_move_menu_focus(&root,axis?10:8,&event));assert(root.focused_child==&a);
  int n=list_calls;assert(!halo_vita_move_menu_focus(&root,axis?9:11,&event));assert(list_calls==n);
 }
 /* Child tabbing has original precedence when both policies are authored. */
 defs[0].flags=(1L<<3)|(1L<<5);int n=list_calls;
 assert(halo_vita_move_menu_focus(&root,9,&event));assert(root.focused_child==&b&&tab_calls==1&&list_calls==n);
 defs[0].flags=1L<<4;assert(halo_vita_move_menu_focus(&root,10,&event));assert(root.focused_child==&a&&tab_calls==2);
 /* The deepest eligible focused list owns navigation. */
 defs[0].flags=1L<<5;defs[1].flags=1L<<6;a.type=2;a.focused_child=&b;root.focused_child=&a;
 assert(halo_vita_move_menu_focus(&root,10,&event));assert(a.focused_child==&a);
 a.focused_child=NULL;a.type=0;defs[1].flags=0;
 /* A deleted list must not be dereferenced for diagnostics/visual updates. */
 struct widget_instance *dead=malloc(sizeof(*dead));*dead=root;dead->focused_child=&a;
 delete_on_list=1;n=syncs;assert(!halo_vita_move_menu_focus(dead,9,&event));assert(syncs==n);
 /* A transition destroys the caller before the dispatcher logs its datum. */
 struct ui_widget_event_handler_reference h={.event_type=0,.function=23,.flags=8};
 defs[0].event_handlers.count=1;defs[0].event_handlers.address=&h;
 dead=calloc(1,sizeof(*dead));dead->definition_tag_index=0;
 assert(halo_vita_dispatch_focused_button(dead,0,&event));
 defs[0].event_handlers.count=0;widget_globals.widget_stack[0]=&h;
 assert(halo_vita_dispatch_focused_button(&root,1,&event));assert(backs==1&&sounds>0);
 puts("PASS actual Vita input routing: both axes, child precedence, deepest focus, history and deleted caller/list under ASan");
}
'''
out = ROOT / 'build/vita/tests/ui-input'
out.mkdir(parents=True, exist_ok=True)
(out / 'input.c').write_text(code)
subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-g', str(out / 'input.c'),
                '-o', str(out / 'input')], check=True)
# LSAN process enumeration is unavailable in the execution sandbox. Retain
# AddressSanitizer/UBSan; this fixture frees every dynamic widget it creates.
subprocess.run([str(out / 'input')], check=True,
               env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
