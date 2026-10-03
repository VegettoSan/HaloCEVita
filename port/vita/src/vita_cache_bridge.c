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
#include "halo_vita_cache.h"
#include "halo_vita_memory.h"
#include "vita_runtime.h"

#define VITA_SYNC_CACHE_REQUEST 0
#define VITA_SCENARIO_SIZE 1456u
#define VITA_SCENARIO_BSP_BLOCK_OFFSET 1444u
#define VITA_SCENARIO_BSP_REFERENCE_SIZE 32u
#define VITA_SBSP_GROUP 0x73627370u

/* cache_files.c exports these but the January header does not declare them. */
char *tag_get_name(long tag_index);
unsigned long tag_get_group_tag(long tag_index);

static uint32_t vita_read_u32(const unsigned char *p)
{
	return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
		(uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

/* This is validation only. It deliberately does not rewrite any pointer yet.
 * Once the original scenario_tags_load path starts using the direct Vita cache,
 * prove that the scenario root and each BSP destination/range can be translated
 * inside the placed tag arena before typed relocation is allowed to commit. */
static void vita_trace_original_scenario_tag_image(void *buffer, long size)
{
	struct vita_cache_info info;
	unsigned char *tags = buffer;
	uint32_t table_address, table_offset, scenario_entry, scenario_root_address;
	uint32_t scenario_root, bsp_count, bsp_address, bsp_offset, i;
	char error[160] = {0};

	if (buffer != (void *)halo_vita_memory_address(HALO_XBOX_TAG_BASE) || size <= 0)
		return;
	if (!vita_cache_validate_index(buffer, (size_t)size, &info, error, sizeof(error))) {
		vita_log("[VITA SCENARIO] original tag-image preflight FAIL: %s", error);
		vita_fatal("original scenario tag image failed Vita index validation");
	}

	table_address = vita_read_u32(tags);
	table_offset = table_address - HALO_XBOX_TAG_BASE;
	scenario_entry = table_offset + (info.scenario_index & 0xFFFFu) * 32u;
	if (scenario_entry > (uint32_t)size || 32u > (uint32_t)size - scenario_entry)
		vita_fatal("original scenario directory entry outside tag image");
	scenario_root_address = vita_read_u32(tags + scenario_entry + 20u);
	if (scenario_root_address < HALO_XBOX_TAG_BASE)
		vita_fatal("original scenario root is not an Xbox tag pointer");
	scenario_root = scenario_root_address - HALO_XBOX_TAG_BASE;
	if (scenario_root > (uint32_t)size || VITA_SCENARIO_SIZE > (uint32_t)size - scenario_root)
		vita_fatal("original scenario root outside tag image");

	bsp_count = vita_read_u32(tags + scenario_root + VITA_SCENARIO_BSP_BLOCK_OFFSET);
	bsp_address = vita_read_u32(tags + scenario_root + VITA_SCENARIO_BSP_BLOCK_OFFSET + 4u);
	if (bsp_count > 64u)
		vita_fatal("original scenario BSP reference count exceeds validation budget");
	if (bsp_count) {
		uint32_t bytes = bsp_count * VITA_SCENARIO_BSP_REFERENCE_SIZE;
		if (bsp_address < HALO_XBOX_TAG_BASE)
			vita_fatal("original scenario BSP block is not an Xbox tag pointer");
		bsp_offset = bsp_address - HALO_XBOX_TAG_BASE;
		if (bsp_offset > (uint32_t)size || bytes > (uint32_t)size - bsp_offset)
			vita_fatal("original scenario BSP reference block outside tag image");
		for (i = 0; i < bsp_count; ++i) {
			unsigned char *reference = tags + bsp_offset + i * VITA_SCENARIO_BSP_REFERENCE_SIZE;
			uint32_t file_offset = vita_read_u32(reference + 0u);
			uint32_t file_size = vita_read_u32(reference + 4u);
			uint32_t xbox_base = vita_read_u32(reference + 8u);
			uint32_t group = vita_read_u32(reference + 16u);
			uintptr_t native_base = halo_vita_memory_address(xbox_base);
			uint32_t arena_offset;

			if (!file_size || group != VITA_SBSP_GROUP ||
				!vita_cache_resource_range_valid(file_offset, file_size) ||
				xbox_base < HALO_XBOX_MEMORY_BASE || !native_base)
				vita_fatal("original scenario BSP reference failed typed Vita preflight");
			arena_offset = xbox_base - HALO_XBOX_MEMORY_BASE;
			if (arena_offset >= HALO_VITA_ARENA_SIZE ||
				file_size > HALO_VITA_ARENA_SIZE - arena_offset)
				vita_fatal("original scenario BSP destination exceeds Vita arena");
			if (i == 0) {
				vita_log("[VITA SCENARIO] first BSP typed preflight PASS: file=[%08lx,%08lx) xbox_base=%08lx native=%p bytes=%lu datum=%08lx",
					(unsigned long)file_offset, (unsigned long)(file_offset + file_size),
					(unsigned long)xbox_base, (void *)native_base, (unsigned long)file_size,
					(unsigned long)vita_read_u32(reference + 28u));
			}
		}
	}
	vita_log("[VITA SCENARIO] original tag-image preflight PASS: tags=%u scenario=%08lx tag_bytes=%ld BSPs=%lu vertex_buffers=%u index_buffers=%u; no pointer mutation",
		info.tag_count, (unsigned long)info.scenario_index, size, (unsigned long)bsp_count,
		info.vertices, info.indices);
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
	if (tag_index == NONE)
		vita_trace_original_scenario_tag_image(buffer, size);
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
