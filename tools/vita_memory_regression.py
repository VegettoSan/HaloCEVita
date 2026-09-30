#!/usr/bin/env python3
"""Run actual Vita/XDK allocator and upstream memory functions with SDK mocks.

Host execution verifies allocation policy and bookkeeping, not ARM layout,
kernel availability, GPU accessibility or hardware page protection.
"""
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build/vita/tests/memory-a017'

PLATFORM = r'''
#ifndef MOCK_PLATFORM_H
#define MOCK_PLATFORM_H
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include "halo_vita_memory.h"
#include "halo_port_capacity.h"
#define HALO_VITA 1
#define HALO_LINUX 1
#define WINAPI
#define TRUE 1
#define FALSE 0
typedef int BOOL;
typedef uint32_t DWORD;
typedef DWORD *PDWORD;
typedef void *LPVOID;
typedef size_t SIZE_T;
/* XDK offsets remain 32-bit even though this host's pointers are 64-bit. */
typedef uint32_t ULONG_PTR;
#define VOID void
#define PAGE_NOACCESS 1
#define PAGE_READONLY 2
#define PAGE_READWRITE 4
#define PAGE_NOCACHE 0x200
#define PAGE_WRITECOMBINE 0x400
#define PLATFORM_ANY_PHYSICAL_ADDRESS 0xffffffffUL
void *platform_contiguous_alloc(unsigned long,unsigned long,unsigned long,DWORD);
void platform_contiguous_free(void *);
BOOL platform_is_contiguous(const void *);
LPVOID XPhysicalAlloc(SIZE_T,ULONG_PTR,ULONG_PTR,DWORD);
VOID XPhysicalFree(LPVOID);
DWORD XQueryMemoryProtect(LPVOID);
BOOL VirtualProtect(LPVOID,SIZE_T,DWORD,PDWORD);
VOID XPhysicalProtect(LPVOID,SIZE_T,DWORD);
#endif
'''

KERNEL = r'''
#include <stddef.h>
#include <stdint.h>
typedef int SceUID;
typedef struct { unsigned size, attr, alignment; } SceKernelAllocMemBlockOpt;
#define SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_ALIGNMENT 4
#define SCE_KERNEL_MEMBLOCK_TYPE_USER_RW 1
SceUID sceKernelAllocMemBlock(const char *,int,unsigned,SceKernelAllocMemBlockOpt *);
int sceKernelGetMemBlockBase(SceUID,void **);
int sceKernelFreeMemBlock(SceUID);
'''

