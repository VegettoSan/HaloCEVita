#!/usr/bin/env python3
"""Execute original indexed draw owners; SetIndices must select base + index."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / 'build/vita/tests/index-base'
out.mkdir(parents=True, exist_ok=True)
renderer = (root / 'port/linux/src/d3d8_gl.c').read_text()
setter = renderer[renderer.index('void WINAPI D3DDevice_SetIndices('):renderer.index('void WINAPI D3DDevice_DrawVertices(')]
draw = renderer[renderer.index('void WINAPI D3DDevice_DrawIndexedVertices('):renderer.index('/* ---------- immediate mode */')]
prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#define WINAPI
#define CONST const
#define FALSE 0
#define TRUE 1
#define GL_UNSIGNED_SHORT 1
#define D3DPT_QUADLIST 9
typedef unsigned UINT, GLuint, D3DPRIMITIVETYPE;
typedef int BOOL, GLint, GLsizei;
typedef uint16_t WORD;
typedef struct { uintptr_t Data; } D3DIndexBuffer;
static struct { UINT base_vertex_index; } device;
static struct { unsigned long streamed_bytes; } stats;
static WORD *D3D__IndexData;
static unsigned long first_vertex, vertex_span, calls;
static int prepare_draw(BOOL immediate) { assert(!immediate); return 1; }
static int mirror_range(unsigned long a,unsigned long b,GLuint *c,unsigned long *d,unsigned long *e) {
    (void)a;(void)b;(void)c;(void)d;(void)e;return 0;
}
static void index_extent(const WORD *p,unsigned long n,unsigned long gen,BOOL cached,unsigned long *lo,unsigned long *hi) {
    assert(!gen&&!cached);*lo=*hi=p[0];for(unsigned long i=1;i<n;i++){if(p[i]<*lo)*lo=p[i];if(p[i]>*hi)*hi=p[i];}
}
static void trace_draw(const char *s,D3DPRIMITIVETYPE t,unsigned long n,const float *v) {(void)s;(void)t;(void)n;(void)v;}
static void setup_streams(unsigned long first,unsigned long count) {first_vertex=first;vertex_span=count;calls++;}
static void gl_check_errors(const char *s) {(void)s;}
static void state_element_array_buffer(GLuint b) {(void)b;}
static unsigned primitive_mode(D3DPRIMITIVETYPE t) {return t;}
static void glDrawElementsBaseVertex(unsigned t,GLsizei n,unsigned f,const void *p,GLint base) {
    assert(t==1&&n==3&&f==GL_UNSIGNED_SHORT&&p==(void *)7&&base==-4);
}
static WORD *quad_indices(const WORD *p,unsigned long n,unsigned long *count) {(void)p;(void)n;(void)count;abort();}
static unsigned long index_upload(const void *p,unsigned long n) {assert(p&&n==6);return 7;}
'''
suffix = r'''
int main(void) {
    WORD indices[]={6,4,5};D3DIndexBuffer ib={(uintptr_t)indices};
    D3DDevice_SetIndices(&ib,100);
    assert(D3D__IndexData==indices);
    D3DDevice_DrawIndexedVertices(1,3,indices);
    assert(calls==1&&first_vertex==104&&vertex_span==3);
    D3DDevice_SetIndices(&ib,0);
    D3DDevice_DrawIndexedVertices(1,3,indices);
    assert(calls==2&&first_vertex==4&&vertex_span==3);
    D3DDevice_SetIndices(NULL,65536);
    assert(!D3D__IndexData);
    D3DDevice_DrawIndexedVertices(1,3,indices);
    assert(calls==3&&first_vertex==65540&&vertex_span==3);
    D3DDevice_DrawIndexedVertices(1,0,indices);
    D3DDevice_DrawIndexedVertices(1,3,NULL);
    assert(calls==3);
    puts("PASS original indexed draw: base+minimum stream range, zero base, large base and empty-draw guards; original index rebasing unchanged");
}
'''
source = out / 'contract.c'
source.write_text(prefix + setter + draw + suffix)
subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', str(source), '-o', str(out / 'contract')], check=True)
subprocess.run([str(out / 'contract')], check=True)
