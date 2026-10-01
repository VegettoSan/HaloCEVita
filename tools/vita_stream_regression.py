#!/usr/bin/env python3
"""Run actual stream functions against a bounded in-flight buffer model.

Models vitaGL6e7fe40 SubData cloning and mapping use markers, not GPU output.
The historical uploader exhausts clone capacity; the Vita path must reuse
storage after synchronization, copy exact bytes and preserve draw reservations.
"""
from pathlib import Path
import os
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PREFIX = r'''
#define _GNU_SOURCE
#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
typedef unsigned GLuint, GLenum;
typedef long GLintptr, GLsizeiptr;
#define GL_ARRAY_BUFFER 1
#define GL_ELEMENT_ARRAY_BUFFER 2
#define GL_STREAM_DRAW 3
#define GL_MAP_WRITE_BIT 4
#define STREAM_BUFFER_SIZE 64UL
#define INDEX_BUFFER_SIZE 32UL
static struct { unsigned long stream_offset, index_offset; GLuint stream_buffer,index_buffer; } device={0,0,11,12};
static unsigned char vertex[64], indices[32];
static unsigned finished, mapped, unmapped, cloned, allocations, driver_abort;
static int in_flight, map_failure, unmap_failure, gl_error, fatal;
static jmp_buf recovery;
static void vita_log(const char *f,...) {(void)f;}
static _Noreturn void vita_fatal(const char *f,...) {(void)f;fatal=1;longjmp(recovery,1);}
static void gl_check_errors(const char *where) {(void)where;if(gl_error)vita_fatal("GL error");}
static void state_array_buffer(GLuint b) {assert(b==11);}
static void state_element_array_buffer(GLuint b) {assert(b==12);}
static void glFinish(void) {finished++;in_flight=0;}
static void *glMapBufferRange(GLenum t,GLintptr off,GLsizeiptr n,unsigned bits) {
    assert(!in_flight && bits==GL_MAP_WRITE_BIT && n>0 && off>=0);
    unsigned long cap=t==GL_ARRAY_BUFFER?64:32;
    assert((unsigned long)off+(unsigned long)n<=cap);mapped++;
    return map_failure?NULL:(t==GL_ARRAY_BUFFER?vertex:indices)+off;
}
static int glUnmapBuffer(GLenum t) {assert(t==1||t==2);unmapped++;return !unmap_failure;}
static void glBufferData(GLenum t,unsigned long n,const void *p,GLenum usage) {
    assert((t==1||t==2)&&!p&&usage==3&&n==(t==1?64:32));allocations++;
}
static void glBufferSubData(GLenum t,GLintptr off,GLsizeiptr n,const void *p) {
    assert(t==1||t==2);
    /* Primary buffers.c: each in-flight update allocates a full-size copy.
       This model has room for two pending copies before NULL allocation. */
    if(in_flight && ++cloned>2) {driver_abort=1;longjmp(recovery,1);}
    memcpy((t==1?vertex:indices)+off,p,(size_t)n);
}
'''
SUFFIX = r'''
int main(void) {
    (void)glBufferData;(void)glBufferSubData;(void)glMapBufferRange;(void)glUnmapBuffer;
    (void)glFinish;(void)gl_check_errors;(void)vita_log;(void)index_upload;
    unsigned char src[64];for(unsigned i=0;i<64;i++)src[i]=(unsigned char)(i+1);
#ifdef HISTORICAL
    if(!setjmp(recovery)) {
        for(unsigned i=0;i<8;i++) {stream_upload(src,16);in_flight=1;}
        assert(!"historical cloning must exhaust the bounded model");
    }
    assert(driver_abort && cloned==3);
    puts("PASS historical actual uploader reproduces in-flight clone exhaustion");
#else
    /* Hundreds of real upload/draw transitions use the same allocations. */
    for(unsigned i=0;i<512;i++) {
        unsigned long off=stream_upload(src,17);assert(off==((i%2)*32));
        assert(!memcmp(vertex+off,src,17));in_flight=1;
        unsigned long idx=index_upload(src,6);assert(idx==((i%2)*16));
        assert(!memcmp(indices+idx,src,6));in_flight=1;
    }
    assert(!cloned && !allocations && finished==1024 && mapped==1024 && unmapped==1024);
    /* Reserve ALL streams of a draw before setting attribute offsets. */
    device.stream_offset=48;stream_reserve(48);assert(device.stream_offset==0);
    assert(stream_upload(src,17)==0);assert(stream_upload(src+17,15)==32);
    assert(!memcmp(vertex,src,17)&&!memcmp(vertex+32,src+17,15));
    device.stream_offset=64;assert(stream_upload(src,16)==0); /* next-frame marker */
    device.index_offset=32;assert(index_upload(src,6)==0);
    /* Source ends exactly at an inaccessible page: padding cannot be read. */
    long page=sysconf(_SC_PAGESIZE);assert(page>0);
    unsigned char *guard=mmap(NULL,(size_t)page*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(guard!=MAP_FAILED && !mprotect(guard+page,(size_t)page,PROT_NONE));
    memcpy(guard+page-13,src,13);device.stream_offset=0;
    assert(stream_upload(guard+page-13,13)==0);assert(!memcmp(vertex,src,13));
    device.index_offset=0;assert(index_upload(guard+page-6,6)==0);
    assert(!munmap(guard,(size_t)page*2));
    for(unsigned problem=0;problem<10;problem++) {
        device.stream_offset=0;device.index_offset=0;fatal=0;
        unsigned char before[64];memcpy(before,vertex,64);
        unsigned previous=unmapped;
        map_failure=problem==0;unmap_failure=problem==1;gl_error=problem==2;
        if(!setjmp(recovery)) {
            switch(problem) {
            case 0:case 1:case 2:stream_upload(src,16);break;
            case 3:stream_upload(src,65);break;
            case 4:index_upload(src,33);break;
            case 5:stream_upload(src,~0UL);break;
            case 6:stream_upload(src,0);break;
            case 7:stream_reserve(65);break;
            case 8:device.index_offset=33;index_upload(src,6);break;
            case 9:stream_upload(NULL,16);break;
            }
            assert(!"invalid write must stop");
        }
        assert(fatal);
        if(problem!=1) {assert(unmapped==previous);assert(!memcmp(before,vertex,64));}
        map_failure=unmap_failure=gl_error=0;
    }
    puts("PASS actual Vita stream/index: 512 draws, bounded reuse, synchronization, multi-stream wrap, exact guard-page bytes and 10 failures");
#endif
}
'''


