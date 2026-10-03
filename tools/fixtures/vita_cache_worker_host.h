
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <zlib.h>
typedef unsigned char byte,boolean;
typedef float real;
typedef void *HANDLE;
typedef int BOOL;
typedef struct {uint32_t Internal,InternalHigh,Offset,OffsetHigh;HANDLE hEvent;} OVERLAPPED;
typedef union {int64_t QuadPart;struct{uint32_t LowPart;int32_t HighPart;}u;} LARGE_INTEGER;
#define _error_silent 0
#define error(...) ((void)0)
#define SwitchToThread() sched_yield()
#define CALLBACK
#define WINAPI
#define __stdcall
#define NONE -1
#define TRUE 1
#define FALSE 0
#define FLAG(i) (1U<<(i))
#define TEST_FLAG(f,i) ((f)&FLAG(i))
#define SET_FLAG(f,i,v) ((v)?((f)|=FLAG(i)):((f)&=~FLAG(i)))
#define BIT_VECTOR_SIZE_IN_LONGS(n) (((n)+31)/32)
#define BIT_VECTOR_TEST_FLAG(f,i) TEST_FLAG((f)[(i)/32],(i)%32)
#define BIT_VECTOR_SET_FLAG(f,i,v) SET_FLAG((f)[(i)/32],(i)%32,v)
#define csmemset memset
#define csstrcpy strcpy
#define csprintf(d,...) (sprintf(d,__VA_ARGS__),(d))
#define match_assert(f,l,c) do{if(!(c)){fprintf(stderr,"assert line %d: %s\n",l,#c);abort();}}while(0)
#define match_vassert(f,l,c,m) do{if(!(c)){fprintf(stderr,"assert line %d: %s (%s)\n",l,#c,m);abort();}}while(0)
#define WAIT_OBJECT_0 0
#define WAIT_IO_COMPLETION 192
#define WAIT_TIMEOUT 258
#define INFINITE UINT32_MAX
#define THREAD_PRIORITY_ABOVE_NORMAL 1
#define THREAD_PRIORITY_NORMAL 0
#define ERROR_SUCCESS 0
#define ERROR_INVALID_USER_BUFFER 1784
#define ERROR_NOT_ENOUGH_MEMORY 8
#define ERROR_NO_SYSTEM_RESOURCES 1450
#define GENERIC_READ 0x80000000
#define OPEN_EXISTING 3
#define FILE_FLAG_NO_BUFFERING 1
#define FILE_FLAG_OVERLAPPED 2
#define INVALID_HANDLE_VALUE ((void *)-1)
#define PAGE_READWRITE 4
#define PAGE_READONLY 2
#define _cache_copy_bad_file_failure 0
#define _cache_copy_read_failure 1
#define _cache_copy_write_failure 2
#define _cache_copy_in_progress 3
#define _cache_copy_finished 4
struct event {pthread_mutex_t lock;pthread_cond_t cond;int manual,signaled;};
HANDLE CreateEvent(void*a,int m,int v,void*b){struct event*e=calloc(1,sizeof(*e));pthread_mutex_init(&e->lock,0);pthread_cond_init(&e->cond,0);e->manual=m;e->signaled=v;return e;}
int SetEvent(HANDLE h){struct event*e=h;pthread_mutex_lock(&e->lock);e->signaled=1;pthread_cond_signal(&e->cond);pthread_mutex_unlock(&e->lock);return 1;}
int ResetEvent(HANDLE h){struct event*e=h;pthread_mutex_lock(&e->lock);e->signaled=0;pthread_mutex_unlock(&e->lock);return 1;}
typedef void (*platform_apc_routine)(void*,void*,void*);


/* PRODUCTION_APCS */
unsigned WaitForSingleObjectEx(HANDLE h,unsigned ms,int alert){if(alert&&platform_run_apcs())return 192;struct event*e=h;pthread_mutex_lock(&e->lock);if(!e->signaled && ms==0){pthread_mutex_unlock(&e->lock);return 258;}while(!e->signaled)pthread_cond_wait(&e->cond,&e->lock);if(!e->manual)e->signaled=0;pthread_mutex_unlock(&e->lock);return 0;}
unsigned WaitForSingleObject(HANDLE h,unsigned ms){return WaitForSingleObjectEx(h,ms,0);}
unsigned SleepEx(unsigned ms,int alert){if(alert&&platform_run_apcs())return 192;usleep(ms*1000);return 0;}
void Sleep(unsigned ms){usleep(ms*1000);}
static __thread unsigned err;
void SetLastError(unsigned n){err=n;}
unsigned GetLastError(void){return err;}
int SetThreadPriority(HANDLE h,int p){return 1;}
int QueryPerformanceCounter(LARGE_INTEGER*n){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);n->QuadPart=t.tv_sec*1000000LL+t.tv_nsec/1000;return 1;}
int QueryPerformanceFrequency(LARGE_INTEGER*n){n->QuadPart=1000000;return 1;}
struct thread_start{unsigned(*start)(void*);void*arg;};
void*run(void*p){struct thread_start*t=p;t->start(t->arg);return 0;}
HANDLE CreateThread(void*a,unsigned stack,unsigned(*f)(void*),void*p,unsigned flags,void*id){pthread_t*t=malloc(sizeof(*t));struct thread_start*s=malloc(sizeof(*s));s->start=f;s->arg=p;assert(!pthread_create(t,0,run,s));return t;}
HANDLE CreateFile(const char*n,unsigned a,unsigned share,void*s,unsigned create,unsigned flags,void*t){int fd=open(n,O_RDONLY);return fd<0?INVALID_HANDLE_VALUE:(void*)(intptr_t)(fd+1);}
unsigned GetFileSize(HANDLE h,void*p){off_t cur=lseek((intptr_t)h-1,0,SEEK_CUR),end=lseek((intptr_t)h-1,0,SEEK_END);lseek((intptr_t)h-1,cur,SEEK_SET);return end;}
int CloseHandle(HANDLE h){return close((intptr_t)h-1)==0;}
void XPhysicalProtect(void*p,unsigned n,unsigned pr){}
void print_status(const char *s){puts(s);}
int cache_file_header_verify(const void*h,const char*s,int fatal){const uint32_t*p=h;return p[0]==0x68656164&&p[1]==5&&p[511]==0x666f6f74;}
void cache_copy_set_priority(boolean);
static void callback_apc(void*f,void*o,void*x){OVERLAPPED*r=o;((void(*)(unsigned,unsigned,OVERLAPPED*))f)(r->Internal,r->InternalHigh,r);}
int ReadFileEx(HANDLE h,void*b,unsigned n,OVERLAPPED*r,void(*cb)(unsigned,unsigned,OVERLAPPED*)){ssize_t v=pread((intptr_t)h-1,b,n,r->Offset);r->Internal=v<0?6:0;r->InternalHigh=v<0?0:v;platform_queue_apc(callback_apc,cb,r,0);return 1;}
int WriteFileEx(HANDLE h,const void*b,unsigned n,OVERLAPPED*r,void(*cb)(unsigned,unsigned,OVERLAPPED*)){ssize_t v=pwrite((intptr_t)h-1,b,n,r->Offset);r->Internal=v<0?5:0;r->InternalHigh=v<0?0:v;platform_queue_apc(callback_apc,cb,r,0);return 1;}
