/* SDK ABI boundary: the arena owns one kernel allocation, outside newlib. */
#include "vita_runtime.h"
#include "halo_vita_memory.h"
#include <psp2/kernel/sysmem.h>
#include <string.h>

static SceUID arena_uid = -1;
static void *arena_base;

uintptr_t halo_vita_memory_base(void) { return (uintptr_t)arena_base; }
uintptr_t halo_vita_memory_address(uintptr_t xbox_address)
{
	if (!arena_base || xbox_address < HALO_XBOX_MEMORY_BASE ||
		xbox_address - HALO_XBOX_MEMORY_BASE >= HALO_VITA_ARENA_SIZE) return 0;
	return (uintptr_t)arena_base + xbox_address - HALO_XBOX_MEMORY_BASE;
}
int vita_memory_initialize(void)
{
	int result;
	SceKernelAllocMemBlockOpt options;
	if (arena_base) return 1;
	vita_log("[VITA 013] native Xbox arena begin: 96MiB USER_RW; heap64MiB; GL RAM16MiB/CDRAM24MiB");
	memset(&options, 0, sizeof(options)); options.size = sizeof(options);
	options.attr = SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_ALIGNMENT; options.alignment = 65536;
	arena_uid = sceKernelAllocMemBlock("HaloXboxArena", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW,
		HALO_VITA_ARENA_SIZE, &options);
	if (arena_uid < 0) {
		vita_log("Xbox arena allocation failed: 0x%08x", (unsigned)arena_uid);
		return 0;
	}
	result = sceKernelGetMemBlockBase(arena_uid, &arena_base);
	if (result < 0 || !arena_base) {
		vita_log("Xbox arena base failed: 0x%08x", (unsigned)result);
		sceKernelFreeMemBlock(arena_uid); arena_uid = -1; arena_base = NULL;
		return 0;
	}
	vita_log("Xbox arena base=%p size=%u; original VA rebased; no fixed mapping/Android loader", arena_base,
		HALO_VITA_ARENA_SIZE);
	return 1;
}
void vita_memory_shutdown(void)
{
	if (arena_uid >= 0) {
		int result = sceKernelFreeMemBlock(arena_uid);
		vita_log("Xbox arena release result=0x%08x", (unsigned)result);
	}
	arena_base = NULL; arena_uid = -1;
}
