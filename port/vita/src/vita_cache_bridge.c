/* Game-ABI side of the Vita cache resource boundary.
 *
 * Xbox cache_file_read() is asynchronous because the retail path reads a
 * sector-aligned uncompressed cache. Vita now prepares a seekable logical
 * resource stream at binding time, validating compressed data before menu/audio
 * activation. Requests synchronously seek/read only their exact bytes; they no
 * longer decompress every prefix. Completion is raised after the read, and the
 * original texture/sound caches still own their destinations and lifetimes.
 */
#include "cseries/cseries.h"
#include "cache/cache_files.h"
#include "bitmaps/bitmap_group.h"
#include "tag_files/tag_groups.h"
#include "scenario/scenario_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "halo_vita_cache.h"
#include "halo_vita_memory.h"
#include "vita_runtime.h"

#define VITA_SYNC_CACHE_REQUEST 0
#define VITA_SCENARIO_SIZE 1456u
#define VITA_SCENARIO_BSP_REFERENCE_SIZE 32u
#define VITA_SBSP_GROUP 0x73627370u
#define VITA_SBSP_HEADER_SIGNATURE 0x73627370u
#define VITA_MAX_SCENARIO_BSPS 64u

/* cache_files.c exports these but the January header does not declare them. */
char *tag_get_name(long tag_index);
unsigned long tag_get_group_tag(long tag_index);

struct vita_scenario_bsp_binding {
	uint32_t file_offset;
	uint32_t file_size;
	uint32_t xbox_base;
	uintptr_t native_base;
};

static unsigned char *vita_scenario_tags;
static size_t vita_scenario_tag_size;
static struct vita_scenario_bsp_binding vita_scenario_bsps[VITA_MAX_SCENARIO_BSPS];
static uint32_t vita_scenario_bsp_count;

