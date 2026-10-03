#!/usr/bin/env python3
"""Compile the shipping native thread bootstrap against a deterministic SDK fixture."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HEADER = r'''
#include <stddef.h>
typedef unsigned int SceSize;
typedef int SceUID;
typedef int (*SceKernelThreadEntry)(SceSize, void *);
int sceKernelGetThreadId(void);
void sceKernelExitThread(int);
SceUID sceKernelCreateThread(const char *, SceKernelThreadEntry, int, unsigned int, int, int, void *);
int sceKernelStartThread(SceUID, SceSize, void *);
int sceKernelWaitThreadEnd(SceUID, int *, void *);
int sceKernelDeleteThread(SceUID);
void vita_log(const char *, ...);
int halo_vita_run(int);
int vita_run_engine(int);
'''
TEST = r'''
#include <assert.h>
#include <string.h>
static SceKernelThreadEntry entry;
static int mode, created, started, waited, deleted, ran, exited, status;
void vita_log(const char *format, ...) {(void)format;}
int sceKernelGetThreadId(void) {return 7;}
void sceKernelExitThread(int result) {++exited;status=result;}
int halo_vita_run(int platform) {assert(platform==1);++ran;return 23;}
SceUID sceKernelCreateThread(const char *name,SceKernelThreadEntry fn,int priority,
 unsigned int stack,int attributes,int affinity,void *options) {
 assert(!strcmp(name,"HaloCE engine"));assert(priority==0x10000100);
 assert(stack==16u*1024u*1024u);assert(!attributes && !affinity && !options);
 ++created;entry=fn;return mode==1 ? -1 : 7;
}
int sceKernelStartThread(SceUID id,SceSize size,void *argument) {
 assert(id==7 && size==sizeof(int));++started;
 if(mode==2)return -2;
 int copied=*(int *)argument;status=entry(size,&copied);return 0;
}
int sceKernelWaitThreadEnd(SceUID id,int *result,void *timeout) {
 assert(id==7 && !timeout);++waited;if(mode==3)return -3;*result=status;return 0;
}
int sceKernelDeleteThread(SceUID id) {assert(id==7);++deleted;return mode==4 ? -4 : 0;}
static void reset(int failure) {mode=failure;created=started=waited=deleted=ran=exited=status=0;}
int main(void) {
 reset(0);assert(vita_run_engine(0)==1);assert(!created && !ran);
 reset(0);assert(vita_run_engine(1)==23);assert(created==1 && started==1 && waited==1 && deleted==1 && ran==1 && exited==1);
 reset(1);assert(vita_run_engine(1)==1);assert(created==1 && !started && !waited && !deleted && !ran);
 reset(2);assert(vita_run_engine(1)==1);assert(created==1 && started==1 && !waited && deleted==1 && !ran);
 reset(3);assert(vita_run_engine(1)==1);assert(waited==1 && !deleted && ran==1);
 reset(4);assert(vita_run_engine(1)==1);assert(waited==1 && deleted==1 && ran==1);
 assert(entry(0,NULL)==1);assert(entry(sizeof(int),NULL)==1);
 return 0;
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    (path / 'psp2/kernel').mkdir(parents=True)
    (path / 'psp2/kernel/threadmgr.h').write_text(HEADER)
    (path / 'vita_runtime.h').write_text('')
    (path / 'test.c').write_text(HEADER + TEST)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-I' + directory,
                    str(ROOT / 'port/vita/src/vita_bootstrap.c'), str(path / 'test.c'),
                    '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS actual bootstrap: 16MiB engine stack, same-thread lifecycle, status, failure cleanup, no default-stack fallback')
