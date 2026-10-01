#!/usr/bin/env python3
"""Execute real Vita base-array reflection/upload helpers.

Model the installed non-strict driver: base-only handles, offset zero and
compiled spans. Check original register indices, dirty updates and bounds.
No GPU/compiler acceptance or visible-menu claim.
"""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
build = ROOT / 'build/vita/tests/program-a062'
build.mkdir(parents=True, exist_ok=True)
source = r'''
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef unsigned GLuint, GLenum;
typedef int GLint, GLsizei;
#define XGPU_VERTEX_ATTRIBUTE_COUNT 16
#define XGPU_VERTEX_CONSTANT_COUNT 192
#define GL_ACTIVE_UNIFORMS 1
#define GL_FLOAT_VEC4 2
static unsigned bound, uploaded, indexed, queried;
static int problem, absent, reported=60, pixel_count=2;
static jmp_buf failure;
static float constants[192][4], pixels[8][4], gpu_vertex[192][4], gpu_pixel[8][4];
static unsigned long serials[192];
static void platform_log(const char *f,...) {(void)f;}
static _Noreturn void vita_fatal(const char *f,...) {(void)f;longjmp(failure,1);}
static void glBindAttribLocation(GLuint p,GLuint i,const char *name) {
    char expected[16];snprintf(expected,sizeof(expected),"v%u_in",i);
    assert(p==7&&i==bound&&!strcmp(expected,name));bound++;
}
static void glGetProgramiv(GLuint p,GLenum key,GLint *value) {
    assert(p==7&&key==GL_ACTIVE_UNIFORMS);*value=problem==5?-1:2;
}
static void glGetActiveUniform(GLuint p,GLuint index,GLsizei cap,GLsizei *length,
    GLint *count,GLenum *type,char *name) {
    assert(p==7&&index<2&&cap>=16);
    const char *s=index==0?(absent?"unrelated":problem==6?"c[0]":"c"):"ps_c0";
    strcpy(name,s);*length=(int)strlen(s);*count=index==0?reported:pixel_count;
    *type=problem==1?99:GL_FLOAT_VEC4;queried++;
}
static GLint glGetUniformLocation(GLuint p,const char *name) {
    assert(p==7);if(strchr(name,'[')){indexed++;return -1;}
    if(!strcmp(name,"c"))return problem==4?-1:100;
    if(!strcmp(name,"ps_c0"))return 200;
    return -1;
}
static void glUniform4fv(GLint loc,GLsizei count,const float *values) {
    assert(loc==100||loc==200);
    if(loc==100){assert(count==reported&&values==constants[0]);memcpy(gpu_vertex,values,(size_t)count*16);}
    else {assert(count==pixel_count&&values==pixels[0]);memcpy(gpu_pixel,values,(size_t)count*16);}
    uploaded++;
}
#include "halo_vita_program.h"
int main(void) {
    struct halo_vita_uniform_array c,p;
    float shadow[8][4];
    for(unsigned i=0;i<192;i++) {serials[i]=3;for(unsigned j=0;j<4;j++)constants[i][j]=(float)(i*4+j);}
    for(unsigned i=0;i<8;i++)for(unsigned j=0;j<4;j++)pixels[i][j]=(float)(i*4+j);
    memset(gpu_vertex,0xaa,sizeof(gpu_vertex));memset(gpu_pixel,0xaa,sizeof(gpu_pixel));
    unsigned char untouched[16];memcpy(untouched,gpu_vertex[60],16);
    halo_vita_bind_vertex_inputs(7);assert(bound==16);
    /* Historical indexed lookup returns no handle in this exact-name model. */
    assert(glGetUniformLocation(7,"c[28]")==-1);indexed=0;
    halo_vita_find_vertex_constants(7,&c);assert(c.location==100&&c.count==60&&!indexed);
    halo_vita_upload_vertex_constants(&c,constants,serials,0);
    assert(uploaded==1&&!memcmp(gpu_vertex,constants,60*16)&&!memcmp(untouched,gpu_vertex[60],16));
    uploaded=0;serials[191]=4;
    halo_vita_upload_vertex_constants(&c,constants,serials,3);assert(!uploaded);
    serials[28]=4;constants[28][0]=888;
    halo_vita_upload_vertex_constants(&c,constants,serials,3);assert(uploaded==1&&gpu_vertex[28][0]==888);
    uploaded=0;halo_vita_upload_vertex_constants(&c,constants,serials,4);assert(!uploaded);
    memset(shadow,0xff,sizeof(shadow));
    halo_vita_find_uniform_array(7,"ps_c0",&p,8);assert(p.count==2&&p.location==200);
    halo_vita_upload_uniform_array(&p,shadow,pixels);assert(uploaded==1&&!memcmp(gpu_pixel,pixels,32));
    assert(!memcmp(untouched,gpu_pixel[2],16));
    halo_vita_upload_uniform_array(&p,shadow,pixels);assert(uploaded==1);
    pixels[7][0]=123;halo_vita_upload_uniform_array(&p,shadow,pixels);assert(uploaded==1);
    pixels[1][1]=77;halo_vita_upload_uniform_array(&p,shadow,pixels);assert(uploaded==2&&gpu_pixel[1][1]==77);
    /* Full192 reflected spans retain every original index; no compaction. */
    reported=192;halo_vita_find_vertex_constants(7,&c);halo_vita_upload_vertex_constants(&c,constants,serials,0);
    assert(!memcmp(gpu_vertex,constants,sizeof(constants)));
    absent=1;halo_vita_find_vertex_constants(7,&c);assert(c.count==0&&c.location==-1);
    uploaded=0;halo_vita_upload_vertex_constants(&c,constants,serials,0);assert(!uploaded);absent=0;
    /* Driver may return base or [0] reflection names, never invented offsets. */
    problem=6;halo_vita_find_vertex_constants(7,&c);assert(c.count==192&&!indexed);
    for(problem=1;problem<=5;problem++) {
        reported=problem==2?193:problem==3?0:60;
        if(!setjmp(failure)) {halo_vita_find_vertex_constants(7,&c);assert(!"invalid reflection must reject");}
    }
    assert(queried>0&&!indexed);
    puts("PASS actual Vita base-array helpers: non-strict handles, compiled bounds/guards, original c[28]/c[191], dirty/absent arrays and 5 invalid reflection cases");
}
'''
(build / 'contract.c').write_text(source)
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-I'+str(ROOT/'port/vita/include'),str(build/'contract.c'),'-o',str(build/'contract')],check=True)
subprocess.run([str(build/'contract')],check=True)
