#!/usr/bin/env python3
"""Actual staged sound lifecycle, arm/refresh phases and deferred cache service."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
s=(root/'port/vita/src/halo_menu_audio.c').read_text()
body=s[s.index('/* Public original implementation'):]
s=(root/'source/sound/sound_manager.c').read_text()
refresh=s[s.index('boolean halo_vita_sound_menu_refresh('):s.rindex('#endif')]
s=(root/'port/vita/src/halo_menu_audio_guard.c').read_text()
guard=s[s.index('long __wrap_sound_render_time('):s.index('void __wrap_compute_sound_obstruction(')]
renderer=(root/'port/vita/src/halo_renderer_runtime.c').read_text()
assert renderer.index('rasterizer_present(NULL, NULL);') < renderer.index('halo_vita_menu_audio_frame();')
code=r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
typedef int boolean;typedef float real;
#define NONE (-1L)
#define TRUE 1
#define FALSE 0
#define SOUND_DEFINITION_TAG 1
#define MAXIMUM_SOUND_DATA_SIZE 0x400000
#define _sound_spatialization_mode_none 0
enum {_looping_sound_refresh_start,_looping_sound_refresh_loop,_looping_sound_refresh_stop};
struct tag_iterator {int next;};
struct tag_block {long count;void *address;};
struct tag_data {long size,file_offset;};
struct sound_permutation {long unknown0;unsigned long unknown1;struct tag_data samples;};
struct sound_pitch_range {short actual_permutation_count;struct tag_block permutations;};
struct sound_definition {struct tag_block pitch_ranges;};
struct sound_source {short spatialization_mode,pad;real scale,gain,location[11],obstruction,occlusion;};
#define csmemset memset
#define TAG_BLOCK_GET_ELEMENT(b,n,t) (((t *)(b)->address)+(n))
static struct sound_permutation perm={NONE,0,{36,4096}};
static struct sound_pitch_range range={1,{1,&perm}};
static struct sound_definition definition={{1,&range}};
static int running,active,registered,classes,initialized,opened,rendered,closed,destroyed;
static int refresh_calls,feedback_calls,phase[32],feedbacks[8],idled,yields;
static long ids[32];static jmp_buf failure;
static void vita_log(const char *s,...){(void)s;}
static _Noreturn void vita_fatal(const char *s){(void)s;longjmp(failure,1);}
static int game_in_progress(void){return running;}
static void tag_iterator_new(struct tag_iterator *i,int group){assert(group==1);i->next=0;}
static long tag_iterator_next(struct tag_iterator *i){return i->next++?NONE:12;}
static struct sound_definition *sound_definition_get(long i){assert(i==12);return &definition;}
static const char *tag_get_name(long i){assert(i==12 || i==13);return "synthetic_loop";}
static void sound_cache_sound_new(long i,struct sound_permutation *p){assert(i==12 && !initialized && p==&perm);registered++;}
static void sound_classes_initialize(void){assert(registered==1);classes=1;}
static void sound_classes_initialize_for_new_map(void){assert(classes);}
static void sound_initialize(void){assert(classes && !initialized);initialized=active=1;}
static void sound_cache_open(void){assert(active);opened=1;}
static void sound_initialize_for_new_map(void){assert(opened);}
static void sound_render(void){assert(active && opened);rendered++;}
static void sound_stop_all(void){assert(active);}
static void sound_dispose_from_old_map(void){assert(active);}
static void sound_cache_close(void){assert(active);closed=1;}
static void sound_dispose(void){assert(closed);active=0;}
static void sound_classes_dispose(void){assert(!active);classes=0;}
static void halo_vita_audio_mixer_shutdown(void){assert(!active && closed && !classes);destroyed=1;}
static void ui_play_audio_feedback_sound(short f){assert(active);feedbacks[feedback_calls++]=f;}
static unsigned long system_milliseconds(void){return 1234;}
static long __real_sound_render_time(void){return 5678;}
static void __real_sound_idle(void){assert(active && opened);idled++;}
static int SwitchToThread(void){yields++;return 1;}
static boolean sound_refresh_looping(long definition,long identifier,struct sound_source *source,short state,boolean alternate,real fade){
assert(definition==identifier && !source->spatialization_mode && source->scale==1 && source->gain==1 && !source->obstruction && !source->occlusion && !alternate && fade==0);
ids[refresh_calls]=definition;phase[refresh_calls++]=state;return 0;}
void halo_vita_menu_audio_stop(void);
boolean sound_is_active(void){return active;}
'''+refresh+body+guard+r'''
int main(void){
assert(!halo_vita_menu_audio_start(12));
__wrap_sound_idle();assert(yields==1 && !idled && __wrap_sound_render_time()==1234);
perm.unknown1=0x82000000;
if(!setjmp(failure)){halo_vita_menu_audio_initialize();assert(!"stale cache accepted");}
assert(!registered && !initialized);perm.unknown1=0;
running=1;if(!setjmp(failure)){halo_vita_menu_audio_initialize();assert(!"running game accepted");}running=0;
assert(halo_vita_menu_audio_initialize() && halo_vita_menu_audio_initialize());
assert(registered==1 && initialized && opened && halo_vita_menu_audio_ready());
__wrap_sound_idle();assert(idled==1 && __wrap_sound_render_time()==5678);
assert(halo_vita_menu_audio_start(12));assert(!refresh_calls);
halo_vita_menu_audio_frame();assert(refresh_calls==1 && phase[0]==_looping_sound_refresh_start && rendered==1);
halo_vita_menu_audio_start(12);halo_vita_menu_audio_frame();assert(phase[1]==_looping_sound_refresh_loop);
halo_vita_menu_audio_start(13);assert(phase[2]==_looping_sound_refresh_stop && ids[2]==12);
halo_vita_menu_audio_frame();assert(phase[3]==_looping_sound_refresh_start && ids[3]==13);
for(int i=0;i<4;i++)halo_vita_menu_audio_feedback_probe();
assert(feedback_calls==4 && feedbacks[0]==1 && feedbacks[1]==2 && feedbacks[2]==3 && feedbacks[3]==1);
halo_vita_menu_audio_dispose();assert(destroyed && !halo_vita_menu_audio_ready());
__wrap_sound_idle();assert(idled==1 && yields==2 && __wrap_sound_render_time()==1234);
int calls=refresh_calls;halo_vita_menu_audio_frame();halo_vita_menu_audio_dispose();assert(calls==refresh_calls);
puts("PASS actual staged audio: cold-cache rejection, original-owner order, no playback until rendered frame, start/loop/stop/source phases, feedback, pre/post-lifecycle cache-service gating and original disposal before SDL close");}
'''
out=root/'build/vita/tests/menu-audio';out.mkdir(parents=True,exist_ok=True)
p=out/'menu.c';p.write_text(code);exe=out/'menu'
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(p),'-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