def functions(source):
    begin = source.index('/* makes room for size bytes of uploads, orphaning')
    end = source.index('#if defined(HALO_ANDROID) || defined(HALO_VITA)\n/* stream_upload', begin)
    index = source.index('static unsigned long index_upload(')
    return source[begin:end] + source[index:source.index('static void attribute_format(', index)]


def main():
    output = ROOT / 'build/vita/tests'
    output.mkdir(parents=True, exist_ok=True)
    source = (ROOT / 'port/linux/src/d3d8_gl.c').read_text()
    helper = source[source.index('#ifdef HALO_VITA\n/* vitaGL SubData'):source.index('/* makes room for size bytes of uploads, orphaning')]
    old = subprocess.check_output(['git', 'show', '940566584ea55f742a51488533915e3797dc9397:port/linux/src/d3d8_gl.c'], cwd=ROOT, text=True)
    # Excluded Vita branches must preserve the original other-backend tokens.
    for define in [[], ['-DHALO_ANDROID']]:
        outputs = [subprocess.check_output([os.environ.get('HOST_CC','cc'),'-E','-P','-x','c',*define,'-'], input=functions(text), text=True) for text in [old,source]]
        assert re.sub(r'\s+', '', outputs[0]) == re.sub(r'\s+', '', outputs[1])
    print('PASS desktop/Android stream uploader preprocessed tokens unchanged')
    for label, code in [('historical',functions(old)), ('vita',helper+functions(source))]:
        test = output / f'stream-{label}.c'
        test.write_text(PREFIX + code + SUFFIX)
        exe = output / f'stream-{label}'
        flags = ['-DHISTORICAL'] if label=='historical' else []
        subprocess.run([os.environ.get('HOST_CC','cc'),'-std=c11','-Wall','-Wextra','-Werror','-DHALO_VITA',*flags,str(test),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)


if __name__=='__main__':
    main()
