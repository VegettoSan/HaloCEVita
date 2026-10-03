/* Original streaming decompressor; CPU scratch pages never reach the GPU.
 * Native Ex I/O copies synchronously and completes on issuing-thread APCs.
 * Replace only the unavailable scratch-page protection boundary, not the copy
 * algorithm, sequencing, zlib decoder, slot owner or publication protocol. */
#include "cseries/cseries.h"
#include <xtl.h>
#include "vita_runtime.h"
static VOID WINAPI vita_cache_scratch_protect(LPVOID address, SIZE_T bytes, DWORD policy);
#define XPhysicalProtect vita_cache_scratch_protect
#include "../../../source/cache/cache_files_decompress_windows.c"
#undef XPhysicalProtect

static VOID WINAPI vita_cache_scratch_protect(LPVOID address, SIZE_T bytes, DWORD policy)
{
    uintptr_t base = (uintptr_t)global_self->allocated_buffer;
    uintptr_t pointer = (uintptr_t)address;
    if (!base || pointer < base || pointer - base > TOTAL_BUFFER_SIZE ||
        bytes > TOTAL_BUFFER_SIZE - (pointer - base) ||
        (policy != PAGE_READONLY && policy != PAGE_READWRITE))
        vita_fatal("original decompressor scratch request outside its owned buffer");
    /* No fake VirtualProtect result: this void boundary validates ownership.
     * The issuing worker cannot reuse scratch before its Ex copy has finished. */
}
