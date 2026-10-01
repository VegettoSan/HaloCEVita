#!/usr/bin/env python3
"""Run original screen/coordinate/present arithmetic for half-scale Vita.

Checks source logic and mocked presentation calls, not display pixels/FPS.
"""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / 'build/vita/tests/screen'
out.mkdir(parents=True, exist_ok=True)
s = (root / 'port/linux/src/d3d8_gl.c').read_text()
header = (root / 'port/vita/include/halo_vita_graphics.h').read_text()
screen = s[s.index('#define SCREEN_HEIGHT'):s.index('/* ---------- state the XDK')]
pixel = s[s.index('static GLint target_pixel('):s.index('/* binds the framebuffer')]
present = s[s.index('\t\tplatform_video_drawable_size(&window_width'):s.index('\t\txgpu_gl_state_invalidate();',s.index('void WINAPI D3DDevice_Present'))]
graphics = (root / 'port/vita/src/vita_graphics.c').read_text()
platform = graphics[graphics.index('int platform_screen_mode('):graphics.index('void platform_video_swap(')]
prefix = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int GLint, GLsizei;
typedef unsigned GLuint;
#define GL_DRAW_FRAMEBUFFER 1
#define GL_READ_FRAMEBUFFER 2
#define GL_SCISSOR_TEST 3
#define GL_TRUE 1
#define GL_COLOR_BUFFER_BIT 4
#define GL_LINEAR 5
static float target_scale[2];
static void platform_log(const char *s, ...) { (void)s; }
#ifdef HALO_ANDROID
static int config_integer(const char *s) { assert(!strcmp(s,"display.screen_width")); return 640; }
#endif
static int blit_values[8], swaps;
static void glBindFramebuffer(int target, unsigned f) { assert(target==1 || target==2); assert(f==0 || f==77); }
static unsigned framebuffer_get(unsigned c, unsigned d) { assert(c==1 && d==0); return 77; }
static void glDisable(int f) { assert(f==GL_SCISSOR_TEST); }
static void glColorMask(int r, int g, int b, int a) { assert(r&&g&&b&&a); }
static void glClearColor(float r,float g,float b,float a) { assert(!r&&!g&&!b&&a==1); }
static void glClear(unsigned m) { assert(m==GL_COLOR_BUFFER_BIT); }
static void glBlitFramebuffer(int a,int b,int c,int d,int e,int f,int g,int h,unsigned m,unsigned filter) {
    int v[]={a,b,c,d,e,f,g,h}; memcpy(blit_values,v,sizeof(v)); assert(m==GL_COLOR_BUFFER_BIT && filter==GL_LINEAR);
}
static void platform_video_swap(void) { swaps++; }
'''
wrapper = '''static void actual_present(void) {
struct { struct { unsigned long gl_width,gl_height; unsigned texture; } target; } stored;
__typeof__(stored) *back_buffer=&stored;
back_buffer->target.gl_width=(unsigned long)(halo_screen_width()*screen_scale[0]+0.5f);
back_buffer->target.gl_height=(unsigned long)(SCREEN_HEIGHT*screen_scale[1]+0.5f);
back_buffer->target.texture=1;
int window_width,window_height,width,height,x,y;
''' + present + '}\n'
suffix = r'''
int main(void) {
    long w,h; assert(!platform_screen_mode(NULL,&h));
    assert(platform_screen_mode(&w,&h) && w==960 && h==544);
    int pw,ph; platform_video_drawable_size(&pw,&ph); assert(pw==960&&ph==544);
    long selected=halo_screen_width();
#ifdef HALO_VITA
    assert(selected==640 && screen_scale[0]==0.5f && screen_scale[1]==0.5f);
    halo_screen_ui_offset(1); assert(UI_OFFSET==0);
    target_scale[0]=screen_scale[0]; target_scale[1]=screen_scale[1];
    assert(target_pixel(640,0)==320 && target_pixel(480,1)==240);
    assert(target_pixel(1,0)==1 && target_pixel(639,0)==320);
    assert(target_pixel(0,0)==0 && target_pixel(40,0)==20 && target_pixel(100,0)-target_pixel(40,0)==30);
    actual_present();
    int want[]={0,0,320,240,117,544,842,0}; assert(!memcmp(blit_values,want,sizeof(want)) && swaps==1);
    assert(halo_screen_width()==640);
#elif defined(HALO_ANDROID)
    assert(selected==640 && screen_scale[0]==1 && screen_scale[1]==1);
    halo_screen_ui_offset(1); assert(UI_OFFSET==0);
    target_scale[0]=target_scale[1]=1; assert(target_pixel(640,0)==640);
    actual_present(); assert(blit_values[2]==640&&blit_values[3]==480&&swaps==1);
#else
    assert(selected==846 && fabsf(screen_scale[0]-960.0f/846.0f)<1e-6f);
    assert(fabsf(screen_scale[1]-544.0f/480.0f)<1e-6f);
    halo_screen_ui_offset(1); assert(UI_OFFSET==103);
    target_scale[0]=screen_scale[0]; target_scale[1]=screen_scale[1];
    assert(target_pixel(846,0)==960 && target_pixel(480,1)==544);
    actual_present(); assert(blit_values[2]==960&&blit_values[3]==544&&swaps==1);
#endif
    halo_screen_ui_offset(0); assert(UI_OFFSET==0);
    puts("PASS actual screen/coordinate/present: policy, scale, UI centering, rounding and original letterbox/blit");
}
'''
for mode in ['HALO_VITA','HALO_ANDROID','DESKTOP']:
    code=prefix+header+platform+screen+pixel+wrapper+suffix
    p=out/(mode+'.c');p.write_text(code)
    subprocess.run(['cc','-std=gnu11','-Wall','-Wextra','-Werror','-D'+mode,str(p),'-lm','-o',str(out/mode)],check=True)
    subprocess.run([str(out/mode)],check=True)
