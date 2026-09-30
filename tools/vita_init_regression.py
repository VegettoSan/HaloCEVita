#!/usr/bin/env python3
"""Host regression for Vita init return handling; mocks SDK, never proves GXM.

Compiles the actual initialization function and its state from vita_graphics.c.
Checks normal return0, resolution-fallback return1, missing context metadata,
and repeated-call safety. Requires a host C compiler, not arm-vita-eabi-gcc.
"""
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PREFIX = r'''

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
typedef unsigned char GLboolean, GLubyte;
typedef int GLint;
typedef struct { int unused; } SceIoStat;
#define VGL_MODE_SHADER_PAIR 0
#define SCE_GXM_MULTISAMPLE_NONE 0
#define GL_VIEWPORT 1
#define GL_VERSION 2
#define GL_VENDOR 3
#define GL_RENDERER 4
#define GL_SHADING_LANGUAGE_VERSION 5
#define HALO_VITA_LEGACY_SIZE 2097152
#define HALO_VITA_GL_RAM_SIZE 16777216
#define HALO_VITA_GL_CDRAM_SIZE 25165824
static int case_id, init_calls, actual_width, actual_height;
static int sceIoGetstat(const char *p, SceIoStat *s) { (void)p; (void)s; return 0; }
static void vita_log(const char *f, ...) { (void)f; }
static void vglSetSemanticBindingMode(int m) { (void)m; }
static GLboolean vglInitWithCustomSizes(int a,int b,int c,int d,int e,int f,int g,int h) {
    assert(a==2097152 && b==960 && c==544 && d==16777216 && e==25165824 && f==0 && g==0 && h==0);
    ++init_calls; return case_id==1;
}
static void glGetIntegerv(int pname, int *v) {
    assert(pname==GL_VIEWPORT);v[0]=v[1]=0;v[2]=case_id==3 ? 0 : case_id==1 ? 480 : 960;v[3]=case_id==1 ? 272 : 544;
}
static const GLubyte *glGetString(int name) { return case_id==2 && name==GL_VERSION ? NULL : (const GLubyte *)"MOCK GL"; }
static void glViewport(int x,int y,int w,int h) { assert(x==0 && y==0);actual_width=w;actual_height=h; }
static void vita_free_memory(uint32_t *a,uint32_t *b,uint32_t *c) { *a=*b=*c=1024; }
'''

SUFFIX = r'''
int main(int argc,char **argv) {
    assert(argc==2);case_id=atoi(argv[1]);
    int expected=case_id<2;
    assert(vita_graphics_initialize()==expected);
    assert(vita_graphics_initialize()==expected);
    assert(init_calls==1);
    if(expected) assert(actual_width==(case_id==1 ? 480 : 960) && actual_height==(case_id==1 ? 272 : 544));
    printf("PASS actual initialization function: case %d (%s); no double initialization\n",case_id,
      case_id==0 ? "return 0 accepted" : case_id==1 ? "return 1 fallback accepted" : case_id==2 ? "NULL version rejected" : "zero viewport rejected");
}
'''

def main():
    source = (ROOT / 'port/vita/src/vita_graphics.c').read_text()
    start = source.index('static int compiler_available;')
    end = source.index('static void compiler_log')
    output = ROOT / 'build/vita/tests'
    output.mkdir(parents=True, exist_ok=True)
    test = output / 'graphics-init-regression.c'
    test.write_text(PREFIX + source[start:end] + SUFFIX)
    executable = output / 'graphics-init-regression'
    subprocess.run([os.environ.get('HOST_CC', 'cc'), '-std=c11', '-Wall', '-Wextra', '-Werror',
                    str(test), '-o', str(executable)], check=True)
    for case in range(4):
        subprocess.run([str(executable), str(case)], check=True)


if __name__ == '__main__':
    main()
