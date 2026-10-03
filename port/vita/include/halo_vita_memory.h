#ifndef HALO_VITA_MEMORY_H
#define HALO_VITA_MEMORY_H
#include <stdint.h>
#include <stddef.h>

/* Xbox physical addresses are offsets in this native arena, never Vita
 * physical addresses. Keep the original offsets, not the original VA. */
#define HALO_VITA_ARENA_SIZE (96u * 1024u * 1024u)
#define HALO_XBOX_MEMORY_BASE 0x80000000u
#define HALO_XBOX_TAG_BASE 0x803A6000u
#define HALO_VITA_TAG_CAPACITY 0x01600000u
uintptr_t halo_vita_memory_base(void);
uintptr_t halo_vita_memory_address(uintptr_t xbox_address);
int vita_memory_initialize(void);
void vita_memory_shutdown(void);
/* Prepare only Halo's original physical-memory map inside the placed Vita
 * arena. This is the entry needed before shell_initialize(), which remains the
 * owner of game_state_initialize() in an original-main-loop target. */
int halo_vita_memory_prepare_original_shell(void);
/* Current staged UI route keeps its historical combined physical-memory +
 * game-state initialization. It now reuses the physical-only phase above. */
int halo_vita_memory_initialize(void);
void halo_vita_memory_dispose(void);
#endif