DRIVER = r'''
#define _POSIX_C_SOURCE 200112L
#include "platform.h"
#include "cache/physical_memory_map.h"
#include "psp2/kernel/sysmem.h"
#include "vita_runtime.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
static void *allocation;
static int allocs, frees;
void vita_log(const char *format, ...) { (void)format; }
SceUID sceKernelAllocMemBlock(const char *name,int type,unsigned size,SceKernelAllocMemBlockOpt *opt)
{
 (void)name; assert(type == SCE_KERNEL_MEMBLOCK_TYPE_USER_RW);
 assert(size == HALO_VITA_ARENA_SIZE && opt->alignment == 65536 && opt->attr == 4);
 assert(!allocation); assert(!posix_memalign(&allocation,opt->alignment,size)); ++allocs; return 42;
}
int sceKernelGetMemBlockBase(SceUID uid,void **base) { assert(uid == 42); *base=allocation; return 0; }
int sceKernelFreeMemBlock(SceUID uid) { assert(uid == 42); free(allocation); allocation=NULL; ++frees; return 0; }
int main(void)
{
 uintptr_t base; void *p,*q; DWORD old; int i;
 assert(!halo_vita_memory_address(0x803A6000u));
 assert(vita_memory_initialize() && vita_memory_initialize()); assert(allocs == 1);
 base=halo_vita_memory_base(); assert(!(base % 65536));
 assert(!halo_vita_memory_address(0x7fffffffu) && !halo_vita_memory_address(0x86000000u));
 for(i=0;i<2;++i) {
  assert(halo_vita_memory_initialize());
  assert((uintptr_t)physical_memory_get_game_state_base_address() == base+0x1a00000u);
  assert((uintptr_t)physical_memory_get_tag_cache_base_address() == base+0x3a6000u);
  assert(XQueryMemoryProtect(physical_memory_get_game_state_base_address()) == PAGE_READWRITE);
  assert(!XPhysicalAlloc(4096,0x1a00000,0,PAGE_READWRITE));
  assert(!XPhysicalAlloc(0,PLATFORM_ANY_PHYSICAL_ADDRESS,0,PAGE_READWRITE));
  assert(!XPhysicalAlloc(HALO_VITA_ARENA_SIZE+1,PLATFORM_ANY_PHYSICAL_ADDRESS,0,PAGE_READWRITE));
  assert(!XPhysicalAlloc(4096,1,0,PAGE_READWRITE));
  assert(!XPhysicalAlloc(4096,PLATFORM_ANY_PHYSICAL_ADDRESS,3,PAGE_READWRITE));
  assert(!XPhysicalAlloc(4096,PLATFORM_ANY_PHYSICAL_ADDRESS,0,PAGE_READONLY));
  p=XPhysicalAlloc(4096,4096,4096,PAGE_READWRITE); assert((uintptr_t)p == base+4096);
  memset(p,0xab,4096); assert(!XPhysicalAlloc(4096,4096,4096,PAGE_READWRITE));
  assert(VirtualProtect(p,4096,PAGE_READWRITE,&old) && old == PAGE_READWRITE);
  assert(!VirtualProtect(p,4097,PAGE_READWRITE,&old));
  assert(!VirtualProtect(p,4096,PAGE_READONLY,&old));
  assert(!VirtualProtect((void*)base,4096,PAGE_READWRITE,&old));
  XPhysicalFree(p); assert(XQueryMemoryProtect(p) == PAGE_NOACCESS);
  q=XPhysicalAlloc(4096,4096,4096,PAGE_READWRITE); assert(q == p && ((unsigned char*)q)[0] == 0);
  XPhysicalFree(q);
  q=XPhysicalAlloc(8192,PLATFORM_ANY_PHYSICAL_ADDRESS,131072,PAGE_READWRITE);
  assert(q && !((uintptr_t)q % 131072)); XPhysicalFree(q);
  halo_vita_memory_dispose();
  assert(XQueryMemoryProtect((void*)(base+0x1a00000u)) == PAGE_NOACCESS);
 }
 vita_memory_shutdown(); vita_memory_shutdown();
 assert(allocs == 1 && frees == 1 && !halo_vita_memory_base());
 puts("PASS: actual allocator + upstream physical memory/game-state functions; 2 lifecycle cycles, overlap/alignment/bounds/zero/reuse/protection checks");
 return 0;
}
'''


def main():
    os.chdir(ROOT)
    (BUILD / 'psp2/kernel').mkdir(parents=True, exist_ok=True)
    (BUILD / 'cache').mkdir(exist_ok=True)
    (BUILD / 'saved games').mkdir(exist_ok=True)
    (BUILD / 'platform.h').write_text(PLATFORM)
    (BUILD / 'psp2/kernel/sysmem.h').write_text(KERNEL)
    (BUILD / 'cseries.h').write_text('#include "platform.h"\n#include <assert.h>\ntypedef unsigned char byte;\n#define match_assert(f,l,c) assert(c)\n')
    (BUILD / 'cseries_windows.h').write_text('')
    (BUILD / 'cache/physical_memory_map.h').write_bytes((ROOT / 'source/cache/physical_memory_map.h').read_bytes())
    (BUILD / 'saved games/game_state.h').write_text('void *game_state_allocate_buffer(unsigned long,unsigned long,unsigned long);\nvoid game_state_free_buffer(void);\n')
    source = (ROOT / 'source/saved games/game_state_xbox.c').read_text()
    functions = source[source.index('void *game_state_allocate_buffer('):source.index('void game_state_create_or_open_file(')]
    state = '#include "cseries.h"\n#include "cache/physical_memory_map.h"\n#define CPU_PAGE_SIZE 4096\n'
    state += 'struct { int buffer_allocated; void *buffer; unsigned long buffer_size; } xbox_game_state_globals;\n'
    (BUILD / 'state.c').write_text(state + functions)
    (BUILD / 'driver.c').write_text(DRIVER)
    command = ['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-DHALO_VITA', '-DHALO_LINUX',
               '-I' + str(BUILD), '-Iport/vita/include', '-Iport/linux/include',
               str(BUILD / 'driver.c'), str(BUILD / 'state.c'),
               'port/vita/src/vita_memory.c', 'port/vita/src/xbox_memory_vita.c',
               'source/cache/physical_memory_map.c', '-pthread', '-o', str(BUILD / 'memory-test')]
    subprocess.run(command, check=True)
    subprocess.run([str(BUILD / 'memory-test')], check=True)


if __name__ == '__main__':
    main()
