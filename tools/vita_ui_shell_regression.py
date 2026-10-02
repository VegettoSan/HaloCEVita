#!/usr/bin/env python3
"""Execute the actual Vita keyboard/frame boundary under ASan/UBSan.

Engine owners are modeled at their call boundary; this does not certify Vita
linkage or actual keyboard rendering/storage.
"""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'port/vita/src/halo_ui_widget_vita.c').read_text()
actual = source[source.index('int halo_vita_ui_process_menu_action('):source.rindex('#endif')]
code = r'''
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
typedef int boolean;
typedef unsigned char byte;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define MAXIMUM_NUMBER_OF_LOCAL_PLAYERS 4
enum {_event_type_button=3};
enum {_gamepad_analog_button_a, _widget_event_b_button,
 _gamepad_analog_button_x, _gamepad_analog_button_y,
 _gamepad_analog_button_black, _gamepad_analog_button_white,
 _gamepad_analog_button_left_trigger, _gamepad_analog_button_right_trigger,
 _widget_event_dpad_up, _widget_event_dpad_down,
 _widget_event_dpad_left, _widget_event_dpad_right,
 _gamepad_binary_button_start};
struct event_record {short type,controller_index;struct {struct {byte index,value;} button;} data;};
struct ui_widget_definition {int dummy;};
struct widget_instance {short local_player_index;long definition_tag_index;};
struct widget_stack_data {long previous_widget_tag,focused_child_parent_widget_tag;
 short local_player_index,focused_child_index;};
static struct {int initialized;unsigned long current_system_milliseconds;
 struct widget_instance *active_widgets[4];void *widget_stack[4];} widget_globals;
static struct widget_instance root={.local_player_index=0},restored;
static struct ui_widget_definition definition;
static int we_are_at_the_main_menu=1,keyboard,posts,processed,flushed,dispatches,moves,updates,pops,focus_restores;
static int posted_index,dispatched_index,delete_on_update;
static unsigned long system_milliseconds(void) {return 1234;}
static void vita_log(const char *f,...) {(void)f;}
static void halo_vita_ui_event_reset(void) {}
static struct widget_instance *halo_vita_menu_root(short i) {assert(i==0);return widget_globals.active_widgets[0];}
static int virtual_keyboard_active(void) {return keyboard;}
static void halo_vita_ui_post_button(short i) {posts++;posted_index=i;}
static void virtual_keyboard_process(void) {processed++;}
static void event_manager_flush(void) {flushed++;}
static int halo_vita_move_menu_focus(struct widget_instance *w,short i,struct event_record *e) {
 assert(w==&root&&e->type==3&&e->controller_index==0&&e->data.button.value==1);
 moves++;dispatched_index=i;return TRUE;}
static int halo_vita_dispatch_focused_button(struct widget_instance *w,short i,struct event_record *e) {
 assert(w==&root&&e->data.button.index==i);dispatches++;dispatched_index=i;return TRUE;}
static struct ui_widget_definition *ui_widget_definition_get(long i) {(void)i;return &definition;}
static void widget_instance_process_one_event_recursive(struct widget_instance *w,
 struct ui_widget_definition *d,struct event_record *e,boolean *deleted) {
 assert(w==&root&&d==&definition&&e->type==0&&e->controller_index==0);updates++;
 if(delete_on_update){widget_globals.active_widgets[0]=NULL;*deleted=TRUE;}}
static void pop_widget(void **stack,struct widget_stack_data *data) {
 assert(*stack);*stack=NULL;pops++;*data=(struct widget_stack_data){.previous_widget_tag=23,
 .focused_child_parent_widget_tag=24,.local_player_index=0,.focused_child_index=2};}
static struct widget_instance *ui_widget_load_by_name_or_tag(void *name,long tag,void *parent,
 short player,long a,long b,long c) {
 assert(!name&&tag==23&&!parent&&player==0&&a==NONE&&b==NONE&&c==NONE);
 widget_globals.active_widgets[0]=&restored;return &restored;}
static void widget_instance_set_focused_child_by_index(long tag,struct widget_instance *w,short index) {
 assert(tag==24&&w==&restored&&index==2);focus_restores++;}
''' + actual + r'''
int main(void) {
 widget_globals.initialized=1;widget_globals.active_widgets[0]=&root;
 const int actions[]={1,2,3,4,5,6,7,8,9,10,11};
 const int buttons[]={0,2,8,9,10,11,1,12,6,7,3};
 keyboard=1;
 for(int i=0;i<11;i++) {
  assert(halo_vita_ui_process_menu_action(actions[i]));assert(posted_index==buttons[i]);
 }
 assert(posts==11&&processed==0&&flushed==0&&!moves&&!dispatches);
 halo_vita_ui_process_shell_frame();assert(processed==1&&flushed==1&&!updates);
 keyboard=0;
 for(int i=0;i<11;i++) {
  assert(halo_vita_ui_process_menu_action(actions[i]));assert(dispatched_index==buttons[i]);
 }
 assert(moves==4&&dispatches==7&&posts==11);
 halo_vita_ui_process_shell_frame();assert(updates==1&&widget_globals.current_system_milliseconds==1234);
 delete_on_update=1;widget_globals.widget_stack[0]=&root;
 halo_vita_ui_process_shell_frame();assert(updates==2&&pops==1&&focus_restores==1);
 assert(widget_globals.active_widgets[0]==&restored&&!widget_globals.widget_stack[0]);
 we_are_at_the_main_menu=0;halo_vita_ui_process_shell_frame();assert(updates==2);
 assert(!halo_vita_ui_process_menu_action(1));
 puts("PASS actual Vita shell: keyboard input queues before one frame update, keyboard exclusivity, original null update and history/focus restoration");
}
'''
with tempfile.TemporaryDirectory(prefix='halo-vita-shell-') as temporary:
    path = Path(temporary)
    (path / 'test.c').write_text(code)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-g', str(path / 'test.c'),
                    '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
