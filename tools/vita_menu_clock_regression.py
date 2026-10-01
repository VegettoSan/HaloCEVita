#!/usr/bin/env python3
"""Run the original native Halo frame clock and Vita UI timestamp bridge."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
s = (root/'source/main/main.c').read_text()
a = s.index('static void main_update_time_unthrottled(')
b = s.index('\n#endif\nstatic void main_update_time(', a) if '\n#endif\nstatic void main_update_time(' in s[a:] else s.index('\nstatic void main_update_time(', a)
body = s[a:b].replace('\n#endif','').replace('#ifdef HALO_VITA_MENU_BRINGUP','')
u = (root/'source/interface/ui_widget.c').read_text()
a = u.index('void halo_vita_ui_render_clock_update(')
b = u.index('\n/* Read-only checkpoint',a)
code = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define PIN(n,lo,hi) ((n)<(lo)?(lo):(n)>(hi)?(hi):(n))
#define CEILING(n,hi) ((n)>(hi)?(hi):(n))
#define _game_connection_local 0
#define TRUE 1
#define FALSE 0
typedef float real;
typedef struct {int64_t QuadPart;} LARGE_INTEGER;
static int64_t now;
static int debug_force_frame_rate_update;
static struct {void *movie;real seconds_elapsed,recording_dt;
    int connection;unsigned long frame_start_milliseconds;
    uint64_t rasterizer_initial_index;} main_globals;
static struct {uint64_t frame_and_vertical_blank_index;} rasterizer_globals;
static struct {int initialized;unsigned long current_system_milliseconds;} widget_globals;
static real profiled;
static void QueryPerformanceCounter(LARGE_INTEGER *p){p->QuadPart=now;}
static void QueryPerformanceFrequency(LARGE_INTEGER *p){p->QuadPart=1000000;}
static unsigned long system_milliseconds(void){return (unsigned long)(now/1000);}
static void profile_seconds_elapsed(real n){profiled=n;}
#define match_assert(path,line,condition) assert(condition)
'''+body+u[a:b]+r'''
int main(void){
    widget_globals.initialized=1;main_globals.connection=1;
    now=1000000;rasterizer_globals.frame_and_vertical_blank_index=7;
    halo_vita_main_render_time_update();halo_vita_ui_render_clock_update();
    assert(main_globals.seconds_elapsed==0.f && profiled==0.f && main_globals.rasterizer_initial_index==7);
    assert(widget_globals.current_system_milliseconds==1000);
    now+=33333;halo_vita_main_render_time_update();halo_vita_ui_render_clock_update();
    assert(main_globals.seconds_elapsed>0.03332f && main_globals.seconds_elapsed<0.03334f);
    assert(profiled==main_globals.seconds_elapsed && widget_globals.current_system_milliseconds==1033);
    now+=4000000;halo_vita_main_render_time_update();assert(main_globals.seconds_elapsed==1.f);
    main_globals.connection=0;now+=1000000;halo_vita_main_render_time_update();
    assert(main_globals.seconds_elapsed>0.06666f && main_globals.seconds_elapsed<0.06667f);
    debug_force_frame_rate_update=1;now+=1000000;halo_vita_main_render_time_update();
    assert(main_globals.seconds_elapsed>0.03333f && main_globals.seconds_elapsed<0.03334f);
    main_globals.movie=&now;main_globals.recording_dt=0.125f;now+=33333;
    halo_vita_main_render_time_update();assert(main_globals.seconds_elapsed==0.125f);
    puts("PASS original native frame clock:counter delta, profile/index, long-frame/local-rate/movie policies and actual UI timestamp");
}
'''
out=root/'build/vita/tests/menu-clock';out.mkdir(parents=True,exist_ok=True)
p=out/'clock.c';p.write_text(code);exe=out/'clock'
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(p),'-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
