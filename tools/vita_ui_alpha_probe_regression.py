#!/usr/bin/env python3
"""Run the real bounded alpha probe against a stateful native-GL model.

Verifies restoration/bounds/resource lifetime. GPU readback/compiler acceptance
is deliberately not claimed by this host contract.
"""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
out = ROOT / 'build/vita/tests/ui-alpha'
out.mkdir(parents=True, exist_ok=True)
code = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned GLuint,GLenum;
typedef int GLint,GLsizei;
typedef float GLfloat;
typedef unsigned char GLboolean;
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_NO_ERROR 0
#define GL_TEXTURE0 100
#define HALO_VITA_RENDER_WIDTH 320
#define HALO_VITA_RENDER_HEIGHT 240
#define HALO_VITA_GAME_WIDTH 640
#define HALO_VITA_GAME_HEIGHT 480
enum { GL_READ_FRAMEBUFFER_BINDING=1,GL_DRAW_FRAMEBUFFER_BINDING,GL_ACTIVE_TEXTURE,
 GL_TEXTURE_BINDING_2D,GL_BLEND_SRC,GL_BLEND_DST,GL_BLEND_EQUATION,
 GL_COLOR_WRITEMASK,GL_CURRENT_PROGRAM,GL_COLOR_CLEAR_VALUE,GL_BLEND,
 GL_DEPTH_TEST,GL_STENCIL_TEST,GL_SCISSOR_TEST,GL_CULL_FACE,
 GL_READ_FRAMEBUFFER,GL_DRAW_FRAMEBUFFER,GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,
 GL_TEXTURE_2D,GL_FRAMEBUFFER_COMPLETE,GL_RGBA,GL_UNSIGNED_BYTE,
 GL_COLOR_BUFFER_BIT,GL_TRIANGLE_FAN };
struct xgpu_texture_description {unsigned long width,height;};
static GLint read_fb=8,draw_fb=9,active=103,bindings[4]={11,11,123,0},mask[4]={1,1,1,0};
static GLfloat clear_color[4]={.2f,.3f,.4f,.5f};
static GLboolean enabled[32];
static GLuint next_fbo=20,source_fbo,output_fbo;
static int draws,reads,finishes,created_textures,deleted_textures,created_fbos,deleted_fbos;
static int incomplete;
static void vita_log(const char *s,...) {(void)s;}
static void vita_fatal(const char *s) {(void)s;abort();}
static GLenum glGetError(void) {return GL_NO_ERROR;}
static void glGetIntegerv(GLenum k,GLint *v) {
 switch(k){
 case GL_READ_FRAMEBUFFER_BINDING:*v=read_fb;break;
 case GL_DRAW_FRAMEBUFFER_BINDING:*v=draw_fb;break;
 case GL_ACTIVE_TEXTURE:*v=active;break;
 case GL_TEXTURE_BINDING_2D:*v=bindings[active-100];break;
 case GL_BLEND_SRC:*v=0x302;break;case GL_BLEND_DST:*v=0x303;break;
 case GL_BLEND_EQUATION:*v=0x8006;break;case GL_CURRENT_PROGRAM:*v=7;break;
 case GL_COLOR_WRITEMASK:memcpy(v,mask,sizeof(mask));break;
 default:assert(0);
 }}
