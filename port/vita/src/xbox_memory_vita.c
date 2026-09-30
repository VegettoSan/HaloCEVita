/* Native ARM XDK memory implementation, preserving the Linux allocator's
 * placed-offset/top-down policy inside a movable 96MiB Vita arena. */
#include "platform.h"
#include "halo_vita_memory.h"
#include "vita_runtime.h"
#include "cache/physical_memory_map.h"
#include "saved games/game_state.h"
#include <string.h>

#define PAGE_SIZE_BYTES 4096u
#define PAGE_COUNT (HALO_VITA_ARENA_SIZE / PAGE_SIZE_BYTES)
static DWORD protection[PAGE_COUNT];
static unsigned long lengths[PAGE_COUNT];
static pthread_mutex_t memory_lock = PTHREAD_MUTEX_INITIALIZER;
static int memory_live;
static int state_live;

BOOL platform_is_contiguous(const void *address)
{
	uintptr_t base = halo_vita_memory_base(), value = (uintptr_t)address;
	return base && value >= base && value - base < HALO_VITA_ARENA_SIZE;
}
static int pages_free(unsigned long first, unsigned long count)
{
	unsigned long i;
	if (first >= PAGE_COUNT || count > PAGE_COUNT - first) return 0;
	for (i = first; i < first + count; ++i) if (protection[i]) return 0;
	return 1;
}
void *platform_contiguous_alloc(unsigned long size, unsigned long alignment,
	unsigned long requested, DWORD protect)
{
	unsigned long first = PAGE_COUNT, count, step, i, candidate;
	void *address = NULL;
	if (!halo_vita_memory_base() || !size || size > HALO_VITA_ARENA_SIZE ||
		(alignment && (alignment & (alignment - 1)))) return NULL;
	count = (size + PAGE_SIZE_BYTES - 1) / PAGE_SIZE_BYTES;
	step = alignment > PAGE_SIZE_BYTES ? alignment / PAGE_SIZE_BYTES : 1;
	protect &= ~(PAGE_WRITECOMBINE | PAGE_NOCACHE);
	/* USER_RW is not executable and cannot emulate per-page write faults. */
	if (protect != PAGE_READWRITE) {
		vita_log("XPhysicalAlloc unsupported protection=0x%08lx", (unsigned long)protect);
		return NULL;
	}
	pthread_mutex_lock(&memory_lock);
	if (requested != PLATFORM_ANY_PHYSICAL_ADDRESS) {
		if (!(requested & (PAGE_SIZE_BYTES - 1)) && !((halo_vita_memory_base() + requested) % (step * PAGE_SIZE_BYTES)) &&
			pages_free(requested / PAGE_SIZE_BYTES, count)) first = requested / PAGE_SIZE_BYTES;
	} else if (count <= PAGE_COUNT) {
		candidate = PAGE_COUNT - count;
		for (;;) {
			unsigned long remainder = (candidate + halo_vita_memory_base() / PAGE_SIZE_BYTES) % step;
			if (remainder > candidate) break;
			candidate -= remainder;
			if (pages_free(candidate, count)) { first = candidate; break; }
			if (!candidate) break;
			--candidate;
		}
	}
	if (first != PAGE_COUNT) {
		address = (void *)(halo_vita_memory_base() + first * PAGE_SIZE_BYTES);
		memset(address, 0, count * PAGE_SIZE_BYTES);
		for (i = first; i < first + count; ++i) protection[i] = protect;
		lengths[first] = count;
	}
	pthread_mutex_unlock(&memory_lock);
	vita_log("XPhysicalAlloc size=%lu offset=%08lx align=%lu -> %p", size, requested, alignment, address);
	return address;
}
void platform_contiguous_free(void *address)
{
	unsigned long first, count, i;
	if (!platform_is_contiguous(address) || ((uintptr_t)address - halo_vita_memory_base()) % PAGE_SIZE_BYTES) return;
	first = ((uintptr_t)address - halo_vita_memory_base()) / PAGE_SIZE_BYTES;
	pthread_mutex_lock(&memory_lock);
	count = lengths[first];
	for (i = first; i < first + count; ++i) protection[i] = 0;
	lengths[first] = 0;
	pthread_mutex_unlock(&memory_lock);
}
LPVOID WINAPI XPhysicalAlloc(SIZE_T size, ULONG_PTR requested, ULONG_PTR alignment, DWORD protect)
{
	return platform_contiguous_alloc(size, alignment, requested, protect);
}
VOID WINAPI XPhysicalFree(LPVOID address) { platform_contiguous_free(address); }
DWORD WINAPI XQueryMemoryProtect(LPVOID address)
{
	DWORD result = PAGE_NOACCESS;
	if (platform_is_contiguous(address)) {
		pthread_mutex_lock(&memory_lock);
		result = protection[((uintptr_t)address - halo_vita_memory_base()) / PAGE_SIZE_BYTES];
		if (!result) result = PAGE_NOACCESS;
		pthread_mutex_unlock(&memory_lock);
	}
	return result;
}
BOOL WINAPI VirtualProtect(LPVOID address, SIZE_T size, DWORD protect, PDWORD previous)
{
	unsigned long first, last, i;
	protect &= ~(PAGE_WRITECOMBINE | PAGE_NOCACHE);
	/* Report a real failure rather than claim that READONLY/NOACCESS is
	 * enforced. The renderer's Linux SIGSEGV write tracker needs adaptation. */
	if (previous) *previous = XQueryMemoryProtect(address);
	if (protect == PAGE_READWRITE && size && platform_is_contiguous(address) &&
		size <= HALO_VITA_ARENA_SIZE - ((uintptr_t)address - halo_vita_memory_base())) {
		first = ((uintptr_t)address - halo_vita_memory_base()) / PAGE_SIZE_BYTES;
		last = ((uintptr_t)address - halo_vita_memory_base() + size - 1) / PAGE_SIZE_BYTES;
		pthread_mutex_lock(&memory_lock);
		for (i = first; i <= last && protection[i] == PAGE_READWRITE; ++i) {}
		pthread_mutex_unlock(&memory_lock);
		if (i > last) return TRUE;
	}
	vita_log("VirtualProtect BLOCKED address=%p size=%u protection=%08lx", address, (unsigned)size,
		(unsigned long)protect);
	return FALSE;
}
VOID WINAPI XPhysicalProtect(LPVOID address, SIZE_T size, DWORD protect)
{
	if (!VirtualProtect(address, size, protect, NULL)) vita_log("XPhysicalProtect requested policy not enforced");
}
int halo_vita_memory_initialize(void)
{
	void *extra;
	if (memory_live) return 1;
	physical_memory_allocate();
	memory_live = 1;
	physical_memory_verify();

	/* Let the original game-state initializer own its allocation exactly once.
	 * It binds the already placed physical-memory region, creates the backing
	 * save file through the Vita XAPI bridge and allocates the real header. */
	vita_log("[VITA 033] original game-state initialization begin");
	game_state_initialize();
	state_live = 1;
	vita_log("[VITA 034] original game-state initialized on placed arena");

	vita_log("Halo physical_memory_allocate/verify returned: game=%p tags=%p texture=%p sound=%p",
		physical_memory_get_game_state_base_address(), physical_memory_get_tag_cache_base_address(),
		physical_memory_get_texture_cache_base_address(), physical_memory_get_sound_cache_base_address());
	extra = XPhysicalAlloc(8192, PLATFORM_ANY_PHYSICAL_ADDRESS, 16384, PAGE_READWRITE);
	if (!extra || ((uintptr_t)extra - halo_vita_memory_base()) % 16384) return 0;
	memset(extra, 0x5a, 8192); XPhysicalFree(extra);
	if (XQueryMemoryProtect(extra) != PAGE_NOACCESS) return 0;
	vita_log("[VITA 014] Halo physical memory PASS; protection query tracks allocations, no page-fault enforcement");
	return 1;
}
void halo_vita_memory_dispose(void)
{
	if (state_live) game_state_dispose();
	state_live = 0;
	if (memory_live) physical_memory_free();
	memory_live = 0;
	memset(protection, 0, sizeof(protection)); memset(lengths, 0, sizeof(lengths));
}