static uint32_t vita_read_u32(const unsigned char *p)
{
	return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
		(uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void vita_write_u32(unsigned char *p, uint32_t value)
{
	p[0] = (unsigned char)value;
	p[1] = (unsigned char)(value >> 8);
	p[2] = (unsigned char)(value >> 16);
	p[3] = (unsigned char)(value >> 24);
}

static void *vita_rebase_tag_pointer(const void *raw_pointer, size_t bytes, const char *what)
{
	uintptr_t raw = (uintptr_t)raw_pointer;
	uint32_t offset;
	if (!raw)
		return NULL;
	if (!vita_scenario_tags || raw < HALO_XBOX_TAG_BASE) {
		vita_log("[VITA SCENARIO] tag pointer rejected: %s raw=%08lx", what, (unsigned long)raw);
		vita_fatal("scenario tag pointer is outside the Xbox tag image");
	}
	offset = (uint32_t)(raw - HALO_XBOX_TAG_BASE);
	if (offset > vita_scenario_tag_size || bytes > vita_scenario_tag_size - offset) {
		vita_log("[VITA SCENARIO] tag span rejected: %s raw=%08lx bytes=%lu tag_bytes=%lu",
			what, (unsigned long)raw, (unsigned long)bytes, (unsigned long)vita_scenario_tag_size);
		vita_fatal("scenario tag pointer span is outside the loaded tag image");
	}
	return vita_scenario_tags + offset;
}

static void vita_rebase_tag_reference(struct tag_reference *reference, const char *what)
{
	if (reference->name)
		reference->name = vita_rebase_tag_pointer(reference->name, 1, what);
}

static void vita_rebase_tag_block(struct tag_block *block, const char *what)
{
	if (block->count < 0 || block->definition) {
		vita_log("[VITA SCENARIO] tag block rejected: %s count=%ld definition=%p",
			what, block->count, block->definition);
		vita_fatal("scenario tag block metadata is not compiled-cache form");
	}
	if (block->address)
		block->address = vita_rebase_tag_pointer(block->address, 1, what);
	else if (block->count)
		vita_fatal("scenario non-empty tag block has null address");
}

static void vita_rebase_tag_data(struct tag_data *data, const char *what)
{
	size_t bytes;
	if (data->size < 0 || data->definition) {
		vita_log("[VITA SCENARIO] tag data rejected: %s size=%ld definition=%p",
			what, data->size, data->definition);
		vita_fatal("scenario tag data metadata is not compiled-cache form");
	}
	bytes = data->size > 0 ? (size_t)data->size : 1u;
	if (data->address)
		data->address = vita_rebase_tag_pointer(data->address, bytes, what);
	else if (data->size)
		vita_fatal("scenario non-empty tag data has null address");
}

static void vita_rebase_scenario_top_level(struct scenario *scenario)
{
	unsigned i;
#define RB_REF(field) vita_rebase_tag_reference(&scenario->field, #field)
#define RB_BLOCK(field) vita_rebase_tag_block(&scenario->field, #field)
#define RB_DATA(field) vita_rebase_tag_data(&scenario->field, #field)
	RB_REF(ugly_structure_bsp);
	RB_REF(unloved_globals);
	RB_REF(bad_sky);
	RB_BLOCK(sky_references);
	RB_BLOCK(scenario_references);
	RB_BLOCK(predicted_ui_resources);
	RB_BLOCK(functions);
	RB_DATA(editor_scenario_data);
	RB_BLOCK(comments);
	RB_BLOCK(object_names);
	RB_BLOCK(scenery);
	RB_BLOCK(scenery_palette);
	RB_BLOCK(bipeds);
	RB_BLOCK(biped_palette);
	RB_BLOCK(vehicles);
	RB_BLOCK(vehicle_palette);
	RB_BLOCK(equipment);
	RB_BLOCK(equipment_palette);
	RB_BLOCK(weapons);
	RB_BLOCK(weapon_palette);
	RB_BLOCK(device_groups);
	RB_BLOCK(machines);
	RB_BLOCK(machine_palette);
	RB_BLOCK(controls);
	RB_BLOCK(control_palette);
	RB_BLOCK(light_fixtures);
	RB_BLOCK(light_fixtures_palette);
	RB_BLOCK(sound_scenery);
	RB_BLOCK(sound_scenery_palette);
	for (i = 0; i < NUMBEROF(scenario->unused_blocks); ++i)
		vita_rebase_tag_block(&scenario->unused_blocks[i], "unused_blocks");
	RB_BLOCK(starting_profiles);
	RB_BLOCK(players);
	RB_BLOCK(trigger_volumes);
	RB_BLOCK(recorded_animations);
	RB_BLOCK(netgame_flags);
	RB_BLOCK(netgame_equipment);
	RB_BLOCK(scenario_starting_equipment);
	RB_BLOCK(bsp_switch_trigger_volumes);
	RB_BLOCK(decals);
	RB_BLOCK(decal_palette);
	RB_BLOCK(detail_object_collection_palette);
	RB_BLOCK(ai_actor_palette);
	RB_BLOCK(ai_encounters);
	RB_BLOCK(ai_command_lists);
	RB_BLOCK(ai_animation_references);
	RB_BLOCK(ai_script_references);
	RB_BLOCK(ai_recording_references);
	RB_BLOCK(ai_conversations);
	RB_DATA(hs_syntax_data);
	RB_DATA(hs_string_constants);
	RB_BLOCK(hs_scripts);
	RB_BLOCK(hs_globals);
	RB_BLOCK(hs_references);
	RB_BLOCK(hs_source_files);
	RB_BLOCK(cutscene_flags);
	RB_BLOCK(cutscene_camera_points);
	RB_BLOCK(cutscene_chapter_titles);
	RB_REF(custom_object_names);
	RB_REF(ingame_help_text);
	RB_REF(hud_messages);
	RB_BLOCK(structure_bsp_references);
#undef RB_REF
#undef RB_BLOCK
#undef RB_DATA
}

static void *vita_rebase_bsp_pointer(const struct vita_scenario_bsp_binding *binding,
	const void *raw_pointer, size_t bytes, const char *what)
{
	uintptr_t raw = (uintptr_t)raw_pointer;
	uint32_t offset;
	if (!raw)
		return NULL;
	if (raw < binding->xbox_base) {
		vita_log("[VITA BSP] pointer rejected: %s raw=%08lx bsp_base=%08lx",
			what, (unsigned long)raw, (unsigned long)binding->xbox_base);
		vita_fatal("BSP pointer is below its Xbox payload base");
	}
	offset = (uint32_t)(raw - binding->xbox_base);
	if (offset > binding->file_size || bytes > binding->file_size - offset) {
		vita_log("[VITA BSP] span rejected: %s raw=%08lx bytes=%lu payload=%lu",
			what, (unsigned long)raw, (unsigned long)bytes, (unsigned long)binding->file_size);
		vita_fatal("BSP pointer span is outside the loaded payload");
	}
	return (void *)(binding->native_base + offset);
}

static void vita_rebase_bsp_block(const struct vita_scenario_bsp_binding *binding,
	struct tag_block *block, const char *what)
{
	if (block->count < 0 || block->definition) {
		vita_log("[VITA BSP] tag block rejected: %s count=%ld definition=%p",
			what, block->count, block->definition);
		vita_fatal("BSP tag block metadata is not compiled-cache form");
	}
	if (block->address)
		block->address = vita_rebase_bsp_pointer(binding, block->address, 1, what);
	else if (block->count)
		vita_fatal("BSP non-empty tag block has null address");
}

static void vita_rebase_bsp_data(const struct vita_scenario_bsp_binding *binding,
	struct tag_data *data, const char *what)
{
	size_t bytes;
	if (data->size < 0 || data->definition) {
		vita_log("[VITA BSP] tag data rejected: %s size=%ld definition=%p",
			what, data->size, data->definition);
		vita_fatal("BSP tag data metadata is not compiled-cache form");
	}
	bytes = data->size > 0 ? (size_t)data->size : 1u;
	if (data->address)
		data->address = vita_rebase_bsp_pointer(binding, data->address, bytes, what);
	else if (data->size)
		vita_fatal("BSP non-empty tag data has null address");
}

static void vita_rebase_structure_bsp_top_level(const struct vita_scenario_bsp_binding *binding,
	struct structure_bsp *bsp)
{
#define RB_BSP_BLOCK(field) vita_rebase_bsp_block(binding, &bsp->field, #field)
#define RB_BSP_DATA(field) vita_rebase_bsp_data(binding, &bsp->field, #field)
	vita_rebase_tag_reference(&bsp->lightmap_group, "structure_bsp.lightmap_group");
	RB_BSP_BLOCK(collision_materials);
	RB_BSP_BLOCK(collision_bsp);
	RB_BSP_BLOCK(nodes);
	RB_BSP_BLOCK(leaves);
	RB_BSP_BLOCK(surface_references);
	RB_BSP_BLOCK(surfaces);
	RB_BSP_BLOCK(lightmaps);
	RB_BSP_BLOCK(lens_flares);
	RB_BSP_BLOCK(lens_flare_markers);
	RB_BSP_BLOCK(clusters);
	RB_BSP_DATA(cluster_data);
	RB_BSP_BLOCK(cluster_portals);
	RB_BSP_BLOCK(breakable_surfaces);
	RB_BSP_BLOCK(fog_planes);
	RB_BSP_BLOCK(fog_regions);
	RB_BSP_BLOCK(fog_palette);
	RB_BSP_BLOCK(weather_palette);
	RB_BSP_BLOCK(weather_polyhedra);
	RB_BSP_BLOCK(pathfinding_surfaces);
	RB_BSP_BLOCK(pathfinding_edges);
	RB_BSP_BLOCK(background_sound_palette);
	RB_BSP_BLOCK(sound_environment_palette);
	RB_BSP_DATA(sound_cluster_data);
	RB_BSP_BLOCK(markers);
	RB_BSP_BLOCK(detail_object_data);
	RB_BSP_BLOCK(runtime_decals);
	/* leaf_map.bsp is runtime state, not a compiled-cache pointer. Leave it for
	 * leaf_map_initialize_from_bsp; only its compiled blocks are relocated. */
	vita_rebase_bsp_block(binding, &bsp->leaf_map.leaves, "leaf_map.leaves");
	vita_rebase_bsp_block(binding, &bsp->leaf_map.portals, "leaf_map.portals");
#undef RB_BSP_BLOCK
#undef RB_BSP_DATA
}

static void vita_activate_original_scenario_tag_image(void *buffer, long size)
{
	struct vita_cache_info info;
	unsigned char *tags = buffer;
	uint32_t table_address, table_offset, scenario_entry, scenario_root_address;
	uint32_t i;
	struct scenario *scenario;
	char error[160] = {0};

	if (buffer != (void *)halo_vita_memory_address(HALO_XBOX_TAG_BASE) || size <= 0)
		return;
	if (!vita_cache_validate_index(buffer, (size_t)size, &info, error, sizeof(error))) {
		vita_log("[VITA SCENARIO] original tag-image activation FAIL: %s", error);
		vita_fatal("original scenario tag image failed Vita index validation");
	}
	vita_scenario_tags = tags;
	vita_scenario_tag_size = (size_t)size;
	vita_scenario_bsp_count = 0;
	memset(vita_scenario_bsps, 0, sizeof(vita_scenario_bsps));

	table_address = vita_read_u32(tags);
	table_offset = table_address - HALO_XBOX_TAG_BASE;
	scenario_entry = table_offset + (info.scenario_index & 0xFFFFu) * 32u;
	if (scenario_entry > (uint32_t)size || 32u > (uint32_t)size - scenario_entry)
		vita_fatal("original scenario directory entry outside tag image");
	scenario_root_address = vita_read_u32(tags + scenario_entry + 20u);

	/* Commit the compiled tag header/directory only after the complete raw index
	 * was validated. All rebased values remain inside this process-owned arena. */
	vita_write_u32(tags, (uint32_t)(uintptr_t)vita_rebase_tag_pointer((void *)(uintptr_t)table_address,
		info.tag_count * 32u, "tag_header.tag_instances"));
	if (info.vertices)
		vita_write_u32(tags + 20u, (uint32_t)(uintptr_t)vita_rebase_tag_pointer(
			(void *)(uintptr_t)vita_read_u32(tags + 20u), info.vertices * 12u, "tag_header.vertex_buffers"));
	if (info.indices)
		vita_write_u32(tags + 28u, (uint32_t)(uintptr_t)vita_rebase_tag_pointer(
			(void *)(uintptr_t)vita_read_u32(tags + 28u), info.indices * 12u, "tag_header.index_buffers"));
	for (i = 0; i < info.tag_count; ++i) {
		unsigned char *entry = tags + table_offset + i * 32u;
		uint32_t raw_name = vita_read_u32(entry + 16u);
		uint32_t raw_root = vita_read_u32(entry + 20u);
		vita_write_u32(entry + 16u, (uint32_t)(uintptr_t)vita_rebase_tag_pointer(
			(void *)(uintptr_t)raw_name, 1u, "tag_instance.name"));
		if (raw_root)
			vita_write_u32(entry + 20u, (uint32_t)(uintptr_t)vita_rebase_tag_pointer(
				(void *)(uintptr_t)raw_root, 1u, "tag_instance.base_address"));
	}

	scenario = vita_rebase_tag_pointer((void *)(uintptr_t)scenario_root_address,
		VITA_SCENARIO_SIZE, "scenario root");
	vita_rebase_scenario_top_level(scenario);
	if (scenario->structure_bsp_references.count < 0 ||
		(unsigned long)scenario->structure_bsp_references.count > VITA_MAX_SCENARIO_BSPS)
		vita_fatal("original scenario BSP reference count exceeds Vita validation budget");
	vita_scenario_bsp_count = (uint32_t)scenario->structure_bsp_references.count;
	for (i = 0; i < vita_scenario_bsp_count; ++i) {
		struct scenario_structure_bsp_reference *reference =
			&((struct scenario_structure_bsp_reference *)scenario->structure_bsp_references.address)[i];
		uintptr_t raw_base = (uintptr_t)reference->base_address;
		uintptr_t native_base = halo_vita_memory_address(raw_base);
		uint32_t arena_offset;
		if (reference->file_offset < 0 || reference->file_size <= 0 ||
			reference->structure_bsp.group_tag != VITA_SBSP_GROUP ||
			!vita_cache_resource_range_valid((uint32_t)reference->file_offset, (size_t)reference->file_size) ||
			raw_base < HALO_XBOX_MEMORY_BASE || !native_base)
			vita_fatal("original scenario BSP reference failed typed Vita activation");
		arena_offset = (uint32_t)(raw_base - HALO_XBOX_MEMORY_BASE);
		if (arena_offset >= HALO_VITA_ARENA_SIZE ||
			(uint32_t)reference->file_size > HALO_VITA_ARENA_SIZE - arena_offset)
			vita_fatal("original scenario BSP destination exceeds Vita arena");
		vita_rebase_tag_reference(&reference->structure_bsp, "scenario.structure_bsp reference");
		vita_scenario_bsps[i].file_offset = (uint32_t)reference->file_offset;
		vita_scenario_bsps[i].file_size = (uint32_t)reference->file_size;
		vita_scenario_bsps[i].xbox_base = (uint32_t)raw_base;
		vita_scenario_bsps[i].native_base = native_base;
		reference->base_address = (void *)native_base;
		if (!i)
			vita_log("[VITA SCENARIO] first BSP destination relocated: file=[%08lx,%08lx) xbox=%08lx native=%p bytes=%lu datum=%08lx",
				(unsigned long)reference->file_offset,
				(unsigned long)(reference->file_offset + reference->file_size),
				(unsigned long)raw_base, (void *)native_base,
				(unsigned long)reference->file_size,
				(unsigned long)reference->structure_bsp.index);
	}
	vita_log("[VITA SCENARIO] original tag-image top-level relocation PASS: tags=%u scenario=%08lx tag_bytes=%ld BSPs=%u vertex_buffers=%u index_buffers=%u",
		info.tag_count, (unsigned long)info.scenario_index, size, vita_scenario_bsp_count,
		info.vertices, info.indices);
}

static void vita_activate_original_bsp_payload(void *buffer, long size)
{
	uint32_t i;
	for (i = 0; i < vita_scenario_bsp_count; ++i) {
		struct vita_scenario_bsp_binding *binding = &vita_scenario_bsps[i];
		unsigned char *bytes;
		uint32_t raw_root, vertex_count, raw_vertices, index_count, raw_indices;
		struct structure_bsp *bsp;
		if ((uintptr_t)buffer != binding->native_base)
			continue;
		if (size <= 0 || (uint32_t)size != binding->file_size)
			vita_fatal("original BSP read size differs from scenario reference");
		bytes = buffer;
		if (size < 24 || vita_read_u32(bytes + 20u) != VITA_SBSP_HEADER_SIGNATURE)
			vita_fatal("original BSP payload header/signature invalid");
		raw_root = vita_read_u32(bytes + 0u);
		vertex_count = vita_read_u32(bytes + 4u);
		raw_vertices = vita_read_u32(bytes + 8u);
		index_count = vita_read_u32(bytes + 12u);
		raw_indices = vita_read_u32(bytes + 16u);
		bsp = vita_rebase_bsp_pointer(binding, (void *)(uintptr_t)raw_root,
			sizeof(*bsp), "structure_bsp root");
		vita_write_u32(bytes + 0u, (uint32_t)(uintptr_t)bsp);
		if (vertex_count)
			vita_write_u32(bytes + 8u, (uint32_t)(uintptr_t)vita_rebase_bsp_pointer(binding,
				(void *)(uintptr_t)raw_vertices, vertex_count * 12u, "BSP vertex buffer directory"));
		if (index_count)
			vita_write_u32(bytes + 16u, (uint32_t)(uintptr_t)vita_rebase_bsp_pointer(binding,
				(void *)(uintptr_t)raw_indices, index_count * 12u, "BSP index buffer directory"));
		vita_rebase_structure_bsp_top_level(binding, bsp);
		vita_log("[VITA BSP] original BSP top-level relocation PASS: index=%u xbox=%08lx native=%p bytes=%ld root=%p vertices=%lu indices=%lu",
			i, (unsigned long)binding->xbox_base, buffer, size, bsp,
			(unsigned long)vertex_count, (unsigned long)index_count);
		return;
	}
}

static boolean first_bitmap_request_traced = FALSE;
static boolean trace_first_bitmap_request(long tag_index, long offset, long size, void *buffer)
{
	long bitmap_index;
	struct bitmap_group *group;

	if (first_bitmap_request_traced || tag_index == NONE)
		return FALSE;
	if (tag_get_group_tag(tag_index) != BITMAP_GROUP_TAG)
		return FALSE;
	group = bitmap_group_get(tag_index);
	for (bitmap_index = 0; bitmap_index < group->bitmaps.count; ++bitmap_index) {
		struct bitmap_data *bitmap = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, bitmap_index, struct bitmap_data);
		if (bitmap->pixels_offset == offset && bitmap->pixels_size == size) {
			char *path = tag_get_name(tag_index);
			unsigned long range_end = (unsigned long)offset + (unsigned long)size;
			first_bitmap_request_traced = TRUE;
			vita_log("[VITA CACHE] first original bitmap resource request: tag=%08lx path=%s bitmap=%ld type=%d format=%d dimensions=%dx%dx%d mipmaps=%d pixels_offset=%ld pixels_size=%ld logical_range=[%08lx,%08lx) destination=%p hardware_format=%p base_address=%p",
				(unsigned long)tag_index, path ? path : "<null>", bitmap_index,
				(int)bitmap->type, (int)bitmap->format,
				(int)bitmap->width, (int)bitmap->height, (int)bitmap->depth,
				(int)bitmap->mipmap_count, bitmap->pixels_offset, bitmap->pixels_size,
				(unsigned long)offset, range_end, buffer, bitmap->hardware_format,
				bitmap->base_address);
			return TRUE;
		}
	}
	/* Do not guess which bitmap supplied the bytes if the original metadata no
	 * longer matches. The generic cache request diagnostics still record it. */
	return FALSE;
}

short cache_file_read(
	long tag_index,
	long offset,
	long size,
	void *buffer,
	boolean *completion_flag_reference,
	boolean blocking)
{
	char error[160] = {0};
	boolean traced_bitmap;
	uint64_t started = vita_time_us();
	static unsigned resource_reads_traced;
	static boolean first_success_logged = FALSE;

	if (!completion_flag_reference) {
		vita_fatal("CACHE RESOURCE BLOCKED: cache_file_read completion pointer is null");
	}
	*completion_flag_reference = FALSE;
	if (!buffer || offset < 0 || size <= 0) {
		vita_log("CACHE RESOURCE BLOCKED: invalid request tag=%08lx offset=%ld size=%ld destination=%p",
			(unsigned long)tag_index, offset, size, buffer);
		vita_fatal("original texture cache cannot complete an invalid resource request");
	}
	if ((unsigned long)size > 0xFFFFFFFFUL - (unsigned long)offset) {
		vita_log("CACHE RESOURCE BLOCKED: logical range overflow tag=%08lx offset=%ld size=%ld destination=%p",
			(unsigned long)tag_index, offset, size, buffer);
		vita_fatal("original texture cache cannot complete an overflowing resource request");
	}
	traced_bitmap = trace_first_bitmap_request(tag_index, offset, size, buffer);
	if (!vita_cache_resource_read((uint32_t)offset, buffer, (size_t)size, error, sizeof(error))) {
		vita_log("CACHE RESOURCE BLOCKED: tag=%08lx logical_offset=%ld size=%ld destination=%p: %s",
			(unsigned long)tag_index, offset, size, buffer, error);
		if (traced_bitmap)
			vita_log("[VITA CACHE] first original bitmap resource read result=FAIL bytes=0 requested=%ld", size);
		vita_fatal("original texture cache cannot complete a failed resource read");
	}
	if (tag_index == NONE) {
		if (buffer == (void *)halo_vita_memory_address(HALO_XBOX_TAG_BASE))
			vita_activate_original_scenario_tag_image(buffer, size);
		else
			vita_activate_original_bsp_payload(buffer, size);
	}
	*completion_flag_reference = TRUE;
	if (resource_reads_traced < 12) {
		++resource_reads_traced;
		vita_log("[VITA RESOURCE] seek/read tag=%08lx group=%08lx offset=%ld bytes=%ld blocking=%d elapsed_us=%llu",
			(unsigned long)tag_index, tag_index == NONE ? 0UL : tag_get_group_tag(tag_index), offset, size, (int)blocking,
			(unsigned long long)(vita_time_us() - started));
	}
	if (traced_bitmap)
		vita_log("[VITA CACHE] first original bitmap resource read result=PASS bytes=%ld destination=%p", size, buffer);
	if (!first_success_logged) {
		first_success_logged = TRUE;
		vita_log("[VITA CACHE] original cache_file_read logical resource PASS: tag=%08lx logical_offset=%ld logical_end=%08lx size=%ld destination=%p blocking=%d synchronous=1",
			(unsigned long)tag_index, offset, (unsigned long)offset + (unsigned long)size,
			size, buffer, (int)blocking);
	}
	return VITA_SYNC_CACHE_REQUEST;
}

void cache_file_promote_read(short request_index)
{
	/* All successful Vita requests complete before cache_file_read returns, so
	 * the original texture-cache path should never need promotion. Keep the
	 * contract explicit and diagnose an impossible asynchronous transition. */
	if (request_index != VITA_SYNC_CACHE_REQUEST)
		vita_log("CACHE RESOURCE BLOCKED: unexpected Vita request handle %d", (int)request_index);
}

void cache_file_block_until_not_busy(void)
{
	/* There is no outstanding Vita cache I/O: current resource reads are
	 * synchronous and validated before their completion flag is raised. */
}
