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
/* Checked Xbox-v5 reader. It can inspect either a compressed source map or an
 * already prepared logical cache without changing serialized tag/resource
 * bytes. */
int vita_cache_read(FILE *file, void *tags, size_t capacity, struct vita_cache_info *info,
	vita_cache_progress progress, void *context, char *error, size_t error_size);
int vita_cache_validate_index(const void *tags, size_t length, struct vita_cache_info *info,
	char *error, size_t error_size);
int vita_cache_read_logical_range(FILE *file, uint32_t expected_logical_size,
	uint32_t logical_offset, void *destination, size_t bytes,
	vita_cache_progress progress, void *context, char *error, size_t error_size);

/* Retail Halo precaches d:\\maps\\<name>.map into persistent z:\\cacheNNN.map
 * slots before cache_file_open. Vita preserves that ownership split with
 * ux0:data/HaloCE/cache000.map ... cache005.map. A slot is published only after
 * the complete payload has been copied/inflated and validated; the real 0x800
 * source header is written last, matching cache_files_decompress_windows.c's
 * commit protocol. The source map is never modified. */
int vita_cache_slot_valid(const char *source_path, unsigned slot_index,
	char *slot_path, size_t slot_path_size, char *error, size_t error_size);
int vita_cache_prepare_slot(const char *source_path, unsigned slot_index,
	char *slot_path, size_t slot_path_size, int *reused,
	vita_cache_progress progress, void *context, char *error, size_t error_size);

/* Bind cache_file_read to an already prepared, uncompressed logical cache
 * slot. Binding a compressed source map is deliberately rejected: decompression
 * belongs to the precache phase, never cache_file_open/resource reads. */
int vita_cache_resource_bind(const char *path, uint32_t logical_size);
const char *vita_cache_resource_error(void);
void vita_cache_resource_unbind(void);
/* Validate a compiled bitmap's absolute logical range without reading pixels. */
int vita_cache_resource_range_valid(uint32_t logical_offset, size_t bytes);
int vita_cache_resource_read(uint32_t logical_offset, void *destination, size_t bytes,
	char *error, size_t error_size);
/* Nonzero only while the original cache_file_open/scenario_tags_load lifetime
 * owns a Vita cache slot. The manual ui.map bring-up mount intentionally does
 * not claim this lifetime. */
size_t halo_vita_cache_direct_tag_size(void);
int halo_vita_cache_original_range_valid(uint32_t offset, size_t bytes);
void halo_vita_cache_activate_original_tags(void *buffer, long size);
void halo_vita_cache_activate_original_bsp(void *buffer, long size);
/* Original cache tag blocks/data/references retain serialized Xbox addresses
 * below their already-relocated roots. This resolver is used only by original
 * tag accessors: runtime/native owners pass through unchanged, while owners in
 * an activated tag/BSP image resolve against its validated Xbox->Vita span. */
void *halo_vita_cache_resolve_compiled_pointer(const void *owner,
	const void *serialized_pointer, size_t bytes);
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
/* Mount a validated ui tag image for original Halo consumers. The caller must
 * unmount before replacing/freeing the image; unmount restores every pointer. */
int halo_vita_cache_mount_menu(void *tags, size_t length);
int halo_vita_cache_validate_menu(void);
int halo_vita_cache_unmount_menu(void);
int halo_vita_ui_runtime_initialize(void);
int halo_vita_ui_runtime_dispose(void);
int halo_vita_menu_root_checkpoint(void);
int halo_vita_menu_update_checkpoint(void);
int halo_vita_menu_render_checkpoint(void);
int vita_cache_probe(int graphics, int core, int shaders);
#endif