#!/usr/bin/env python3
"""Execute the native Ex adapter and the reused kernel's real TLS APC queue.

I/O faults are injected at the scalar SDK boundary. Actual positioned Vita I/O
is separately exercised by vita_xapi_regression.py. This fixture supplies host
types, not evidence for the ARM32 XDK layout (checked by the native build).
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HEADER = r'''
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
typedef uint32_t DWORD;
typedef int BOOL;
typedef void *HANDLE, *LPVOID;
typedef const void *LPCVOID;
typedef struct { DWORD Internal, InternalHigh, Offset, OffsetHigh; HANDLE hEvent; } OVERLAPPED, *LPOVERLAPPED;
typedef struct { DWORD low, high; } FILETIME, *LPFILETIME;
typedef void (*LPOVERLAPPED_COMPLETION_ROUTINE)(DWORD,DWORD,LPOVERLAPPED);
typedef void (*platform_apc_routine)(void *,void *,void *);
#define WINAPI
#define TRUE 1
#define FALSE 0
#define ERROR_SUCCESS 0
#define ERROR_INVALID_PARAMETER 87
#define ERROR_HANDLE_EOF 38
void SetLastError(DWORD);
DWORD GetLastError(void);
void platform_queue_apc(platform_apc_routine,void *,void *,void *);
long platform_run_apcs(void);
BOOL ReadFileEx(HANDLE,LPVOID,DWORD,LPOVERLAPPED,LPOVERLAPPED_COMPLETION_ROUTINE);
BOOL WriteFileEx(HANDLE,LPCVOID,DWORD,LPOVERLAPPED,LPOVERLAPPED_COMPLETION_ROUTINE);
'''
HARNESS = r'''
#include "platform.h"
#include <assert.h>
static __thread DWORD last_error;
static __thread int calls, fault;
static __thread DWORD expected_error, expected_count;
static __thread OVERLAPPED *expected_request;
static __thread pthread_t issuing;
void SetLastError(DWORD e) { last_error=e; }
DWORD GetLastError(void) { return last_error; }
int vita_xapi_read_at(void *h,void *b,uint32_t n,uint64_t o,uint32_t *d) {
 assert(h==(void *)7 && b && o==UINT64_C(0x100000002));
 *d=fault?0:n/2;SetLastError(fault?6:0);return !fault;
}
int vita_xapi_write_at(void *h,const void *b,uint32_t n,uint64_t o,uint32_t *d) {
 assert(h==(void *)7 && b && o==UINT64_C(0x100000002));
 *d=fault?0:n/2;SetLastError(fault?5:0);return !fault;
}
int vita_xapi_fd_times(void *h,uint64_t t[3]) { (void)h;(void)t;return 0; }
int vita_xapi_set_fd_times(void *h,const uint64_t *a,const uint64_t *b,const uint64_t *c) {
 (void)h;(void)a;(void)b;(void)c;return 0;
}
static void complete(DWORD e,DWORD n,LPOVERLAPPED r) {
 assert(pthread_equal(issuing,pthread_self()));
 assert(r==expected_request && e==expected_error && n==expected_count);
 ++calls;
}
static void one(int write,int failed,int eof) {
 OVERLAPPED r={0};char b[8]={0};issuing=pthread_self();expected_request=&r;
 fault=failed;expected_error=failed?(write?5:6):(eof?38:0);
 expected_count=failed||eof?0:4;r.Offset=2;r.OffsetHigh=1;
 int before=calls;
 BOOL ok=write?WriteFileEx((void *)7,b,8,&r,complete):ReadFileEx((void *)7,b,eof?1:8,&r,complete);
 assert(ok && GetLastError()==0 && calls==before);
 assert(r.Internal==expected_error && r.InternalHigh==expected_count);
 assert(platform_run_apcs()==1 && calls==before+1);
 assert(platform_run_apcs()==0);
}
static void *worker(void *unused) {
 (void)unused;assert(platform_run_apcs()==0);one(1,0,0);return NULL;
}
int main(void) {
 char b[8]={0};OVERLAPPED r={0};
 assert(!ReadFileEx((void *)7,b,8,NULL,complete) && GetLastError()==87);
 assert(!WriteFileEx((void *)7,b,8,&r,NULL) && GetLastError()==87);
 one(0,0,0);one(1,0,0);one(0,1,0);one(1,1,0);one(0,0,1);
 /* Keep a main-thread callback queued while another thread drains its own. */
 pthread_t t;issuing=pthread_self();expected_request=&r;expected_error=0;expected_count=4;
 r.Offset=2;r.OffsetHigh=1;fault=0;
 assert(ReadFileEx((void *)7,b,8,&r,complete));int before=calls;
 assert(!pthread_create(&t,NULL,worker,NULL));assert(!pthread_join(t,NULL));
 assert(calls==before && platform_run_apcs()==1 && calls==before+1);
 return 0;
}
'''


def main():
    kernel = (ROOT / 'port/linux/src/xbox_kernel.c').read_text()
    apcs = kernel.split('/* ---------- asynchronous procedure calls */')[1].split('/* ---------- waiting */')[0]
    with tempfile.TemporaryDirectory(prefix='halo-vita-completions-') as tmp:
        work = Path(tmp)
        (work / 'platform.h').write_text(HEADER)
        (work / 'apcs.c').write_text('#include "platform.h"\n' + apcs)
        (work / 'test.c').write_text(HARNESS)
        binary = work / 'test'
        subprocess.run(['gcc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-I', str(work),
                        str(ROOT / 'port/vita/src/vita_original_file_api.c'),
                        str(work / 'apcs.c'), str(work / 'test.c'), '-pthread', '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, timeout=10)
    print('PASS: actual ReadFileEx/WriteFileEx and TLS APC queue; deferred issuing-thread completion, offset high bits, short I/O, EOF/errors and independent queues')


if __name__ == '__main__':
    main()
