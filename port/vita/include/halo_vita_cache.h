#ifndef HALO_VITA_CACHE_H
#define HALO_VITA_CACHE_H
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

typedef int (*vita_cache_progress)(uint32_t logical_bytes, void *context);
struct vita_cache_info {
	uint32_t logical_size, tag_offset, tag_size;
	uint32_t tag_count, scenario_index, vertices, indices;
	uint32_t tag_crc;
	int compressed;
};
/* Read-only streaming: no full-map allocation, conversion or cache file. */
int vita_cache_read(FILE *file, void *tags, size_t capacity, struct vita_cache_info *info,
	vita_cache_progress progress, void *context, char *error, size_t error_size);
int vita_cache_validate_index(const void *tags, size_t length, struct vita_cache_info *info,
	char *error, size_t error_size);
struct vita_menu_relocation;
struct vita_menu_stats {
    uint32_t widgets, fonts, string_lists, bitmap_groups, pointers;
    uint32_t blocks, references, bitmap_count, volume_count;
    uint32_t menu_index, menu_widgets;
    uint32_t block_addresses, data_addresses, reference_names;
};
/* Plan and validate all known fields first; apply only after complete success.
 * Scope: widget-reachable layouts, strings/fonts/bitmap metadata, sky/BSP refs.
 * No BSP payload/GPU descriptors or other tag groups are activated. */
struct vita_menu_relocation *vita_cache_relocate_menu(void *tags, size_t length,
    uint32_t native_base, struct vita_menu_stats *stats, char *error, size_t error_size);
void vita_cache_restore_menu(struct vita_menu_relocation *plan);
int halo_vita_menu_tags_probe(uint32_t menu_index, const struct vita_menu_stats *stats);
int halo_vita_cache_index_probe(void *tags, size_t length);
int vita_cache_probe(int graphics, int core, int shaders);
#endif
