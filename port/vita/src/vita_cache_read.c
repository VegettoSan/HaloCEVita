/* Portable, SDK-ABI reader for Xbox-v5 caches. Format contract is from
 * source/cache/cache_files.c and cache_files_decompress_windows.c. */
#include "halo_vita_cache.h"
#include "halo_vita_memory.h"
#include <string.h>
#include <stdlib.h>
#include <zlib.h>

#define CHUNK 32768u
#define CACHE_HEADER_SIZE 2048u
#define MAX_LOGICAL_SIZE (512u * 1024u * 1024u)
#define RESOURCE_PATH_CAPACITY 320u

struct vita_cache_header_state {
	unsigned char bytes[CACHE_HEADER_SIZE];
	long disk_size;
	uint32_t logical_size;
	int compressed;
};

static char resource_map_path[RESOURCE_PATH_CAPACITY];
static uint32_t resource_map_logical_size;

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
static int cache_header_read(FILE *file, struct vita_cache_header_state *state,
	char *error, size_t error_size)
{
	if (!file || !state || fseek(file, 0, SEEK_END) ||
		(state->disk_size = ftell(file)) < (long)CACHE_HEADER_SIZE ||
		fseek(file, 0, SEEK_SET) ||
		fread(state->bytes, 1, sizeof(state->bytes), file) != sizeof(state->bytes))
		return fail(error, error_size, "cache header I/O failed");
	state->logical_size = u32(state->bytes + 8);
	if (u32(state->bytes) != 0x68656164u || u32(state->bytes + 4) != 5 ||
		u32(state->bytes + 2044) != 0x666f6f74u ||
		!memchr(state->bytes + 32, 0, 32) || !memchr(state->bytes + 64, 0, 32) ||
		state->logical_size > MAX_LOGICAL_SIZE || state->logical_size < CACHE_HEADER_SIZE)
		return fail(error, error_size, "invalid Xbox-v5 cache header");
	state->compressed = (uint32_t)state->disk_size < state->logical_size;
	return 1;
}
static int cache_capture_range(FILE *file, const struct vita_cache_header_state *state,
	uint32_t logical_offset, uint32_t bytes, void *destination,
	vita_cache_progress progress, void *context, char *error, size_t error_size)
{
	unsigned char *target = destination;
	uint32_t captured = 0, end, next_progress = 0;
	if (logical_offset > state->logical_size || bytes > state->logical_size - logical_offset)
		return fail(error, error_size, "logical range outside cache");
	if (!bytes) return 1;
	if (!destination) return fail(error, error_size, "logical range destination is null");
	end = logical_offset + bytes;
	if (!state->compressed) {
		if (fseek(file, (long)logical_offset, SEEK_SET))
			return fail(error, error_size, "logical range seek failed");
		while (captured < bytes) {
			uint32_t amount = bytes - captured;
			if (amount > CHUNK) amount = CHUNK;
			if (fread(target + captured, 1, amount, file) != amount)
				return fail(error, error_size, "truncated logical range");
			captured += amount;
			if (progress && !progress(logical_offset + captured, context))
				return fail(error, error_size, "cancelled");
		}
		return 1;
	}
	if (logical_offset < CACHE_HEADER_SIZE) {
		uint32_t first_end = end < CACHE_HEADER_SIZE ? end : CACHE_HEADER_SIZE;
		uint32_t header_bytes = first_end - logical_offset;
		memcpy(target, state->bytes + logical_offset, header_bytes);
		captured += header_bytes;
	}
	if (end <= CACHE_HEADER_SIZE) return 1;
	{
		unsigned char *input = NULL, *output = NULL;
		uint32_t position = CACHE_HEADER_SIZE;
		int status = Z_OK, success = 0;
		z_stream stream;
		if (fseek(file, CACHE_HEADER_SIZE, SEEK_SET))
			return fail(error, error_size, "compressed stream seek failed");
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
			if (produced > state->logical_size - position) {
				fail(error, error_size, "inflated cache exceeds logical size"); goto inflate_done;
			}
			first = position > logical_offset ? position : logical_offset;
			if (first < CACHE_HEADER_SIZE) first = CACHE_HEADER_SIZE;
			last = position + produced < end ? position + produced : end;
			if (last > first) {
				memcpy(target + first - logical_offset, output + first - position, last - first);
				captured += last - first;
			}
			position += produced;
			if (progress && (position >= next_progress || status == Z_STREAM_END)) {
				if (!progress(position, context)) { fail(error, error_size, "cancelled"); goto inflate_done; }
				next_progress = position + 1024 * 1024;
			}
		}
		if (position != state->logical_size || captured != bytes) {
			fail(error, error_size, "inflated length/logical range mismatch"); goto inflate_done;
		}
		success = 1;
inflate_done:
		inflateEnd(&stream);
done:
		free(input); free(output); return success;
	}
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
	struct vita_cache_header_state state;
	if (!info) return fail(error, error_size, "cache info is null");
	memset(info, 0, sizeof(*info));
	if (!cache_header_read(file, &state, error, error_size)) return 0;
	info->logical_size = state.logical_size;
	info->tag_offset = u32(state.bytes + 16); info->tag_size = u32(state.bytes + 20);
	if (info->tag_offset < CACHE_HEADER_SIZE || info->tag_offset > info->logical_size ||
		info->tag_size < 36 || info->tag_size > capacity || info->tag_size > HALO_VITA_TAG_CAPACITY ||
		info->tag_size > info->logical_size - info->tag_offset)
		return fail(error, error_size, "invalid Xbox-v5 cache bounds");
	info->compressed = state.compressed;
	if (!cache_capture_range(file, &state, info->tag_offset, info->tag_size, tags,
		progress, context, error, error_size)) return 0;
	return vita_cache_validate_index(tags, info->tag_size, info, error, error_size);
}
int vita_cache_read_logical_range(FILE *file, uint32_t expected_logical_size,
	uint32_t logical_offset, void *destination, size_t bytes,
	vita_cache_progress progress, void *context, char *error, size_t error_size)
{
	struct vita_cache_header_state state;
	if (bytes > UINT32_MAX) return fail(error, error_size, "logical range too large");
	if (!cache_header_read(file, &state, error, error_size)) return 0;
	if (expected_logical_size && state.logical_size != expected_logical_size)
		return fail(error, error_size, "cache logical size changed");
	return cache_capture_range(file, &state, logical_offset, (uint32_t)bytes, destination,
		progress, context, error, error_size);
}
int vita_cache_resource_bind(const char *path, uint32_t logical_size)
{
	size_t length;
	if (!path || !*path || logical_size < CACHE_HEADER_SIZE || logical_size > MAX_LOGICAL_SIZE) return 0;
	length = strlen(path);
	if (length >= sizeof(resource_map_path)) return 0;
	memcpy(resource_map_path, path, length + 1);
	resource_map_logical_size = logical_size;
	return 1;
}
void vita_cache_resource_unbind(void)
{
	resource_map_path[0] = 0;
	resource_map_logical_size = 0;
}
int vita_cache_resource_read(uint32_t logical_offset, void *destination, size_t bytes,
	char *error, size_t error_size)
{
	FILE *file;
	int result;
	if (!resource_map_path[0] || !resource_map_logical_size)
		return fail(error, error_size, "no cache resource map bound");
	file = fopen(resource_map_path, "rb");
	if (!file) return fail(error, error_size, "cache resource map open failed");
	result = vita_cache_read_logical_range(file, resource_map_logical_size, logical_offset,
		destination, bytes, NULL, NULL, error, error_size);
	fclose(file);
	return result;
}
