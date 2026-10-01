#!/usr/bin/env python3
"""Run actual texture stage binding with uploads that mutate active-unit state."""
from pathlib import Path
import subprocess
root = Path(__file__).resolve().parents[1]
s = (root/'port/linux/src/d3d8_gl.c').read_text()
state = s[s.index('static void state_texture('):s.index('static void state_sampler(')]
bind = s[s.index('static void bind_textures('):s.index('static GLenum stencil_operation(')]
code = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef unsigned GLenum, GLuint, DWORD, D3DCOLOR;
typedef int BOOL;
#define GL_TEXTURE0 100
#define GL_TEXTURE_2D 1
#define GL_TEXTURE_CUBE_MAP 2
#define GL_TEXTURE_3D 3
#define D3DTSS_MAXSTAGES 4
#define PLATFORM_PHYSICAL_TO_VIRTUAL(p) ((void *)(unsigned long)(p))
enum {_xgpu_sampler_none,_xgpu_sampler_2d,_xgpu_sampler_cube,_xgpu_sampler_3d};
typedef struct {DWORD Data,Format,Size;} D3DBaseTexture;
struct xgpu_texture_description {int linear,cube_map,levels,width,height;};
struct xgpu_render_target {GLuint texture;int width,height;};
struct nv2a_pixel_shader_key {int sampler_type[4];};
static struct {D3DBaseTexture *textures[4],*palettes[4];GLuint samplers[4];} device;
static struct {GLuint textures[4][3];GLenum active_texture;} gl_state;
static unsigned active, bound[4], uploads, dirty[4], mode[4]={1,1,1,1};
static struct xgpu_render_target rt={9,16,8};
static void invalidate(void){memset(&gl_state,0xff,sizeof(gl_state));}
static void glActiveTexture(GLenum t){assert(t>=100 && t<104);active=t-100;}
static void glBindTexture(GLenum t,GLuint id){assert(t==GL_TEXTURE_2D);bound[active]=id;}
static void vita_fatal(const char *s){(void)s;abort();}
static unsigned long stage_texture_mode(int s){return mode[s];}
static struct xgpu_render_target *xgpu_render_target_find(DWORD p){return p==9?&rt:NULL;}
static void xgpu_texture_describe(DWORD f,DWORD z,struct xgpu_texture_description *d){
*d=(struct xgpu_texture_description){0,0,(int)z,16,8};(void)f;}
static GLuint mip_composite_get(const struct xgpu_texture_description *d,DWORD p){
assert(d->levels>1 && p==9);glBindTexture(GL_TEXTURE_2D,19);invalidate();return 19;}
static GLuint xgpu_texture_get(const DWORD *p,const D3DCOLOR *pal,GLenum *t,struct xgpu_texture_description *d){
const D3DBaseTexture *tex=(const D3DBaseTexture *)p;(void)pal;
*t=GL_TEXTURE_2D;*d=(struct xgpu_texture_description){tex->Format==1,0,1,16,8};
if(dirty[tex->Data-1]){glBindTexture(*t,tex->Data);invalidate();dirty[tex->Data-1]=0;uploads++;}
return tex->Data;}
static void state_sampler(int s,GLuint id){(void)s;(void)id;}
static void configure_sampler(int s,BOOL mip){(void)s;(void)mip;}
'''+state+bind+r'''
int main(void){
D3DBaseTexture tex[4]={{1,0,1},{2,0,1},{3,0,1},{4,0,1}};
struct nv2a_pixel_shader_key key;float scale[4][4];
for(int i=0;i<4;i++){device.textures[i]=&tex[i];dirty[i]=1;}invalidate();
bind_textures(&key,scale);assert(uploads==4);
for(int i=0;i<4;i++)assert(bound[i]==(unsigned)i+1 && key.sampler_type[i]==_xgpu_sampler_2d);
/* A refreshed late-stage glyph must not replace an earlier bound image. */
for(int changed=0;changed<4;changed++){
 dirty[changed]=1;bind_textures(&key,scale);
 for(int i=0;i<4;i++)assert(bound[i]==(unsigned)i+1);
}
assert(uploads==8);bind_textures(&key,scale);assert(uploads==8);
tex[2].Format=1;bind_textures(&key,scale);assert(scale[2][0]==1.f/16 && scale[2][1]==1.f/8);
/* Rendered mip composition also binds internally. */
tex[3].Data=9;tex[3].Size=2;bind_textures(&key,scale);
assert(bound[0]==1 && bound[1]==2 && bound[2]==3 && bound[3]==19);
/* Unused/projection stages retain original sampler/identity-scale policy. */
for(int i=0;i<4;i++)mode[i]=i==2?0x11:0;
bind_textures(&key,scale);
for(int i=0;i<4;i++)assert(!bound[i] && scale[i][0]==1 && key.sampler_type[i]==(i==2?_xgpu_sampler_2d:_xgpu_sampler_none));
puts("PASS actual Vita texture-stage binding: cold uploads, refreshed glyphs, warm hits, linear scale, rendered mip composition, unused stages");}
'''
out=root/'build/vita/tests/texture-stage';out.mkdir(parents=True,exist_ok=True)
p=out/'stage.c';p.write_text(code);exe=out/'stage'
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-DHALO_VITA',str(p),'-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
# Demonstrate that the fixture rejects the historical binding order.
old=code.replace(bind,bind[:bind.index('#ifdef HALO_VITA')]+bind[bind.index('#endif')+6:])
p.write_text(old)
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-DHALO_VITA',str(p),'-o',str(exe)],check=True)
failed=subprocess.run([str(exe)],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
assert failed.returncode != 0, 'historical overwrite was not reproduced'
print('PASS historical stage overwrite reproduced')
