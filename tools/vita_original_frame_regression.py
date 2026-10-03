#!/usr/bin/env python3
"""Run actual native menu-frame owners; require original clock/input ordering."""
from pathlib import Path
import subprocess
import tempfile
from vita_original_cache_owner_regression import definition
ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'port/vita/src/halo_ui_original.c').read_text()
actual = (definition(source, 'halo_vita_ui_process_shell_frame') +
          definition(source.replace('int halo_vita_original_render_menu_frame', 'long halo_vita_original_render_menu_frame'), 'halo_vita_original_render_menu_frame'))
code = r'''
#include <assert.h>
#include <stddef.h>
#define TRUE 1
#define FALSE 0
static int operations[16],count,ready,remove_root;
static int halo_vita_ui_original_root_ready(void) {return ready;}
static void step(int n) {operations[count++]=n;}
static void input_frame_begin(void) {step(1);}
static void input_update(void) {step(2);}
static void input_abstraction_update(void) {step(3);}
static void event_manager_update(void) {step(4);}
static void halo_vita_main_render_time_update(void) {step(5);}
static void process_ui_widgets(void) {step(6);if(remove_root)ready=0;}
static void main_pregame_render(void) {assert(ready);step(7);}
static void render_frame_present(void *a,void *b) {assert(!a && !b);step(8);}
static void input_frame_end(void) {step(9);}
'''+actual+r'''
int main(void) {
 ready=1;assert(halo_vita_original_render_menu_frame());assert(count==9);
 for(int i=0;i<9;i++)assert(operations[i]==i+1);
 count=0;ready=0;assert(!halo_vita_original_render_menu_frame());assert(count==0);
 count=0;ready=1;remove_root=1;assert(!halo_vita_original_render_menu_frame());assert(count==7);
 for(int i=0;i<6;i++)assert(operations[i]==i+1);
 assert(operations[6]==9);
 return 0;
}
'''
with tempfile.TemporaryDirectory() as directory:
    path=Path(directory)
    (path/'test.c').write_text(code)
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-DHALO_VITA_ORIGINAL_RUNTIME',
                    str(path/'test.c'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
print('PASS actual original menu frame: input/events -> clock -> widgets -> pregame draw/present -> input end; lost-root frame closes input without drawing')
