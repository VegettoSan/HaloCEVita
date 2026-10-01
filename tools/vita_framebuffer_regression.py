#!/usr/bin/env python3
"""Run actual D3D8 target/FBO bodies against distinct mocked GL namespaces.

Checks API selection, original identity/scaling and failure gates, not GXM
allocation, framebuffer pixels or cross-scene depth/stencil load/store.
"""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / 'build/vita/tests/framebuffer'
out.mkdir(parents=True, exist_ok=True)
s = (root / 'port/linux/src/d3d8_gl.c').read_text()
structs = s[s.index('struct render_target_entry\n'):s.index('/* ---------- the device */')]
targets = s[s.index('static struct render_target_entry *render_target_get('):s.index('/* the pixels per unit')]
bind = s[s.index('static BOOL bind_targets('):s.index('/* ---------- device creation */')]
screen = s[s.index('#define SCREEN_HEIGHT'):s.index('/* ---------- state the XDK')]
header = (root / 'port/vita/include/halo_vita_graphics.h').read_text()
prefix = r'''
#include <assert.h>
#include <stdarg.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#define HALO_VITA 1
#define TRUE 1
#define FALSE 0
#define SCREEN_HEIGHT 480
#define GL_NO_ERROR 0
#define GL_TEXTURE_2D 1
#define GL_RGBA8 2
#define GL_BGRA 3
#define GL_UNSIGNED_BYTE 4
#define GL_RENDERBUFFER 5
#define GL_DEPTH24_STENCIL8 6
#define GL_FRAMEBUFFER 7
#define GL_COLOR_ATTACHMENT0 8
#define GL_DEPTH_STENCIL_ATTACHMENT 9
#define GL_FRAMEBUFFER_COMPLETE 10
typedef int BOOL, GLsizei, GLint;
typedef unsigned GLuint, GLenum;
typedef struct { unsigned long Data, width, height; int depth; } D3DSurface;
struct xgpu_render_target { unsigned long data, width, height, gl_width, gl_height;
    int depth; float scale[2]; GLuint texture; };
static float target_scale[2];
static struct { D3DSurface *render_target, *depth_stencil; unsigned long frame; } device;
static GLuint nt = 1, nr = 100, nf = 200, bound_rb, bound_fb, selected_fb;
static int textures, renderbuffers, fbo_count, texture_attaches, depth_attaches, deleted_rb, deleted_fb;
static int rb_width,rb_height,tex_width,tex_height;
static GLenum error, status = GL_FRAMEBUFFER_COMPLETE;
static int fail_storage, fail_attach, zero_rb, zero_fb, invalidations;
static jmp_buf fatal;
static void vita_log(const char *f, ...) { (void)f; }
_Noreturn static void vita_fatal(const char *f, ...) { (void)f; longjmp(fatal, 1); }
static void platform_log(const char *f, ...) { (void)f; }
static void surface_dimensions(const D3DSurface *s, unsigned long *w, unsigned long *h, BOOL *d)
{ *w=s->width; *h=s->height; *d=s->depth; }
static void xgpu_gl_state_invalidate(void) { invalidations++; }
static void state_framebuffer(GLuint f) { selected_fb = f; }
static void glGenTextures(GLsizei n, GLuint *v) { assert(n==1); *v=nt++; textures++; }
static void glBindTexture(GLenum t, GLuint v) { assert(t==GL_TEXTURE_2D && v<100); }
static void glTexImage2D(GLenum t, int l, int i, GLsizei w, GLsizei h, int b, GLenum f, GLenum y, const void *p)
{ assert(t==GL_TEXTURE_2D && !l && i==GL_RGBA8 && w>0 && h>0 && !b && f==GL_BGRA && y==GL_UNSIGNED_BYTE && !p); tex_width=w;tex_height=h; }
static void glDeleteTextures(GLsizei n, const GLuint *v) { assert(n==1 && *v<100); }
static void glGenRenderbuffers(GLsizei n, GLuint *v) { assert(n==1); *v=zero_rb?0:nr++; renderbuffers++; }
static void glBindRenderbuffer(GLenum t, GLuint v) { assert(t==GL_RENDERBUFFER && (!v || (v>=100 && v<200))); bound_rb=v; }
static void glRenderbufferStorage(GLenum t, GLenum f, GLsizei w, GLsizei h)
{ assert(t==GL_RENDERBUFFER && f==GL_DEPTH24_STENCIL8 && bound_rb && w>0 && h>0); rb_width=w;rb_height=h; if(fail_storage)error=0x501; }
static void glDeleteRenderbuffers(GLsizei n, const GLuint *v) { assert(n==1 && *v>=100 && *v<200); deleted_rb++; }
static void glGenFramebuffers(GLsizei n, GLuint *v) { assert(n==1); *v=zero_fb?0:nf++; fbo_count++; }
static void glBindFramebuffer(GLenum t, GLuint v) { assert(t==GL_FRAMEBUFFER && v>=200); bound_fb=v; }
static void glFramebufferTexture2D(GLenum t, GLenum a, GLenum y, GLuint v, int l)
{ assert(t==GL_FRAMEBUFFER && a==GL_COLOR_ATTACHMENT0 && y==GL_TEXTURE_2D && v && v<100 && !l && bound_fb); texture_attaches++; if(fail_attach)error=0x500; }
static void glFramebufferRenderbuffer(GLenum t, GLenum a, GLenum y, GLuint v)
{ assert(t==GL_FRAMEBUFFER && a==GL_DEPTH_STENCIL_ATTACHMENT && y==GL_RENDERBUFFER && v>=100 && v<200 && bound_fb); depth_attaches++; if(fail_attach)error=0x500; }
static GLenum glCheckFramebufferStatus(GLenum t) { assert(t==GL_FRAMEBUFFER); return status; }
static GLenum glGetError(void) { GLenum e=error; error=0; return e; }
static void glDeleteFramebuffers(GLsizei n, const GLuint *v) { assert(n==1 && *v>=200); deleted_fb++; }
'''
suffix = r'''
#define FAIL(call) do { if (!setjmp(fatal)) { call; assert(!"expected fatal"); } } while(0)
int main(void) {
    assert(halo_screen_width()==640); halo_screen_ui_offset(TRUE); assert(UI_OFFSET==0);
    D3DSurface c={0x1000,640,480,0}, d={0x2000,640,480,1}, c2={0x3000,640,480,0};
    BOOL has_depth=0;
    assert(!render_target_get(NULL));
    D3DSurface empty={0}; assert(!render_target_get(&empty));
    struct render_target_entry *ce=render_target_get(&c), *de=render_target_get(&d);
    assert(ce->target.gl_width==320 && ce->target.gl_height==240);
    assert(rb_width==320 && rb_height==240 && tex_width==320 && tex_height==240);
    assert(ce->target.texture==1 && !ce->depth_buffer);
    assert(de->depth_buffer==100 && !de->target.texture);
    assert(render_target_get(&c)==ce && render_target_get(&d)==de && textures==1 && renderbuffers==1);
    assert(xgpu_render_target_find(c.Data)==&ce->target && !xgpu_render_target_find(d.Data));
    device.render_target=&c; device.depth_stencil=&d;
    assert(bind_targets(&has_depth) && has_depth && ce->last_rendered==1);
    GLuint shared=selected_fb; assert(fbo_count==1 && depth_attaches==1 && texture_attaches==1);
    assert(bind_targets(&has_depth) && selected_fb==shared && depth_attaches==1 && texture_attaches==1);
    device.render_target=&c2;
    assert(bind_targets(&has_depth) && selected_fb==shared && depth_attaches==1 && texture_attaches==2);
    device.render_target=&c;
    assert(bind_targets(&has_depth) && selected_fb==shared && depth_attaches==1 && texture_attaches==3);
    GLuint read=framebuffer_get(ce->target.texture,0); assert(read!=shared && depth_attaches==1);
    assert(framebuffer_get(ce->target.texture,0)==read && fbo_count==2);
    device.depth_stencil=NULL; assert(bind_targets(&has_depth) && !has_depth && selected_fb==read);
    device.render_target=NULL; assert(!bind_targets(&has_depth));
    device.depth_stencil=&d; FAIL(bind_targets(&has_depth));
    device.render_target=&c; device.depth_stencil=&c2; assert(bind_targets(&has_depth) && !has_depth);
    D3DSurface small={0x4000,64,64,0}; device.render_target=&small; device.depth_stencil=&d;
    FAIL(bind_targets(&has_depth)); assert(render_target_get(&small)->target.gl_width==64);
    D3DSurface resized=c; resized.width=64; assert(render_target_get(&resized)!=ce);
    float old=screen_scale[0]; screen_scale[0]=1; assert(render_target_get(&c)!=ce); screen_scale[0]=old;
    assert(render_target_get(&c)==ce);
    D3DSurface bad={0x5000,32,32,1}; struct render_target_entry *head=render_targets;
    fail_storage=1; FAIL(render_target_get(&bad)); assert(render_targets==head && deleted_rb==1 && !xgpu_render_target_find(bad.Data)); fail_storage=0;
    zero_rb=1; FAIL(render_target_get(&bad)); assert(render_targets==head); zero_rb=0;
    struct framebuffer_entry *fbhead=framebuffers;
    status=0x8cd6; FAIL(framebuffer_get(90,0)); assert(framebuffers==fbhead && deleted_fb==1); status=GL_FRAMEBUFFER_COMPLETE;
    fail_attach=1; FAIL(framebuffer_get(91,0)); assert(framebuffers==fbhead && deleted_fb==2); fail_attach=0;
    zero_fb=1; FAIL(framebuffer_get(92,0)); assert(framebuffers==fbhead); zero_fb=0;
    fail_attach=1; FAIL(framebuffer_get(93,de->depth_buffer)); assert(framebuffers==fbhead); fail_attach=0;
    assert(framebuffer_get(ce->target.texture,de->depth_buffer)==shared);
    assert(depth_attaches==1 && invalidations>0);
    puts("PASS actual Vita targets/FBO: namespaces, scale/cache, shared depth identity, attachments and fatal gates");
}
'''
source = out / 'targets.c'
source.write_text(prefix + header + screen + structs + targets + bind + suffix)
subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', str(source), '-o', str(out/'targets')], check=True)
subprocess.run([str(out/'targets')], check=True)
