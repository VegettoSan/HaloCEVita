/* Portable, SDK-ABI reader for Xbox-v5 caches. Format contract is from
 * source/cache/cache_files.c and cache_files_decompress_windows.c. */
#include "halo_vita_cache.h"
#include "halo_vita_memory.h"
#include <string.h>
#include <stdlib.h>
#include <zlib.h>

#define CHUNK 32768u
#define MAX_LOGICAL_SIZE (512u * 1024u * 1024u)
static uint32_t u32(const unsigned char *p)
{
	return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static int fail(char *error, size_t size, const char *reason)
{
	if (size) snprintf(error, size, "%s", reason);
	return 0;
}
static int range(uint32_t pointer, uint32_t bytes, size_t length)
{
	return pointer >= HALO_XBOX_TAG_BASE && pointer - HALO_XBOX_TAG_BASE <= length &&
		bytes <= length - (pointer - HALO_XBOX_TAG_BASE);
}
int vita_cache_validate_index(const void *buffer, size_t length, struct vita_cache_info *info,
	char *error, size_t error_size)
{
	const unsigned char *tags = buffer, *entry;
	uint32_t table, count, i, name, data, scenario;
	if (length < 36 || length > HALO_VITA_TAG_CAPACITY || u32(tags + 32) != 0x74616773u)
		return fail(error, error_size, "invalid tag header/signature/size");
	table = u32(tags); count = u32(tags + 12); scenario = u32(tags + 4);
	if (!count || count > 32767 || (table & 3) || !range(table, count * 32, length))
		return fail(error, error_size, "invalid tag directory bounds");
	if ((scenario & 65535u) >= count) return fail(error, error_size, "invalid scenario datum");
	info->vertices = u32(tags + 16); info->indices = u32(tags + 24);
	if (info->vertices > 32767 || info->indices > 32767 ||
		(info->vertices && ((u32(tags + 20) & 3) || !range(u32(tags + 20), info->vertices * 12, length))) ||
		(info->indices && ((u32(tags + 28) & 3) || !range(u32(tags + 28), info->indices * 12, length))))
		return fail(error, error_size, "invalid GPU resource directory bounds");
	for (i = 0; i < count; ++i) {
		entry = tags + table - HALO_XBOX_TAG_BASE + 32 * i;
		name = u32(entry + 16); data = u32(entry + 20);
		if ((u32(entry + 12) & 65535u) != i || !range(name, 1, length))
			return fail(error, error_size, "invalid tag datum/name address");
		{
			size_t remaining = length - (name - HALO_XBOX_TAG_BASE);
			if (!memchr(tags + name - HALO_XBOX_TAG_BASE, 0, remaining < 256 ? remaining : 256))
				return fail(error, error_size, "unterminated/oversized tag name");
		}
		if (data && ((data & 3) || !range(data, 1, length))) return fail(error, error_size, "invalid tag root address");
		if (i == (scenario & 65535u) &&
			(u32(entry) != 0x73636e72u || u32(entry + 12) != scenario || !range(data, 1456, length)))
			return fail(error, error_size, "scenario root/group/datum mismatch");
	}
	info->tag_count = count; info->scenario_index = scenario;
	info->tag_crc = (uint32_t)crc32(0, tags, (uInt)length);
	return 1;
}
int vita_cache_read(FILE *file, void *tags, size_t capacity, struct vita_cache_info *info,
	vita_cache_progress progress, void *context, char *error, size_t error_size)
{
	unsigned char header[2048], *input = NULL, *output = NULL;
	long disk_size;
	uint32_t position = 2048, captured = 0, next_progress = 0;
	int status = Z_OK, success = 0;
	z_stream stream;
	memset(info, 0, sizeof(*info));
	if (fseek(file, 0, SEEK_END) || (disk_size = ftell(file)) < 2048 || fseek(file, 0, SEEK_SET) ||
		fread(header, 1, sizeof(header), file) != sizeof(header)) return fail(error, error_size, "cache header I/O failed");
	info->logical_size = u32(header + 8); info->tag_offset = u32(header + 16); info->tag_size = u32(header + 20);
	if (u32(header) != 0x68656164u || u32(header + 4) != 5 || u32(header + 2044) != 0x666f6f74u ||
		!memchr(header + 32, 0, 32) || !memchr(header + 64, 0, 32) ||
		info->logical_size > MAX_LOGICAL_SIZE || info->logical_size < 2048 || info->tag_offset < 2048 ||
		info->tag_offset > info->logical_size || info->tag_size < 36 || info->tag_size > capacity ||
		info->tag_size > HALO_VITA_TAG_CAPACITY || info->tag_size > info->logical_size - info->tag_offset)
		return fail(error, error_size, "invalid Xbox-v5 cache bounds");
	info->compressed = (uint32_t)disk_size < info->logical_size;
	if (!info->compressed) {
		if (fseek(file, info->tag_offset, SEEK_SET)) return fail(error, error_size, "tag seek failed");
		while (captured < info->tag_size) {
			uint32_t size = info->tag_size - captured;
			if (size > CHUNK) size = CHUNK;
			if (fread((unsigned char *)tags + captured, 1, size, file) != size)
				return fail(error, error_size, "truncated tag section");
			captured += size;
			if (progress && !progress(info->tag_offset + captured, context)) return fail(error, error_size, "cancelled");
		}
		return vita_cache_validate_index(tags, info->tag_size, info, error, error_size);
	}
	input = malloc(CHUNK); output = malloc(CHUNK);
	if (!input || !output) { fail(error, error_size, "inflate scratch allocation failed"); goto done; }
	memset(&stream, 0, sizeof(stream));
	if (inflateInit(&stream) != Z_OK) { fail(error, error_size, "inflateInit failed"); goto done; }
	while (status != Z_STREAM_END) {
		uInt before_in, produced;
		uint32_t first, last;
		if (!stream.avail_in) {
			stream.avail_in = (uInt)fread(input, 1, CHUNK, file); stream.next_in = input;
			if (!stream.avail_in) { fail(error, error_size, "truncated compressed cache"); goto inflate_done; }
		}
		before_in = stream.avail_in;
		stream.next_out = output; stream.avail_out = CHUNK;
		status = inflate(&stream, Z_NO_FLUSH); produced = CHUNK - stream.avail_out;
		if ((status != Z_OK && status != Z_STREAM_END) || (!produced && before_in == stream.avail_in)) {
			fail(error, error_size, "invalid zlib stream/checksum or no progress"); goto inflate_done;
		}
		if (produced > info->logical_size - position) { fail(error, error_size, "inflated cache exceeds logical size"); goto inflate_done; }
		first = position > info->tag_offset ? position : info->tag_offset;
		last = position + produced < info->tag_offset + info->tag_size ? position + produced : info->tag_offset + info->tag_size;
		if (last > first) {
			memcpy((unsigned char *)tags + first - info->tag_offset, output + first - position, last - first);
			captured += last - first;
		}
		position += produced;
		if (progress && (position >= next_progress || status == Z_STREAM_END)) {
			if (!progress(position, context)) { fail(error, error_size, "cancelled"); goto inflate_done; }
			next_progress = position + 1024 * 1024;
		}
	}
	if (position != info->logical_size || captured != info->tag_size) {
		fail(error, error_size, "inflated length/tag section mismatch"); goto inflate_done;
	}
	success = vita_cache_validate_index(tags, info->tag_size, info, error, error_size);
inflate_done:
	inflateEnd(&stream);
done:
	free(input); free(output); return success;
}
