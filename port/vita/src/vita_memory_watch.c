/* Vita does not use Linux's SIGSEGV/mprotect guest-page write tracker.
 * xbox_textures.c deliberately refreshes guest-backed textures on every use
 * under HALO_VITA, so these hooks preserve the platform contract without
 * pretending Vita has page-generation tracking. */
#include "platform.h"

void memory_watch_initialize(void)
{
}

void memory_watch_protect(unsigned long address, unsigned long size)
{
    (void)address;
    (void)size;
}

unsigned long memory_watch_generation(unsigned long address, unsigned long size)
{
    (void)address;
    (void)size;
    return 0;
}

unsigned long memory_watch_serial(void)
{
    return 0;
}

void memory_watch_prepare_write(void *address, unsigned long size)
{
    (void)address;
    (void)size;
}

void memory_watch_forget(void *address, unsigned long size)
{
    (void)address;
    (void)size;
}
