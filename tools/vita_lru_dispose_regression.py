#!/usr/bin/env python3
"""Execute original combined LRU allocation/init/disposal ownership."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
s=(root/'source/memory/lruv_cache.c').read_text()
def body(name,end):return s[s.index(name):s.index(end,s.index(name))]
funcs=body('long lruv_allocation_size(', 'void lruv_update_function_pointers(')+body('void lruv_initialize(', 'void lruv_idle(')+body('struct lruv_cache *lruv_new(', 'void lruv_flush(')
code=r'''#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define HALO_VITA 1
#define TRUE 1
#define NONE -1
#define SHORT_BITS 16
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#define LRUV_CACHE_SIGNATURE 1234
#define match_assert(f,l,c) assert(c)
#define csmemset memset
#define csstrncpy strncpy
typedef int boolean;
typedef void (*lruv_delete_block_proc)(long);
typedef boolean (*lruv_locked_block_proc)(long);
struct data_array {char name[32];short maximum_count,size;int valid;};
struct lruv_cache_block {long next,previous,pages;};
struct lruv_cache {char name[32];lruv_delete_block_proc delete_block_proc;lruv_locked_block_proc locked_block_proc;long page_count,page_size_bits,tick,first_block_index,last_block_index;struct data_array *blocks;unsigned long signature;};
static void *owned;static unsigned allocations,frees,interior_frees;
static void *match_malloc(const char*f,int l,long n){(void)f;(void)l;assert(!owned);owned=calloc(1,n);allocations++;return owned;}
static void match_free(const char*f,int l,void*p){(void)f;(void)l;assert(p==owned);frees++;free(p);owned=NULL;}
static long data_allocation_size(short n,short size){return sizeof(struct data_array)+n*size;}
static void data_initialize(struct data_array*d,const char*n,short max,short size){memset(d,0,sizeof(*d));strncpy(d->name,n,31);d->maximum_count=max;d->size=size;}
static void data_make_valid(struct data_array*d){d->valid=1;}
static void data_verify(struct data_array*d){assert(owned && d==(struct data_array*)((struct lruv_cache*)owned+1));assert(d->maximum_count==512 && d->valid);}
static void data_dispose(struct data_array*d){assert((void*)d!=owned);interior_frees++; /* historical invalid free is observed, not executed */}
static void lruv_cache_verify(struct lruv_cache*c,boolean b){assert(c==owned && c->signature==1234 && b);data_verify(c->blocks);}
'''+funcs+r'''
int main(void){for(int i=0;i<2;i++){struct lruv_cache*c=lruv_new("xbox sound",1024,12,512,NULL,NULL);assert(c && c->blocks==(struct data_array*)(c+1));lruv_delete(c);assert(!owned);}assert(allocations==2 && frees==2);
#ifdef EXPECT_HISTORICAL
assert(interior_frees==2);puts("PASS historical LRU delete attempts embedded-array free");
#else
assert(!interior_frees);puts("PASS actual LRU allocation/init/delete: one owned free, embedded header checked, two cycles");
#endif
}
'''
out=root/'build/vita/tests/lru-dispose';out.mkdir(parents=True,exist_ok=True)
for mode in ('fixed','historical'):
 c=code if mode=='fixed' else code.replace('#define HALO_VITA 1','#define EXPECT_HISTORICAL 1')
 p=out/(mode+'.c');p.write_text(c)
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wno-unused-function','-Werror',str(p),'-o',str(out/mode)],check=True)
 subprocess.run([str(out/mode)],check=True)
