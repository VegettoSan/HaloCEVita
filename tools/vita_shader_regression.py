#!/usr/bin/env python3
"""Exercise the actual probe's failed-compiler recovery with a mocked SDK.

This verifies that failed code never reaches vitaGL attach/link, while both
stages finish the semantic pair and all allocated objects are released.
It does not validate GLSL acceptance, GXM, or hardware rendering.
"""
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PREFIX = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
typedef unsigned int GLuint, GLenum;
typedef int GLint, GLsizei, shark_log_level;
#define GL_VERTEX_SHADER 1
#define GL_FRAGMENT_SHADER 2
#define GL_COMPILE_STATUS 3
#define GL_SHADER_SOURCE_LENGTH 4
#define GL_LINK_STATUS 5
#define GL_ARRAY_BUFFER 6
#define GL_STATIC_DRAW 7
#define HALO_VITA_DATA_ROOT "./"
static GLuint probe_program, probe_vbo;
static int compiler_available;
static const char *compiler_stage = "vitaGL";
static int scenario, compiles, attaches, links, deleted_shaders, deleted_programs;
static int passed[3];
static void (*log_callback)(const char *, shark_log_level, int);
static void vita_log(const char *format, ...) { (void)format; }
static void shark_install_log_cb(void (*cb)(const char *, shark_log_level, int)) { log_callback=cb; }
static GLuint glCreateShader(GLenum kind) {
    return (scenario==5 && kind==GL_VERTEX_SHADER) || (scenario==6 && kind==GL_FRAGMENT_SHADER) ? 0 : kind;
}
static void glShaderSource(GLuint s, int count, const char *const *text, const GLint *len) {
    assert(s && count==1 && *text && !len);
}
static void glCompileShader(GLuint s) {
    assert(s==(GLuint)++compiles); /* vertex then fragment, including failure */
    passed[s]=!((scenario==1 && s==1) || (scenario==2 && s==2) || scenario==3);
    if(!passed[s]) log_callback("mock compiler rejection",2,42);
}
static void glGetShaderiv(GLuint s, GLenum field, GLint *out) {
    assert(s==1 || s==2);
    if(field==GL_COMPILE_STATUS) *out=passed[s];
    else { assert(field==GL_SHADER_SOURCE_LENGTH); *out=15; }
}
static void glGetShaderInfoLog(GLuint s, GLsizei cap, GLsizei *len, char *out) {
    (void)s; (void)out; assert(cap>0); *len=0; /* SDK without HAVE_SHARK_LOG */
}
static GLenum glGetError(void) { return 0; }
static void glGetShaderSource(GLuint s, GLsizei cap, GLsizei *len, char *out) {
    assert(!passed[s] && cap==15); memcpy(out,"rejected Cg\n",12); *len=12; /* no NUL contract */
}
static GLuint glCreateProgram(void) { assert(passed[1] && passed[2]); return scenario==7 ? 0 : 10; }
static void glAttachShader(GLuint p, GLuint s) {
    assert(p==10 && passed[1] && passed[2] && passed[s]); ++attaches;
}
static void glBindAttribLocation(GLuint p, GLuint index, const char *name) {
    assert(p==10 && ((index==0 && !strcmp(name,"v0_in")) || (index==3 && !strcmp(name,"v3_in"))));
}
static void glLinkProgram(GLuint p) { assert(p==10 && attaches==2 && passed[1] && passed[2]); ++links; }
static void glGetProgramiv(GLuint p, GLenum field, GLint *out) {
    assert(p==10 && field==GL_LINK_STATUS); *out=scenario!=8;
}
static void glGetProgramInfoLog(GLuint p, GLsizei cap, GLsizei *len, char *out) {
    (void)out; assert(p==10 && cap>0); *len=0;
}
static void glDeleteShader(GLuint s) { assert(s==1 || s==2); ++deleted_shaders; }
static void glDeleteProgram(GLuint p) { assert(p==10); ++deleted_programs; }
static void glGenBuffers(GLsizei count, GLuint *buffer) { assert(count==1 && links==1 && scenario==0); *buffer=11; }
static void glBindBuffer(GLenum kind, GLuint b) { assert(kind==GL_ARRAY_BUFFER && (b==11 || b==0)); }
static void glBufferData(GLenum kind, size_t bytes, const void *data, GLenum usage) {
    assert(kind==GL_ARRAY_BUFFER && bytes==96 && data && usage==GL_STATIC_DRAW);
}
'''
SUFFIX = r'''
int main(int argc,char **argv) {
    assert(argc==2); scenario=atoi(argv[1]); compiler_available=scenario!=4;
    assert(vita_graphics_shader_probe("mock vertex", "mock fragment")== (scenario==0));
    assert(compiles==(scenario>=4 && scenario<=6 ? 0 : 2));
    assert(attaches==(scenario==0 || scenario==8 ? 2 : 0));
    assert(links==(scenario==0 || scenario==8 ? 1 : 0));
    assert(deleted_shaders==(scenario==4 ? 0 : scenario==5 || scenario==6 ? 1 : 2));
    assert(deleted_programs==(scenario==8 ? 1 : 0));
    assert(probe_program==(scenario==0 ? 10u : 0u));
    assert(probe_vbo==(scenario==0 ? 11u : 0u));
    for(int s=1;s<=2;s++) {
        if(compiles==2 && !passed[s]) {
            FILE *f=fopen(s==1 ? "vertex_probe.cg" : "fragment_probe.cg","rb");
            char bytes[12]; assert(f && fread(bytes,1,12,f)==12 && fgetc(f)==EOF);
            assert(!memcmp(bytes,"rejected Cg\n",12)); fclose(f);
        }
    }
    printf("PASS actual shader probe: case %d, compiles=%d attach=%d link=%d; cleanup/source dump checked\n",
        scenario,compiles,attaches,links);
}
'''


def main():
    source = (ROOT / 'port/vita/src/vita_graphics.c').read_text()
    source = source[source.index('static void compiler_log'):source.index('/* Original diagnostic glyphs')]
    output = ROOT / 'build/vita/tests'
    output.mkdir(parents=True, exist_ok=True)
    test = output / 'graphics-shader-regression.c'
    test.write_text(PREFIX + source + SUFFIX)
    executable = output / 'graphics-shader-regression'
    subprocess.run([os.environ.get('HOST_CC', 'cc'), '-std=c11', '-Wall', '-Wextra', '-Werror',
                    str(test), '-o', str(executable)], check=True)
    for case in range(9):
        directory = output / f'shader-case-{case}'
        directory.mkdir(exist_ok=True)
        subprocess.run([str(executable), str(case)], cwd=directory, check=True)


if __name__ == '__main__':
    main()
