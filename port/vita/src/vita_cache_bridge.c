/* Game-ABI side of the Vita cache resource boundary.
 *
 * Xbox cache_file_read() is asynchronous because the retail path reads a
 * sector-aligned uncompressed cache. Vita currently keeps the user's Xbox map
 * compressed and resolves each request through the checked logical-range
 * reader. The operation is synchronous: completion is TRUE only after the
 * exact requested bytes have been inflated and the zlib stream/final logical
 * length have been validated. This preserves the original caller contract
 * without a parallel asset loader or fake completion.
 */
#include "cseries/cseries.h"
#include "cache/cache_files.h"
#include "bitmaps/bitmap_group.h"
#include "tag_files/tag_groups.h"
#include "halo_vita_cache.h"
#include "vita_runtime.h"

#define VITA_SYNC_CACHE_REQUEST 0

/* cache_files.c exports these but the January header does not declare them. */
char *tag_get_name(long tag_index);
unsigned long tag_get_group_tag(long tag_index);

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
	static boolean first_success_logged = FALSE;

	if (!completion_flag_reference) {
		vita_log("CACHE RESOURCE BLOCKED: cache_file_read completion pointer is null");
		return NONE;
	}
	*completion_flag_reference = FALSE;
	if (!buffer || offset < 0 || size <= 0) {
		vita_log("CACHE RESOURCE BLOCKED: invalid request tag=%08lx offset=%ld size=%ld destination=%p",
			(unsigned long)tag_index, offset, size, buffer);
		return NONE;
	}
	if ((unsigned long)size > 0xFFFFFFFFUL - (unsigned long)offset) {
		vita_log("CACHE RESOURCE BLOCKED: logical range overflow tag=%08lx offset=%ld size=%ld destination=%p",
			(unsigned long)tag_index, offset, size, buffer);
		return NONE;
	}
	traced_bitmap = trace_first_bitmap_request(tag_index, offset, size, buffer);
	if (!vita_cache_resource_read((uint32_t)offset, buffer, (size_t)size, error, sizeof(error))) {
		vita_log("CACHE RESOURCE BLOCKED: tag=%08lx logical_offset=%ld size=%ld destination=%p: %s",
			(unsigned long)tag_index, offset, size, buffer, error);
		if (traced_bitmap)
			vita_log("[VITA CACHE] first original bitmap resource read result=FAIL bytes=0 requested=%ld", size);
		return NONE;
	}
	*completion_flag_reference = TRUE;
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
