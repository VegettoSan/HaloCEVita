#!/usr/bin/env python3
"""Exercise actual color conversion and attribute format; no GPU proof."""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / 'build/vita/tests/vertex-color'
out.mkdir(parents=True, exist_ok=True)
s = (root / 'port/linux/src/d3d8_gl.c').read_text()
start = s.index('static unsigned long stream_upload_swizzled(')
swizzle = s[start:s.index('\n#endif', s.index('\n\treturn stream_upload(scratch, size);', start))]
start = s.index('static void attribute_format(')
fmt = s[start:s.index('/* upload vertices', start)]
types = sorted(set(re.findall(r'D3DVSDT_\w+', fmt)))
prefix = r'''
#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define XGPU_VERTEX_ATTRIBUTE_COUNT 16
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_FLOAT 2
#define GL_UNSIGNED_BYTE 3
#define GL_SHORT 4
#define GL_BGRA 0x80e1
typedef int GLint, GLboolean;
typedef unsigned GLenum;
struct vertex_element { unsigned long stream, type, offset; };
struct vertex_shader_object { unsigned long element_count; struct vertex_element elements[17]; };
'''
suffix = r'''
int main(void) {
    struct vertex_element e={0,D3DVSDT_D3DCOLOR,4}; GLint n; GLenum t; GLboolean normal;
    attribute_format(&e,&n,&t,&normal);
#if defined(HALO_VITA) || defined(HALO_ANDROID)
    assert(n==4 && t==GL_UNSIGNED_BYTE && normal==GL_TRUE);
    unsigned char src[48], original[48]; for(unsigned i=0;i<48;i++)src[i]=(unsigned char)(i*3);
    memcpy(original,src,48);
    struct vertex_shader_object d={3,{{0,D3DVSDT_D3DCOLOR,4},{1,D3DVSDT_D3DCOLOR,8},{0,D3DVSDT_D3DCOLOR,16}}};
    assert(stream_upload_swizzled(&d,0,src,48,24)==123 && uploaded==48);
    for(unsigned i=0;i<48;i++) { unsigned other=i;
        if(i%24==4 || i%24==16)other=i+2;
        if(i%24==6 || i%24==18)other=i-2;
        assert(copy[i]==src[other]);
    }
    assert(!memcmp(src,original,48));
    stream_upload_swizzled(&d,2,src,48,24); assert(!memcmp(copy,src,48));
    d.element_count=0; stream_upload_swizzled(&d,0,src,48,0); assert(!memcmp(copy,src,48));
#ifdef HALO_VITA
    d.element_count=1; d.elements[0]=(struct vertex_element){0,D3DVSDT_D3DCOLOR,23};
    if(!setjmp(fatal)){stream_upload_swizzled(&d,0,src,48,24);assert(!"bad offset accepted");}
    d.elements[0].offset=4;
    if(!setjmp(fatal)){stream_upload_swizzled(&d,0,src,48,0);assert(!"zero stride accepted");}
    d.element_count=17; for(int i=0;i<17;i++)d.elements[i]=d.elements[0];
    if(!setjmp(fatal)){stream_upload_swizzled(&d,0,src,48,24);assert(!"too many colors accepted");}
#endif
#else
    assert(n==GL_BGRA && t==GL_UNSIGNED_BYTE && normal==GL_TRUE);
#endif
    e.type=D3DVSDT_FLOAT3; attribute_format(&e,&n,&t,&normal); assert(n==3 && t==GL_FLOAT && !normal);
    e.type=D3DVSDT_NORMSHORT2; attribute_format(&e,&n,&t,&normal); assert(n==2 && t==GL_SHORT && normal);
    puts("PASS actual vertex-color byte order, immutable source, stream selection and format");
}
'''
for mode in ['HALO_VITA','HALO_ANDROID','DESKTOP']:
    extra = ''
    if mode == 'HALO_VITA':
        extra += 'static jmp_buf fatal;\n_Noreturn static void vita_fatal(const char *s) { (void)s; longjmp(fatal,1); }\n'
    if mode != 'DESKTOP':
        extra += '''static unsigned char copy[48]; static unsigned long uploaded;
static unsigned long stream_upload(const void *p, unsigned long n) {
assert(n<=48); memcpy(copy,p,n); uploaded=n; return 123; }
'''
    code = prefix + 'enum {' + ','.join(types) + '};\n' + extra
    if mode != 'DESKTOP': code += swizzle
    code += fmt + suffix
    path = out / (mode+'.c'); path.write_text(code)
    binary = out / mode
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-D'+mode,str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
