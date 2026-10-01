#!/usr/bin/env python3
"""Run actual packed-normal conversion/setup, with mocked GPU buffer requests."""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
s = (root / 'port/linux/src/d3d8_gl.c').read_text()
start = s.index('static unsigned long vita_normal_bytes(')
normal = s[start:s.index('\n#endif', start)]
start = s.index('static void setup_streams(')
setup = s[start:s.index('static GLenum primitive_mode(', start)]
start = s.index('static void attribute_format(')
fmt = s[start:s.index('/* upload vertices', start)]
start = s.index('static unsigned long stream_upload_swizzled(')
swizzle = s[start:s.index('\n#endif', s.index('\n\treturn stream_upload(scratch, size);', start))]
types = sorted(set(re.findall(r'D3DVSDT_\w+', fmt + setup)))
code = r'''
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "halo_vita_vertex.h"
#define XGPU_VERTEX_ATTRIBUTE_COUNT 16
#define STREAM_BUFFER_SIZE 2097152UL
#define TRUE 1
#define FALSE 0
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_FLOAT 2
#define GL_SHORT 3
#define GL_UNSIGNED_BYTE 4
#define GL_UNSIGNED_INT 5
#define GL_BGRA 0x80e1
typedef int BOOL, GLint, GLboolean, GLsizei;
typedef unsigned GLuint, GLenum;
struct vertex_element {unsigned long stream,type,offset,reg;};
struct vertex_shader_object {unsigned long element_count,packed_mask;struct vertex_element elements[17];};
static struct {struct vertex_shader_object *vertex_shader;unsigned stream_buffer;
    unsigned long stream_offset;struct {void *data;unsigned long stride;} streams[16];
    float attributes[16][4];} device;
static struct {unsigned long streamed_bytes;} stats;
static unsigned char gpu[STREAM_BUFFER_SIZE];
static unsigned long reservation, minimum, writes;
static struct {unsigned long offset,stride;int size;unsigned type;} attributes[16];
static jmp_buf failure;
static _Noreturn void vita_fatal(const char *s){(void)s;longjmp(failure,1);}
#define PLATFORM_PHYSICAL_TO_VIRTUAL(p) (p)
static unsigned long vita_stream_span(unsigned long n,unsigned long cap){
    if(!n || n>cap || n>ULONG_MAX-15)vita_fatal("span");
    return (n+15)&~15UL;}
static void stream_reserve(unsigned long n){
    assert(n<=STREAM_BUFFER_SIZE); if(n>STREAM_BUFFER_SIZE-device.stream_offset)device.stream_offset=0;
    reservation=n;minimum=device.stream_offset;}
static unsigned long stream_upload(const void *p,unsigned long n){
    unsigned long off=device.stream_offset, span=vita_stream_span(n,STREAM_BUFFER_SIZE);
    assert(off>=minimum && off+span<=minimum+reservation && off+span<=STREAM_BUFFER_SIZE);
    memcpy(gpu+off,p,n);device.stream_offset+=span;writes++;return off;}
static int mirror_range(unsigned long p,unsigned long n,unsigned *b,unsigned long *off,void *unused){
    (void)p;(void)n;(void)b;(void)off;(void)unused;return FALSE;}
static void state_attribute_pointer(unsigned reg,unsigned buffer,int n,unsigned type,
    int normalized,int integer,int stride,unsigned long offset){
    assert(reg<16 && buffer==device.stream_buffer && !integer && stride>=0);
    (void)normalized;attributes[reg].offset=offset;attributes[reg].stride=(unsigned long)stride;
    attributes[reg].size=n;attributes[reg].type=type;}
static void state_attribute_value(unsigned reg,const float *p){assert(reg<16 && p);}
'''
code += 'enum {' + ','.join(types) + '};\n' + fmt + swizzle + normal + setup
code += r'''
static void expect_failure(struct vertex_shader_object *d,unsigned long first,unsigned long count){
    device.vertex_shader=d;if(!setjmp(failure)){setup_streams(first,count);assert(!"invalid draw accepted");}}
int main(void){
    float n[3];
    for(unsigned i=0;i<2048;i++){
        halo_vita_unpack_normpacked3(i|(i<<11),n);
        int v=i>=1024?(int)i-2048:(int)i;
        assert(n[0]==(float)v/1023.f && n[1]==n[0] && n[2]==0.f);}
    for(unsigned i=0;i<1024;i++){
        halo_vita_unpack_normpacked3(i<<22,n);
        int v=i>=512?(int)i-1024:(int)i;assert(n[2]==(float)v/511.f);}
    unsigned char data[3*32],saved[sizeof(data)],second[3*7];
    memset(data,0x51,sizeof(data));memset(second,0x92,sizeof(second));
    uint32_t p=1023u|(1024u<<11)|(511u<<22),q=2047u|(1u<<11)|(512u<<22);
    memcpy(data+32+3,&p,4);memcpy(data+64+3,&q,4);
    memcpy(second+7+1,&q,4);memcpy(second+14+1,&p,4);memcpy(saved,data,sizeof(data));
    struct vertex_shader_object d={4,(1UL<<2)|(1UL<<7),{
        {0,D3DVSDT_FLOAT2,8,0},{0,D3DVSDT_NORMPACKED3,3,2},
        {0,D3DVSDT_D3DCOLOR,20,9},{1,D3DVSDT_NORMPACKED3,1,7}}};
    device.vertex_shader=&d;device.stream_buffer=99;device.streams[0].data=data;
    device.streams[0].stride=32;device.streams[1].data=second;device.streams[1].stride=7;
    device.stream_offset=STREAM_BUFFER_SIZE-16;setup_streams(1,2);
    assert(minimum==0 && reservation==144 && device.stream_offset==144 && writes==4);
    assert(attributes[2].size==3 && attributes[2].type==GL_FLOAT && attributes[2].stride==12);
    assert(attributes[7].size==3 && attributes[7].type==GL_FLOAT);
    halo_vita_unpack_normpacked3(p,n);assert(!memcmp(gpu+attributes[2].offset,n,12));
    halo_vita_unpack_normpacked3(q,n);assert(!memcmp(gpu+attributes[2].offset+12,n,12));
    assert(!memcmp(gpu+attributes[7].offset,n,12));
    assert(!memcmp(data,saved,sizeof(data)));
    assert(!memcmp(gpu+attributes[0].offset,data+40,8));
    assert(gpu[attributes[9].offset]==data[54] && gpu[attributes[9].offset+2]==data[52]);
    expect_failure(&d,0,0);expect_failure(&d,ULONG_MAX,2);expect_failure(&d,0,STREAM_BUFFER_SIZE);
    d.elements[1].offset=31;expect_failure(&d,0,2);d.elements[1].offset=3;
    d.elements[1].stream=16;expect_failure(&d,0,2);d.elements[1].stream=0;
    d.elements[1].reg=16;expect_failure(&d,0,2);d.elements[1].reg=2;
    /* Raw streams individually fit but conversion makes the draw too large. */
    expect_failure(&d,0,45000);
    /* Xbox stride0 repeats the same attribute for every vertex. */
    d.element_count=1;d.elements[0]=(struct vertex_element){0,D3DVSDT_NORMPACKED3,3,2};
    device.streams[0].stride=0;device.stream_offset=0;setup_streams(0,3);
    assert(reservation==112);assert(!memcmp(gpu+attributes[2].offset,gpu+attributes[2].offset+12,12));
    puts("PASS packed normals: exhaustive signed11/10, original asymmetry, unaligned mixed streams, first/count, stride0, source immutability, aggregate/wrap and7 rejection cases");
}
'''
out = root / 'build/vita/tests/packed-vertex'
out.mkdir(parents=True, exist_ok=True)
p = out / 'packed.c'
p.write_text(code)
exe = out / 'packed'
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',
                '-DHALO_VITA','-I'+str(root/'port/vita/include'),str(p),'-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
