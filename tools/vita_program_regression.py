#!/usr/bin/env python3
"""CPU contract checks for the actual Vita helpers used by original d3d8_gl.

Sparse/nonconsecutive uniform locations, inactive registers, full register
range, dirty uploads and explicit NV2A attribute names. No GPU execution.
"""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
build = ROOT / 'build/vita/tests/program-a019'
build.mkdir(parents=True, exist_ok=True)
source = r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef unsigned int GLuint;
typedef int GLint;
#define XGPU_VERTEX_ATTRIBUTE_COUNT 16
#define XGPU_VERTEX_CONSTANT_COUNT 192
static unsigned bound, queried, uploaded, mask;
static float expected[192][4];
static unsigned long serials[192];
static void platform_log(const char *format, ...) { (void)format; }
static void glBindAttribLocation(GLuint program, GLuint index, const char *name) {
    char wanted[16]; snprintf(wanted, sizeof(wanted), "v%u_in", index);
    assert(program==7 && index==bound && !strcmp(name,wanted)); ++bound;
}
static GLint glGetUniformLocation(GLuint program, const char *name) {
    unsigned index; char tail;
    assert(program==7 && sscanf(name,"c[%u]%c",&index,&tail)==1 && index==queried);
    ++queried;
    /* c[0] absent, c[5] active, c[191] active with unrelated location. */
    return index==5 ? 2007 : index==191 ? 12 : -1;
}
static void glUniform4fv(GLint location, int count, const float *values) {
    unsigned index=location==2007?5:191;
    assert((location==2007 || location==12) && count==1 && values==expected[index]);
    assert(values[0]==(float)index && values[3]==(float)index+3);
    ++uploaded; mask|=index==5?1:2;
}
#include "halo_vita_program.h"
int main(void) {
    GLint locations[192]; unsigned i,j;
    for(i=0;i<192;++i) { for(j=0;j<4;++j) expected[i][j]=(float)i+(float)j; serials[i]=3; }
    halo_vita_bind_vertex_inputs(7); assert(bound==16);
    halo_vita_find_vertex_constants(7,locations); assert(queried==192 && locations[0]==-1);
    halo_vita_upload_vertex_constants(locations,expected,serials,0);
    assert(uploaded==2 && mask==3);
    uploaded=mask=0; serials[191]=4;
    halo_vita_upload_vertex_constants(locations,expected,serials,3);
    assert(uploaded==1 && mask==2);
    uploaded=mask=0;
    halo_vita_upload_vertex_constants(locations,expected,serials,4);
    assert(uploaded==0);
    for(i=0;i<192;++i) locations[i]=-1;
    halo_vita_upload_vertex_constants(locations,expected,serials,0); assert(uploaded==0);
    puts("PASS: actual D3D8 Vita helpers:16 attribute bindings,192 explicit lookups, sparse locations, fresh/dirty/inactive uploads");
    return 0;
}
'''
(build / 'contract.c').write_text(source)
subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / 'port/vita/include'),
                str(build / 'contract.c'), '-o', str(build / 'contract')], check=True)
subprocess.run([str(build / 'contract')], check=True)