static void glGetFloatv(GLenum k,GLfloat *v) {assert(k==GL_COLOR_CLEAR_VALUE);memcpy(v,clear_color,sizeof(clear_color));}
static GLboolean glIsEnabled(GLenum k) {assert(k<32);return enabled[k];}
static void glActiveTexture(GLenum k) {assert(k>=100&&k<104);active=k;}
static void glGenFramebuffers(GLsizei n,GLuint *v) {assert(n==1);*v=next_fbo++;created_fbos++;}
static void glBindFramebuffer(GLenum k,GLuint v) {
 if(k==GL_READ_FRAMEBUFFER||k==GL_FRAMEBUFFER)read_fb=v;
 if(k==GL_DRAW_FRAMEBUFFER||k==GL_FRAMEBUFFER)draw_fb=v;
}
static void glFramebufferTexture2D(GLenum k,GLenum a,GLenum t,GLuint v,GLint level) {
 assert(a==GL_COLOR_ATTACHMENT0&&t==GL_TEXTURE_2D&&level==0);
 if(k==GL_READ_FRAMEBUFFER){assert(v==123);source_fbo=read_fb;}
 else{assert(k==GL_FRAMEBUFFER&&v==200);output_fbo=draw_fb;}
}
static GLenum glCheckFramebufferStatus(GLenum k) {(void)k;return incomplete?999:GL_FRAMEBUFFER_COMPLETE;}
static void glFinish(void) {finishes++;}
static void glReadPixels(GLint x,GLint y,GLsizei w,GLsizei h,GLenum f,GLenum t,void *p) {
 assert(w==1&&h==1&&f==GL_RGBA&&t==GL_UNSIGNED_BYTE);
 assert(x>=0&&x<320&&y>=0&&y<240);assert(read_fb==(int)source_fbo||read_fb==(int)output_fbo);
 memset(p,0,4);reads++;
}
static void glDeleteFramebuffers(GLsizei n,const GLuint *v) {assert(n==1&&*v>=20);deleted_fbos++;}
static void glGenTextures(GLsizei n,GLuint *v) {assert(n==1);*v=200;created_textures++;}
static void glBindTexture(GLenum k,GLuint v) {assert(k==GL_TEXTURE_2D);bindings[active-100]=v;}
static void glTexImage2D(GLenum t,GLint l,GLint f,GLsizei w,GLsizei h,GLint b,GLenum src,GLenum type,const void *p) {
 assert(t==GL_TEXTURE_2D&&l==0&&f==GL_RGBA&&w==320&&h==240&&!b&&src==GL_RGBA&&type==GL_UNSIGNED_BYTE&&!p);
}
static void glDisable(GLenum k) {enabled[k]=0;}
static void glEnable(GLenum k) {enabled[k]=1;}
static void glColorMask(GLboolean r,GLboolean g,GLboolean b,GLboolean a) {mask[0]=r;mask[1]=g;mask[2]=b;mask[3]=a;}
static void glClearColor(GLfloat r,GLfloat g,GLfloat b,GLfloat a) {clear_color[0]=r;clear_color[1]=g;clear_color[2]=b;clear_color[3]=a;}
static void glClear(GLenum k) {assert(k==GL_COLOR_BUFFER_BIT&&mask[3]==1&&!enabled[GL_SCISSOR_TEST]);}
static void glDrawArrays(GLenum k,GLint first,GLsizei n) {
 assert(k==GL_TRIANGLE_FAN&&!first&&n==4);
 assert(!enabled[GL_BLEND]&&!enabled[GL_DEPTH_TEST]&&!enabled[GL_STENCIL_TEST]&&!enabled[GL_SCISSOR_TEST]&&!enabled[GL_CULL_FACE]);
 assert(bindings[0]==11&&bindings[1]==11&&bindings[2]==123&&bindings[3]==0);draws++;
}
static void glDeleteTextures(GLsizei n,const GLuint *v) {assert(n==1&&*v==200);deleted_textures++;}
#include "halo_vita_ui_alpha_probe.h"
int main(void) {
 GLfloat constants[192][4]={{0}},ps[8][4]={{0}},vertices[4*64]={0};
 struct xgpu_texture_description description={1024,256};
 GLboolean initial[32];
 enabled[GL_BLEND]=enabled[GL_SCISSOR_TEST]=enabled[GL_CULL_FACE]=1;
 memcpy(initial,enabled,sizeof(initial));
 vertices[0]=-100;vertices[1]=-100;vertices[128]=9999;vertices[129]=9999;
 /* Small helper bitmap is ignored without spending a probe. */
 description.width=4;halo_vita_ui_alpha_probe(vertices,64,123,&description,constants,ps);assert(!draws&&!created_fbos);
 description.width=1024;
 for(int i=0;i<2;i++) {
  incomplete=i;halo_vita_ui_alpha_probe(vertices,64,123,&description,constants,ps);
  assert(read_fb==8&&draw_fb==9&&active==103);
  assert(bindings[0]==11&&bindings[1]==11&&bindings[2]==123&&!bindings[3]);
  assert(mask[0]&&mask[1]&&mask[2]&&!mask[3]);
  assert(!memcmp(initial,enabled,sizeof(initial)));
  assert(clear_color[0]==.2f&&clear_color[1]==.3f&&clear_color[2]==.4f&&clear_color[3]==.5f);
 }
 assert(draws==1&&reads==3&&finishes==2);
 assert(created_textures==deleted_textures&&created_fbos==deleted_fbos);
 int n=created_fbos;halo_vita_ui_alpha_probe(vertices,64,123,&description,constants,ps);assert(created_fbos==n);
 puts("PASS actual UI alpha probe: two-copy budget, clipped read bounds, original bindings, native state restoration, complete/incomplete FBO lifetime");
}
'''
(out / 'probe.c').write_text(code)
subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                '-I' + str(ROOT / 'port/vita/include'), str(out / 'probe.c'),
                '-o', str(out / 'probe')], check=True)
subprocess.run([str(out / 'probe')], check=True)
