/* Executable probes of actual upstream Halo systems; no call to main/main.c
 * is claimed while its physical-memory and renderer contracts are unresolved. */
#include "cseries.h"
#include "cseries_windows.h"
#include "memory/data.h"
#include "memory/memory_pool.h"
#include "memory/crc.h"
#include "math/random_math.h"
#include "cache/cache_files.h"
#include "vita_runtime.h"
#include "xgpu.h"

#undef free
#undef memcpy

/* The upstream cache struct is private to cache_files.c. Reproduce its wire
 * declaration here and assert it; use its real verifier, never a guessed
 * replacement parser. See source/cache/cache_files.c:181. */
struct cache_file_header
{
	unsigned long header_signature;
	long version, file_length;
	byte reservedC[4];
	long tag_data_offset, tag_data_size;
	byte reserved18[8];
	char name[0x20], build[0x20];
	byte reserved60[4];
	unsigned long checksum;
	byte reserved68[0x794];
	unsigned long footer_signature;
};
typedef char vita_pointer32[sizeof(void *) == 4 ? 1 : -1];
typedef char vita_long32[sizeof(long) == 4 ? 1 : -1];
typedef char vita_wchar16[sizeof(wchar_t) == 2 ? 1 : -1];
enum vita_enum_probe { vita_enum_zero, vita_enum_one };
typedef char vita_enum32[sizeof(enum vita_enum_probe) == 4 ? 1 : -1];
struct vita_align_probe { char first; unsigned long long second; };
typedef char vita_align64[offsetof(struct vita_align_probe, second) == 8 ? 1 : -1];
typedef char vita_data_array_layout[sizeof(struct data_array) == 0x38 ? 1 : -1];
typedef char vita_pool_layout[sizeof(struct memory_pool) == 0x38 ? 1 : -1];
typedef char vita_pool_block_layout[sizeof(struct memory_pool_block) == 0x18 ? 1 : -1];
typedef char vita_cache_header_layout[sizeof(struct cache_file_header) == 0x800 ? 1 : -1];
typedef char vita_cache_footer_layout[offsetof(struct cache_file_header, footer_signature) == 0x7FC ? 1 : -1];
typedef char vita_xdk_long_layout[sizeof(DWORD) == 4 && sizeof(LARGE_INTEGER) == 8 ? 1 : -1];

struct probe_datum { short identifier; short pad; long value; };

int halo_vita_core_initialize(void)
{
	struct data_array *data;
	struct data_iterator iterator;
	struct probe_datum *datum;
	struct memory_pool *pool;
	void *a = NULL, *b = NULL;
	long index;
	unsigned long crc;
	int success = 1;
	HANDLE pad;
	XINPUT_STATE input;
	vita_log("ABI pointer=%u long=%u wchar=%u data_array=%u pool=%u cache_header=%u",
		(unsigned)sizeof(void *), (unsigned)sizeof(long), (unsigned)sizeof(wchar_t),
		(unsigned)sizeof(struct data_array), (unsigned)sizeof(struct memory_pool),
		(unsigned)sizeof(struct cache_file_header));
	cseries_initialize();
	vita_log("Halo cseries_initialize + debug_memory_manager_initialize + profile_initialize returned");
	pad = XInputOpen(XDEVICE_TYPE_GAMEPAD, 0, 0, NULL);
	if (pad) {
		vita_log("Halo XInputGetState native bridge result=%lu", (unsigned long)XInputGetState(pad, &input));
		XInputClose(pad);
	}
	data = data_new("vita native datum probe", 16, sizeof(struct probe_datum));
	if (!data) return 0;
	data_make_valid(data);
	index = datum_new(data);
	datum = datum_try_and_get(data, index);
	if (!datum) success = 0;
	else {
		datum->value = 0x12345678;
		data_iterator_new(&iterator, data);
		success &= data_iterator_next(&iterator) == datum;
		success &= datum->value == 0x12345678;
		datum_delete(data, index);
		success &= datum_try_and_get(data, index) == NULL;
	}
	data_dispose(data);
	pool = memory_pool_new("vita native pool probe", 64 * 1024);
	if (!pool) return 0;
	success &= memory_pool_block_allocate(pool, &a, 1024);
	success &= memory_pool_block_allocate(pool, &b, 2048);
	if (a && b) {
		csmemset(b, 0x5A, 2048);
		memory_pool_block_free(pool, &a);
		memory_pool_compact(pool);
		success &= ((byte *)b)[0] == 0x5A && ((byte *)b)[2047] == 0x5A;
		memory_pool_block_free(pool, &b);
	}
	memory_pool_delete(pool);
	crc_new(&crc); crc_checksum_buffer(&crc, "123456789", 9);
	success &= crc == 0x340BC6D9UL; /* upstream stores the uncomplemented CRC */
	debug_check_memory(__FILE__, __LINE__);
	vita_log("Halo data/datum/iterator, compacting pool, guarded heap, CRC: %s crc=%08lx",
		success ? "PASS" : "FAIL", crc);
	return success;
}

int halo_vita_verify_map(const void *bytes, size_t length, const char *name)
{
	struct cache_file_header header;
	if (length != sizeof(header)) return 0;
	memcpy(&header, bytes, sizeof(header));
	/* The upstream verifier calls strlen(name); ensure corrupt data cannot
 * scan outside its fixed-size name/build fields before handing it over. */
	if (!memchr(header.name, 0, sizeof(header.name)) ||
		!memchr(header.build, 0, sizeof(header.build))) return 0;
	vita_log("map %s: version=%ld logical_size=%ld tags_offset=%ld tags_size=%ld build=%s",
		name, header.version, header.file_length, header.tag_data_offset, header.tag_data_size, header.build);
	return cache_file_header_verify(&header, name, FALSE);
}

char *halo_vita_vertex_shader(void)
{
	/* Two NV2A MOV instructions: v0 -> oPos, v3 -> oD0. A translator probe,
 * not retail game microcode; texture/geometry from maps is not drawn yet. */
	DWORD code[8] = {0};
	code[1] = (1UL << 21) | 0x1B;
	code[2] = 2UL << 26;
	code[3] = (15UL << 12) | (1UL << 11);
	code[5] = (1UL << 21) | (3UL << 9) | 0x1B;
	code[6] = 2UL << 26;
	code[7] = (15UL << 12) | (1UL << 11) | (3UL << 3) | 1;
	return nv2a_vertex_shader_to_glsl(code, 2, 0);
}
char *halo_vita_pixel_shader(void)
{
	struct nv2a_pixel_shader_key key;
	csmemset(&key, 0, sizeof(key));
	/* final RGB = v0, alpha = v0.a, through the upstream combiner generator */
	key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] = 0x04200000;
	key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG] = 0x00001400;
	return nv2a_pixel_shader_to_glsl(&key);
}
void halo_vita_core_dispose(void) { cseries_dispose(); }
