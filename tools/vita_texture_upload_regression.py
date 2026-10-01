#!/usr/bin/env python3
"""Test actual upload/mip arithmetic against a strict pixel-store boundary.

Vita GL/decode boundaries are mocked. This proves API/layout requests and
phase errors, not driver acceptance, decoded retail pixels or native ABI.
"""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / 'build/vita/tests/texture-upload'
out.mkdir(parents=True, exist_ok=True)
s = (root / 'port/linux/src/xbox_textures.c').read_text()
formats = s[s.index('enum texel_kind'):s.index('static BOOL kind_compressed')]
geometry = s[s.index('static unsigned long level_dimension'):s.index('void xgpu_texture_describe')]
geometry += s[s.index('static unsigned long level_bytes'):s.index('unsigned long xgpu_texture_level_pitch')]
compressed = s[s.index('static GLenum compressed_format'):s.index('/* ---------- upload */')]
body = s[s.index('#ifdef HALO_VITA\nstatic void vita_upload_check'):s.index('/* ---------- cache */')]
prefix = r'''
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define HALO_VITA 1
#define TRUE 1
#define FALSE 0
#define GL_NO_ERROR 0
#define GL_INVALID_ENUM 0x500
#define GL_TEXTURE_2D 0xde1
#define GL_TEXTURE_3D 0x806f
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_UNPACK_ROW_LENGTH 0xcf2
#define GL_UNPACK_ALIGNMENT 0xcf5
#define GL_RGBA8 0x8058
#define GL_BGRA 0x80e1
#define GL_UNSIGNED_BYTE 0x1401
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83f1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83f2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83f3
#define D3DTEXTURE_CUBEFACE_ALIGNMENT 128
typedef unsigned DWORD, GLuint, GLenum, D3DCOLOR;
typedef int BOOL, GLint, GLsizei;
struct xgpu_texture_description { DWORD format;
    unsigned long width,height,depth,levels; BOOL cube_map,linear,compressed;
    unsigned long pitch;
};
static jmp_buf fatal;
static GLenum gl_error;
static char phase_log[256];
static int row_length=37, fail_store, fail_level=-1, compressed_calls, decoded_calls;
static const unsigned char *base;
static const struct xgpu_texture_description *expected;
static void vita_log(const char *f, ...) {
    if(strstr(f,"rejected phase")) { va_list ap; va_start(ap,f); vsnprintf(phase_log,sizeof(phase_log),f,ap); va_end(ap); }
}
_Noreturn static void vita_fatal(const char *s) { (void)s; longjmp(fatal,1); }
static GLenum glGetError(void) { GLenum e=gl_error; gl_error=0; return e; }
static void glBindTexture(GLenum target, GLuint texture) { assert(target==GL_TEXTURE_2D && texture==2); }
static void xgpu_gl_state_invalidate(void) {}
static void glPixelStorei(GLenum p, GLint v) {
    /* Match the source-confirmed vitaGL selector rejection. */
    if(p!=GL_UNPACK_ROW_LENGTH)gl_error=GL_INVALID_ENUM;
    else { row_length=v; if(fail_store)gl_error=GL_INVALID_ENUM; }
}
static void texture_dump(GLenum t, const struct xgpu_texture_description *d) { assert(t==GL_TEXTURE_2D && d==expected); }
'''
prefix = prefix.replace('#include <assert.h>', '#include <assert.h>\n#include <stdarg.h>')
mocks = r'''
static void decode_level(const struct xgpu_texture_description *d, unsigned long l,
    const unsigned char *src, const D3DCOLOR *p, unsigned long *dst) {
    (void)p; assert(d==expected && !d->compressed);
    assert(src==base+xgpu_texture_level_offset(d,l)); dst[0]=0xff332211UL;
}
static void glCompressedTexImage2D(GLenum t, GLint l, GLenum f, GLsizei w, GLsizei h,
    GLint border, GLsizei bytes, const void *p) {
    assert(t==GL_TEXTURE_2D && !border && row_length==0);
    assert(w==(int)level_dimension(expected->width,l) && h==(int)level_dimension(expected->height,l));
    assert(bytes==(int)level_bytes(expected,l) && p==base+xgpu_texture_level_offset(expected,l));
    assert(f==compressed_format(format_information(expected->format).kind));
    compressed_calls++; if(l==fail_level)gl_error=GL_INVALID_ENUM;
}
static void glTexImage2D(GLenum t, GLint l, GLint f, GLsizei w, GLsizei h,
    GLint border, GLenum format, GLenum type, const void *p) {
    assert(t==GL_TEXTURE_2D && !border && row_length==0 && f==GL_RGBA8 && format==GL_BGRA && type==GL_UNSIGNED_BYTE);
    assert(w==(int)level_dimension(expected->width,l) && h==(int)level_dimension(expected->height,l));
    assert(((const unsigned long *)p)[0]==0xff332211UL); decoded_calls++;
    if(l==fail_level)gl_error=GL_INVALID_ENUM;
}
'''
suffix = r'''
#define FAIL(call) do { if(!setjmp(fatal)){call;assert(!"expected fatal");} } while(0)
int main(void) {
    unsigned char data[21888]={0}; base=data;
    struct xgpu_texture_description d={.format=0xe,.width=128,.height=128,.depth=1,.levels=6,.compressed=1};
    expected=&d;
    /* Demonstrate the old call generates the actual reported error code. */
    glPixelStorei(GL_UNPACK_ALIGNMENT,1); assert(glGetError()==0x500);
    assert(xgpu_texture_face_size(&d)==21840);
    assert(xgpu_texture_level_offset(&d,1)==16384 && xgpu_texture_level_offset(&d,2)==20480);
    upload(2,GL_TEXTURE_2D,&d,data,NULL); assert(compressed_calls==6 && !decoded_calls && !gl_error && row_length==0);
    d.format=0xc; compressed_calls=0; upload(2,GL_TEXTURE_2D,&d,data,NULL); assert(compressed_calls==6);
    d.format=0xf; compressed_calls=0; upload(2,GL_TEXTURE_2D,&d,data,NULL); assert(compressed_calls==6);
    d.format=0xe; compressed_calls=0; fail_level=2; FAIL(upload(2,GL_TEXTURE_2D,&d,data,NULL));
    assert(compressed_calls==3 && strstr(phase_log,"compressed mip") && strstr(phase_log,"level=2")); fail_level=-1;
    compressed_calls=0; fail_store=1; FAIL(upload(2,GL_TEXTURE_2D,&d,data,NULL));
    assert(!compressed_calls && strstr(phase_log,"tight rows")); fail_store=0;
    gl_error=0x500; FAIL(upload(2,GL_TEXTURE_2D,&d,data,NULL)); assert(strstr(phase_log,"bind/incoming state"));
    d=(struct xgpu_texture_description){.format=6,.width=3,.height=2,.depth=1,.levels=1};
    row_length=9; upload(2,GL_TEXTURE_2D,&d,data,NULL); assert(decoded_calls==1 && row_length==0);
    fail_level=0; FAIL(upload(2,GL_TEXTURE_2D,&d,data,NULL)); assert(strstr(phase_log,"decoded BGRA mip")); fail_level=-1;
    d.width=0; FAIL(upload(2,GL_TEXTURE_2D,&d,data,NULL));
    d.width=3; d.depth=2; FAIL(upload(2,GL_TEXTURE_3D,&d,data,NULL));
    puts("PASS actual upload: DXT1/3/5 mip metadata, exact DXT3 range, tight BGRA rows and phase/error rejection");
}
'''
p = out / 'upload.c'; p.write_text(prefix+formats+geometry+compressed+mocks+body+suffix)
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(p),'-o',str(out/'upload')],check=True)
subprocess.run([str(out/'upload')],check=True)
