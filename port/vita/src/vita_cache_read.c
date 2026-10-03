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
static FILE *resource_file;
static char resource_bind_error[160];

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
	uint32_t logical_offset, uint32_t bytes, void *destination, int validate_stream,
	vita_cache_progress progress, void *context, char *error, size_t error_size, FILE *logical_copy)
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
	if (logical_copy && fwrite(state->bytes, 1, CACHE_HEADER_SIZE, logical_copy) != CACHE_HEADER_SIZE)
		return fail(error, error_size, "logical resource header write failed");
	if (end <= CACHE_HEADER_SIZE && !logical_copy) return 1;
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
			if (logical_copy && produced && fwrite(output, 1, produced, logical_copy) != produced) {
				fail(error, error_size, "logical resource payload write failed (check free storage)"); goto inflate_done;
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
			/* After startup has validated the complete map, resource reads only
			 * need the requested logical prefix. Once the range is complete, do
			 * not decompress the rest of a potentially hundreds-of-megabytes map.
			 * Requests reaching logical EOF still validate the zlib checksum. */
			if (!validate_stream && end < state->logical_size && captured == bytes && position >= end) {
				success = 1;
				goto inflate_done;
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
	if (!cache_capture_range(file, &state, info->tag_offset, info->tag_size, tags, 1,
		progress, context, error, error_size, NULL)) return 0;
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
	return cache_capture_range(file, &state, logical_offset, (uint32_t)bytes, destination, 1,
		progress, context, error, error_size, NULL);
}

static int cache_slot_path_from_source(const char *source, unsigned slot_index,
	char *out, size_t capacity)
{
	char directory[RESOURCE_PATH_CAPACITY];
	char *slash, *parent;
	int length;
	size_t source_length;

	if (!source || !*source || !out || !capacity || slot_index >= 6)
		return 0;
	source_length = strlen(source);
	if (source_length >= sizeof(directory))
		return 0;
	memcpy(directory, source, source_length + 1);
	slash = strrchr(directory, '/');
	if (!slash)
		slash = strrchr(directory, '\\');
	if (!slash) {
		length = snprintf(out, capacity, "cache%03u.map", slot_index);
		return length > 0 && (size_t)length < capacity;
	}
	*slash = 0;
	parent = strrchr(directory, '/');
	if (!parent)
		parent = strrchr(directory, '\\');
	/* d:\\maps and z:\\ are separate volumes on Xbox. Vita keeps the same
	 * ownership split inside HaloCE/: maps/ contains only user source maps,
	 * while cacheNNN.map lives beside maps/. Host regressions use the same rule. */
	if (parent && !strcmp(parent + 1, "maps"))
		*parent = 0;
	length = snprintf(out, capacity, "%s/cache%03u.map", directory, slot_index);
	return length > 0 && (size_t)length < capacity;
}

static int cache_slot_matches_state(const char *slot_path,
	const struct vita_cache_header_state *source_state, char *error, size_t error_size)
{
	FILE *slot;
	struct vita_cache_header_state slot_state;
	int result = 0;

	slot = fopen(slot_path, "rb");
	if (!slot)
		return fail(error, error_size, "cache slot missing");
	if (!cache_header_read(slot, &slot_state, error, error_size))
		goto done;
	if (slot_state.disk_size != (long)source_state->logical_size || slot_state.compressed ||
		slot_state.logical_size != source_state->logical_size) {
		fail(error, error_size, "cache slot is not exact logical size");
		goto done;
	}
	/* Upstream cache_files_open_cache_files compares the cached header/checksum
	 * to the source map. Exact 0x800 equality is a stricter form of the same
	 * identity check and guarantees the Vita cache exposes the source header
	 * byte-for-byte, including NTSC/PAL build and scenario metadata. */
	if (memcmp(slot_state.bytes, source_state->bytes, CACHE_HEADER_SIZE)) {
		fail(error, error_size, "cache slot header/source identity mismatch");
		goto done;
	}
	result = 1;
done:
	fclose(slot);
	return result;
}

int vita_cache_slot_valid(const char *source_path, unsigned slot_index,
	char *slot_path, size_t slot_path_size, char *error, size_t error_size)
{
	FILE *source;
	struct vita_cache_header_state source_state;
	int result;

	if (!cache_slot_path_from_source(source_path, slot_index, slot_path, slot_path_size))
		return fail(error, error_size, "cache slot path failed");
	source = fopen(source_path, "rb");
	if (!source)
		return fail(error, error_size, "cache source open failed");
	if (!cache_header_read(source, &source_state, error, error_size)) {
		fclose(source);
		return 0;
	}
	fclose(source);
	if (source_state.disk_size > (long)source_state.logical_size)
		return fail(error, error_size, "source cache exceeds declared logical size");
	result = cache_slot_matches_state(slot_path, &source_state, error, error_size);
	return result;
}

static int cache_copy_plain_payload(FILE *source, FILE *destination, uint32_t payload_size,
	vita_cache_progress progress, void *context, char *error, size_t error_size)
{
	unsigned char *buffer = NULL;
	uint32_t copied = 0;
	int result = 0;

	buffer = malloc(CHUNK);
	if (!buffer)
		return fail(error, error_size, "cache copy scratch allocation failed");
	if (fseek(source, CACHE_HEADER_SIZE, SEEK_SET)) {
		fail(error, error_size, "cache source payload seek failed");
		goto done;
	}
	while (copied < payload_size) {
		uint32_t amount = payload_size - copied;
		if (amount > CHUNK) amount = CHUNK;
		if (fread(buffer, 1, amount, source) != amount) {
			fail(error, error_size, "cache source payload truncated");
			goto done;
		}
		if (fwrite(buffer, 1, amount, destination) != amount) {
			fail(error, error_size, "cache slot payload write failed");
			goto done;
		}
		copied += amount;
		if (progress && !progress(CACHE_HEADER_SIZE + copied, context)) {
			fail(error, error_size, "cancelled");
			goto done;
		}
	}
	result = 1;
done:
	free(buffer);
	return result;
}

static int cache_inflate_payload(FILE *source, FILE *destination, uint32_t payload_size,
	vita_cache_progress progress, void *context, char *error, size_t error_size)
{
	unsigned char *input = NULL, *output = NULL;
	uint32_t written = 0, next_progress = 0;
	int status = Z_OK, result = 0;
	z_stream stream;

	if (fseek(source, CACHE_HEADER_SIZE, SEEK_SET))
		return fail(error, error_size, "compressed cache payload seek failed");
	input = malloc(CHUNK);
	output = malloc(CHUNK);
	if (!input || !output) {
		fail(error, error_size, "cache inflate scratch allocation failed");
		goto done;
	}
	memset(&stream, 0, sizeof(stream));
	if (inflateInit(&stream) != Z_OK) {
		fail(error, error_size, "cache inflateInit failed");
		goto done;
	}
	while (status != Z_STREAM_END) {
		uInt before_in, produced;
		if (!stream.avail_in) {
			stream.avail_in = (uInt)fread(input, 1, CHUNK, source);
			stream.next_in = input;
			if (!stream.avail_in) {
				fail(error, error_size, "truncated compressed cache payload");
				goto inflate_done;
			}
		}
		before_in = stream.avail_in;
		stream.next_out = output;
		stream.avail_out = CHUNK;
		status = inflate(&stream, Z_NO_FLUSH);
		produced = CHUNK - stream.avail_out;
		if ((status != Z_OK && status != Z_STREAM_END) ||
			(!produced && before_in == stream.avail_in)) {
			fail(error, error_size, "invalid compressed cache zlib stream/checksum");
			goto inflate_done;
		}
		if (produced > payload_size - written) {
			fail(error, error_size, "inflated cache payload exceeds declared logical size");
			goto inflate_done;
		}
		if (produced && fwrite(output, 1, produced, destination) != produced) {
			fail(error, error_size, "cache slot payload write failed");
			goto inflate_done;
		}
		written += produced;
		if (progress && (written >= next_progress || status == Z_STREAM_END)) {
			if (!progress(CACHE_HEADER_SIZE + written, context)) {
				fail(error, error_size, "cancelled");
				goto inflate_done;
			}
			next_progress = written + 1024 * 1024;
		}
	}
	if (written != payload_size) {
		fail(error, error_size, "inflated cache payload length mismatch");
		goto inflate_done;
	}
	result = 1;
inflate_done:
	inflateEnd(&stream);
done:
	free(input);
	free(output);
	return result;
}

int vita_cache_prepare_slot(const char *source_path, unsigned slot_index,
	char *slot_path, size_t slot_path_size, int *reused,
	vita_cache_progress progress, void *context, char *error, size_t error_size)
{
	FILE *source = NULL, *slot = NULL, *verify = NULL;
	struct vita_cache_header_state source_state, verify_state;
	unsigned char blank_header[CACHE_HEADER_SIZE];
	uint32_t payload_size;
	int result = 0;

	if (reused) *reused = 0;
	if (!cache_slot_path_from_source(source_path, slot_index, slot_path, slot_path_size))
		return fail(error, error_size, "cache slot path failed");
	source = fopen(source_path, "rb");
	if (!source)
		return fail(error, error_size, "cache source open failed");
	if (!cache_header_read(source, &source_state, error, error_size))
		goto done;
	if (source_state.disk_size > (long)source_state.logical_size) {
		fail(error, error_size, "source cache exceeds declared logical size");
		goto done;
	}
	if (cache_slot_matches_state(slot_path, &source_state, error, error_size)) {
		if (reused) *reused = 1;
		result = 1;
		goto done;
	}

	/* cache_files_decompress_windows.c invalidates the destination first by
	 * writing a blank 0x800 header, writes/decompresses the payload at 0x800,
	 * waits for every payload write, then commits the original header last.
	 * Preserve that ordering: a crash or write failure cannot leave a cache that
	 * looks valid merely because its header was published early. */
	slot = fopen(slot_path, "w+b");
	if (!slot) {
		fail(error, error_size, "cache slot create failed");
		goto done;
	}
	memset(blank_header, 0, sizeof(blank_header));
	if (fwrite(blank_header, 1, sizeof(blank_header), slot) != sizeof(blank_header)) {
		fail(error, error_size, "cache slot blank header write failed");
		goto done;
	}
	payload_size = source_state.logical_size - CACHE_HEADER_SIZE;
	if (source_state.compressed) {
		if (!cache_inflate_payload(source, slot, payload_size,
			progress, context, error, error_size))
			goto done;
	} else {
		if (source_state.disk_size != (long)source_state.logical_size) {
			fail(error, error_size, "uncompressed source size/logical size mismatch");
			goto done;
		}
		if (!cache_copy_plain_payload(source, slot, payload_size,
			progress, context, error, error_size))
			goto done;
	}
	if (fflush(slot) || fseek(slot, 0, SEEK_END) ||
		ftell(slot) != (long)source_state.logical_size) {
		fail(error, error_size, "cache slot payload flush/size failed");
		goto done;
	}
	if (fseek(slot, 0, SEEK_SET) ||
		fwrite(source_state.bytes, 1, CACHE_HEADER_SIZE, slot) != CACHE_HEADER_SIZE ||
		fflush(slot)) {
		fail(error, error_size, "cache slot final header commit failed");
		goto done;
	}
	fclose(slot);
	slot = NULL;

	verify = fopen(slot_path, "rb");
	if (!verify || !cache_header_read(verify, &verify_state, error, error_size))
		goto done;
	if (verify_state.disk_size != (long)source_state.logical_size || verify_state.compressed ||
		memcmp(verify_state.bytes, source_state.bytes, CACHE_HEADER_SIZE)) {
		fail(error, error_size, "committed cache slot failed final identity validation");
		goto done;
	}
	result = 1;
done:
	if (verify) fclose(verify);
	if (slot) fclose(slot);
	if (source) fclose(source);
	if (!result) {
		/* Original preallocated Xbox slots remain present but invalid after a
		 * failed copy. Vita may remove the failed file entirely; either way no
		 * valid header survives and cache_file_open cannot consume partial data. */
		remove(slot_path);
	}
	return result;
}

/* Resource reads never decompress. They attach only to a fully committed
 * logical cache slot prepared by vita_cache_prepare_slot. */
int vita_cache_resource_bind(const char *path, uint32_t logical_size)
{
	FILE *source = NULL;
	struct vita_cache_header_state state;

	vita_cache_resource_unbind();
	resource_bind_error[0] = 0;
	if (!path || !*path || logical_size < CACHE_HEADER_SIZE || logical_size > MAX_LOGICAL_SIZE)
		return fail(resource_bind_error, sizeof(resource_bind_error), "invalid resource map/size");
	if (strlen(path) + 1 > sizeof(resource_map_path))
		return fail(resource_bind_error, sizeof(resource_bind_error), "resource path too long");
	source = fopen(path, "rb");
	if (!source)
		return fail(resource_bind_error, sizeof(resource_bind_error), "resource cache slot open failed");
	if (!cache_header_read(source, &state, resource_bind_error, sizeof(resource_bind_error))) {
		fclose(source);
		return 0;
	}
	if (state.logical_size != logical_size) {
		fclose(source);
		return fail(resource_bind_error, sizeof(resource_bind_error), "resource logical size changed");
	}
	if (state.compressed || state.disk_size != (long)state.logical_size) {
		fclose(source);
		return fail(resource_bind_error, sizeof(resource_bind_error),
			"compressed source must be precached before resource bind");
	}
	memcpy(resource_map_path, path, strlen(path) + 1);
	resource_map_logical_size = logical_size;
	resource_file = source;
	return 1;
}
const char *vita_cache_resource_error(void) { return resource_bind_error; }
void vita_cache_resource_unbind(void)
{
	if (resource_file) fclose(resource_file);
	resource_file = NULL;
	resource_map_path[0] = 0;
	resource_map_logical_size = 0;
}
int vita_cache_resource_range_valid(uint32_t logical_offset, size_t bytes)
{
	return resource_file && resource_map_logical_size && bytes &&
		logical_offset >= CACHE_HEADER_SIZE && logical_offset <= resource_map_logical_size &&
		bytes <= resource_map_logical_size - logical_offset;
}
int vita_cache_resource_read(uint32_t logical_offset, void *destination, size_t bytes,
	char *error, size_t error_size)
{
	if (!resource_file || !resource_map_logical_size)
		return fail(error, error_size, "no cache resource map bound");
	if (!destination || !vita_cache_resource_range_valid(logical_offset, bytes))
		return fail(error, error_size, "invalid bound resource range/destination");
	if (fseek(resource_file, (long)logical_offset, SEEK_SET))
		return fail(error, error_size, "logical resource seek failed");
	if (fread(destination, 1, bytes, resource_file) != bytes)
		return fail(error, error_size, "logical resource read truncated");
	return 1;
}