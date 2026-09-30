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
int halo_vita_cache_index_probe(void *tags, size_t length);
int vita_cache_probe(int graphics, int core, int shaders);
#endif
