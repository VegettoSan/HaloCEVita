#!/usr/bin/env python3
"""Execute actual Vita texture lookup/eviction and exact source shadow helpers."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
s = (root/'port/linux/src/xbox_textures.c').read_text()
start=s.index('struct texture_entry\n')
body=s[start:]
code=r'''
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint32_t DWORD, D3DCOLOR;
typedef unsigned GLuint, GLenum;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define GL_TEXTURE_CUBE_MAP 1
#define GL_TEXTURE_3D 2
#define GL_TEXTURE_2D 3
#define D3DFORMAT_FORMAT_MASK 0xff
#define D3DFORMAT_FORMAT_SHIFT 0
struct xgpu_texture_description {unsigned long width,height,depth,levels,format;BOOL cube_map;};
static unsigned char source[8UL<<20];
static unsigned uploads,deleted,generated;
static int fail_allocation;
static jmp_buf failure;
static void *shadow_malloc(size_t n){return fail_allocation?NULL:malloc(n);}
#define malloc shadow_malloc
#define PLATFORM_PHYSICAL_TO_VIRTUAL(p) (source+(p))
static int platform_is_contiguous(const void *p){return (uintptr_t)p>=(uintptr_t)source &&
    (uintptr_t)p<(uintptr_t)source+sizeof(source);}
static void xgpu_texture_describe(DWORD f,DWORD n,struct xgpu_texture_description *d){
    *d=(struct xgpu_texture_description){n,1,1,1,f,0};}
static unsigned long xgpu_texture_face_size(const struct xgpu_texture_description *d){return d->width;}
static void glGenTextures(int n,GLuint *t){assert(n==1);*t=++generated;}
static void glDeleteTextures(int n,const GLuint *t){assert(n==1 && *t);deleted++;}
static void xgpu_gl_state_invalidate(void){}
static int config_boolean(const char *key){(void)key;return 0;}
static void platform_log(const char *format,...){(void)format;}
static _Noreturn void vita_fatal(const char *s){(void)s;longjmp(failure,1);}
static void upload(GLuint t,GLenum target,const struct xgpu_texture_description *d,
    const unsigned char *base,const D3DCOLOR *p){(void)p;assert(t && target==GL_TEXTURE_2D && d->width && base);uploads++;}
'''+body+r'''
static GLuint get(DWORD data,DWORD size,const D3DCOLOR *p){
    DWORD resource[5]={0,data,0,p?0x0b:0x0e,size};GLenum target;struct xgpu_texture_description desc;
    GLuint result=xgpu_texture_get(resource,p,&target,&desc);assert(target==GL_TEXTURE_2D && desc.width==size);return result;}
static void expire(void){for(int i=0;i<2400;i++)xgpu_texture_cache_begin_frame();assert(!vita_texture_shadow_bytes);}
int main(void){
    unsigned char saved[16];memset(source,0x32,sizeof(source));memcpy(saved,source+1,16);
    GLuint t=get(1,16,NULL);assert(uploads==1 && vita_texture_shadow_bytes==16);
    for(int i=0;i<128;i++)assert(get(1,16,NULL)==t);
    assert(uploads==1 && !memcmp(saved,source+1,16));
    /* Single-byte mutation, including the last byte, always invalidates. */
    source[16]++;assert(get(1,16,NULL)==t && uploads==2);
    source[16]--;assert(get(1,16,NULL)==t && uploads==3);
    D3DCOLOR p[256]={0};assert(get(32,16,p)!=t && uploads==4);
    get(32,16,p);assert(uploads==4);p[255]=7;get(32,16,p);assert(uploads==5);
    /* Source and palette equality both matter, regardless of page serial. */
    source[47]++;get(32,16,p);assert(uploads==6);
    expire();assert(deleted==generated);
    uploads=0;fail_allocation=1;get(64,16,NULL);get(64,16,NULL);
    assert(uploads==2 && !vita_texture_shadow_bytes);fail_allocation=0;get(64,16,NULL);get(64,16,NULL);
    assert(uploads==3 && vita_texture_shadow_bytes==16);expire();
    uploads=0;
    for(unsigned i=0;i<5;i++)get(1+i*(1UL<<20),1UL<<20,NULL);
    assert(uploads==5 && vita_texture_shadow_bytes==VITA_TEXTURE_SHADOW_LIMIT);
    for(unsigned i=0;i<5;i++)get(1+i*(1UL<<20),1UL<<20,NULL);
    assert(uploads==6); /* four equal hits and one safe fallback */
    expire();uploads=0;get(1,VITA_TEXTURE_SHADOW_LIMIT+1,NULL);get(1,VITA_TEXTURE_SHADOW_LIMIT+1,NULL);
    assert(uploads==2 && !vita_texture_shadow_bytes);expire();
    /* Reject range on cache hits too, before memcmp can read beyond arena. */
    if(!setjmp(failure)){get(sizeof(source)-1,16,NULL);assert(!"invalid source accepted");}
    puts("PASS actual Vita texture cache:128 unchanged hits, glyph/source/palette mutations, allocation/budget fallback,4MiB cap, eviction ownership and range rejection");
}
'''
out=root/'build/vita/tests/texture-cache';out.mkdir(parents=True,exist_ok=True)
p=out/'cache.c';p.write_text(code);exe=out/'cache'
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-DHALO_VITA',str(p),'-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
