/* Vita keeps Halo's cache-file API and original scenario/game callers. The
 * retail storage contract is preserved: source maps live under d:\\maps while
 * cache_file_open/read consume an already prepared, uncompressed z:\\cacheNNN
 * image. Vita adapts only those volumes to ux0:data/HaloCE/maps/<name>.map and
 * ux0:data/HaloCE/cacheNNN.map; it does not reinterpret map payload bytes.
 *
 * The original Windows unit remains compiled below for its GPU helpers and as
 * the reference implementation, while the public storage entry points are
 * redirected to the synchronous Vita file boundary. */
#include "vita_runtime.h"
#include "halo_vita_cache.h"
#include "halo_vita_memory.h"

#define tags_header_register_vertex_and_index_buffers halo_vita_dvd_tags_header_register_vertex_and_index_buffers
#define structure_bsp_header_register_vertex_buffers halo_vita_dvd_structure_bsp_header_register_vertex_buffers
#define cache_files_initialize halo_vita_dvd_cache_files_initialize
#define cache_files_dispose halo_vita_dvd_cache_files_dispose
#define cache_files_precache_set_priority halo_vita_dvd_cache_files_precache_set_priority
#define cache_files_precache_in_progress halo_vita_dvd_cache_files_precache_in_progress
#define cache_files_precache_is_copying_map halo_vita_dvd_cache_files_precache_is_copying_map
#define cache_files_precache_map_queue_end halo_vita_dvd_cache_files_precache_map_queue_end
#define cache_files_precache_map_loaded halo_vita_dvd_cache_files_precache_map_loaded
#define cache_files_precache_map_begin halo_vita_dvd_cache_files_precache_map_begin
#define cache_files_precache_map_status halo_vita_dvd_cache_files_precache_map_status
#define cache_files_precache_map_end halo_vita_dvd_cache_files_precache_map_end
#define cache_file_open halo_vita_dvd_cache_file_open
#define cache_file_close halo_vita_dvd_cache_file_close
#define cache_file_read halo_vita_dvd_cache_file_read
#define cache_file_promote_read halo_vita_dvd_cache_file_promote_read
#define cache_file_block_until_not_busy halo_vita_dvd_cache_file_block_until_not_busy
#include "../../../source/cache/cache_files_windows.c"
#undef tags_header_register_vertex_and_index_buffers
#undef structure_bsp_header_register_vertex_buffers
#undef cache_files_initialize
#undef cache_files_dispose
#undef cache_files_precache_set_priority
#undef cache_files_precache_in_progress
#undef cache_files_precache_is_copying_map
#undef cache_files_precache_map_queue_end
#undef cache_files_precache_map_loaded
#undef cache_files_precache_map_begin
#undef cache_files_precache_map_status
#undef cache_files_precache_map_end
#undef cache_file_open
#undef cache_file_close
#undef cache_file_read
#undef cache_file_promote_read
#undef cache_file_block_until_not_busy

static DWORD vita_compiled_vertex_data_to_physical(DWORD data, const char *owner, long index)
{
    uintptr_t native;
    if (!data)
        return 0;
    native = halo_vita_memory_address((uintptr_t)data);
    if (!native) {
        vita_log("[VITA GPU] %s vertex Data outside Xbox arena: index=%ld data=%08lx",
            owner, index, (unsigned long)data);
        vita_fatal("compiled vertex buffer Data cannot be mapped into Vita arena");
    }
    return (DWORD)(native - halo_vita_memory_base());
}

static DWORD vita_compiled_index_data_to_native(DWORD data, long index)
{
    uintptr_t native;
    if (!data)
        return 0;
    native = halo_vita_memory_address((uintptr_t)data);
    if (!native) {
        vita_log("[VITA GPU] tag index-buffer Data outside Xbox arena: index=%ld data=%08lx",
            index, (unsigned long)data);
        vita_fatal("compiled index buffer Data cannot be mapped into Vita arena");
    }
    return (DWORD)native;
}

void tags_header_register_vertex_and_index_buffers(struct cache_file_tag_header *header)
{
    short index;
    DWORD first_vertex_raw = 0;
    DWORD first_index_raw = 0;

    if (!header || header->vertex_buffer_count < 0 || header->index_buffer_count < 0 ||
        (header->vertex_buffer_count && !header->vertex_buffers) ||
        (header->index_buffer_count && !header->index_buffers))
        vita_fatal("compiled tag GPU directory is invalid before registration");

    for (index = 0; index < header->vertex_buffer_count; ++index) {
        D3DVertexBuffer *vertex_buffer = &header->vertex_buffers[index];
        DWORD raw = vertex_buffer->Data;
        if (!index)
            first_vertex_raw = raw;
        vertex_buffer->Common = D3DCOMMON_TYPE_VERTEXBUFFER | 1;
        vertex_buffer->Data = vita_compiled_vertex_data_to_physical(raw, "tag", index);
        /* d3d8_resources.c Register adds base to Data, then stores a physical
         * offset. Xbox compiled buffers carry an absolute 0x8... VA; after the
         * conversion above Data is the arena offset, so the real movable Vita
         * arena is the correct registration base. */
        IDirect3DVertexBuffer8_Register(vertex_buffer, (void *)halo_vita_memory_base());
    }

    for (index = 0; index < header->index_buffer_count; ++index) {
        D3DIndexBuffer *index_buffer = &header->index_buffers[index];
        DWORD raw = index_buffer->Data;
        if (!index)
            first_index_raw = raw;
        index_buffer->Common = D3DCOMMON_TYPE_INDEXBUFFER | 1;
        /* SetIndices stores this field as a WORD * directly. Unlike vertex
         * buffers it is therefore a native virtual pointer, not a physical
         * arena offset. */
        index_buffer->Data = vita_compiled_index_data_to_native(raw, index);
    }

    vita_log("[VITA GPU] compiled tag buffers registered: vertex=%d index=%d first_vertex_xbox=%08lx first_index_xbox=%08lx",
        (int)header->vertex_buffer_count, (int)header->index_buffer_count,
        (unsigned long)first_vertex_raw, (unsigned long)first_index_raw);
}

void structure_bsp_header_register_vertex_buffers(struct cache_file_structure_bsp_header *header)
{
    short index;
    DWORD first_vertex_raw = 0;
    DWORD first_lightmap_raw = 0;

    if (!header || header->vertex_buffer_count < 0 || header->lightmap_vertex_buffer_count < 0 ||
        (header->vertex_buffer_count && !header->vertex_buffers) ||
        (header->lightmap_vertex_buffer_count && !header->lightmap_vertex_buffers))
        vita_fatal("compiled BSP GPU directory is invalid before registration");

    for (index = 0; index < header->vertex_buffer_count; ++index) {
        D3DVertexBuffer *vertex_buffer = &header->vertex_buffers[index];
        DWORD raw = vertex_buffer->Data;
        if (!index)
            first_vertex_raw = raw;
        vertex_buffer->Common = D3DCOMMON_TYPE_VERTEXBUFFER | 1;
        vertex_buffer->Data = vita_compiled_vertex_data_to_physical(raw, "BSP", index);
        IDirect3DVertexBuffer8_Register(vertex_buffer, (void *)halo_vita_memory_base());
    }

    for (index = 0; index < header->lightmap_vertex_buffer_count; ++index) {
        D3DVertexBuffer *vertex_buffer = &header->lightmap_vertex_buffers[index];
        DWORD raw = vertex_buffer->Data;
        if (!index)
            first_lightmap_raw = raw;
        vertex_buffer->Common = D3DCOMMON_TYPE_VERTEXBUFFER | 1;
        vertex_buffer->Data = vita_compiled_vertex_data_to_physical(raw, "BSP lightmap", index);
        IDirect3DVertexBuffer8_Register(vertex_buffer, (void *)halo_vita_memory_base());
    }

    vita_log("[VITA GPU] compiled BSP buffers registered: vertex=%d lightmap=%d first_vertex_xbox=%08lx first_lightmap_xbox=%08lx",
        (int)header->vertex_buffer_count, (int)header->lightmap_vertex_buffer_count,
        (unsigned long)first_vertex_raw, (unsigned long)first_lightmap_raw);
}

static boolean vita_direct_cache_open;
static uint32_t vita_direct_cache_tag_size;
static char vita_direct_cache_name[32];
static char vita_direct_cache_path[320];
static short vita_direct_cache_slot = NONE;

static boolean vita_direct_cache_header(
    const char *scenario_name,
    struct cache_file_header *header,
    char *path,
    size_t path_capacity)
{
    char map_file[48];
    const char *map_name;
    FILE *file;
    int name_terminated, build_terminated;

    if (!scenario_name || !header || !path || !path_capacity) {
        vita_log("[VITA MAP] source-map validation rejected invalid arguments: scenario=%p header=%p path=%p capacity=%lu",
            scenario_name, header, path, (unsigned long)path_capacity);
        return FALSE;
    }
    map_name = tag_name_strip_path(scenario_name);
    if (!map_name || !map_name[0] || csstrlen(map_name) >= 32) {
        vita_log("[VITA MAP] source-map validation rejected scenario name: %s",
            scenario_name ? scenario_name : "<null>");
        return FALSE;
    }
    if (_snprintf(map_file, sizeof(map_file), "%s.map", map_name) < 0) {
        vita_log("[VITA MAP] source-map filename formatting failed: scenario=%s name=%s",
            scenario_name, map_name);
        return FALSE;
    }
    map_file[sizeof(map_file) - 1] = 0;
    if (!vita_map_path(map_file, path, path_capacity)) {
        vita_log("[VITA MAP] source map not found: scenario=%s expected=%s root=" HALO_VITA_DATA_ROOT "maps/",
            scenario_name, map_file);
        return FALSE;
    }

    file = fopen(path, "rb");
    if (!file) {
        vita_log("[VITA MAP] source-map fopen failed: scenario=%s path=%s", scenario_name, path);
        return FALSE;
    }
    if (fread(header, 1, sizeof(*header), file) != sizeof(*header)) {
        fclose(file);
        vita_log("[VITA MAP] source-map short header: scenario=%s path=%s expected=%lu",
            scenario_name, path, (unsigned long)sizeof(*header));
        return FALSE;
    }
    fclose(file);

    name_terminated = memchr(header->name, 0, sizeof(header->name)) != NULL;
    build_terminated = memchr(header->build, 0, sizeof(header->build)) != NULL;
    if (header->header_signature != 'head' || header->footer_signature != 'foot' ||
        header->version != 5 || header->file_length < 0 ||
        header->file_length > 0x11600000 || header->tag_data_offset < 0x800 ||
        header->tag_data_size < 36 || (uint32_t)header->tag_data_size > HALO_VITA_TAG_CAPACITY ||
        header->tag_data_offset > header->file_length ||
        header->tag_data_size > header->file_length - header->tag_data_offset ||
        !name_terminated || !build_terminated) {
        vita_log("[VITA MAP] source-map header rejected: scenario=%s path=%s head=%08lx foot=%08lx version=%ld logical=%ld tag_off=%ld tag_bytes=%ld tag_cap=%lu name_term=%d build_term=%d header_name=%.*s build=%.*s",
            scenario_name, path,
            (unsigned long)header->header_signature,
            (unsigned long)header->footer_signature,
            header->version, header->file_length,
            header->tag_data_offset, header->tag_data_size,
            (unsigned long)HALO_VITA_TAG_CAPACITY,
            name_terminated, build_terminated,
            (int)sizeof(header->name), header->name,
            (int)sizeof(header->build), header->build);
        return FALSE;
    }
    if (_stricmp(header->name, map_name)) {
        vita_log("[VITA MAP] source identity mismatch requested=%s header=%s path=%s",
            map_name, header->name, path);
        return FALSE;
    }
    vita_log("[VITA MAP] source-map header PASS: scenario=%s path=%s name=%s build=%s logical=%ld checksum=%08lx tag_off=%ld tag_bytes=%ld type=%d",
        scenario_name, path, header->name, header->build,
        header->file_length, (unsigned long)header->checksum,
        header->tag_data_offset, header->tag_data_size, (int)header->scenario_type);
    return TRUE;
}

static int vita_cache_slot_range(short scenario_type, unsigned *first, unsigned *last)
{
    switch (scenario_type) {
    case _scenario_type_solo:
        *first = 0; *last = 1; return 1;
    case _scenario_type_multiplayer:
        *first = 3; *last = 5; return 1;
    case _scenario_type_main_menu:
        *first = 2; *last = 2; return 1;
    default:
        return 0;
    }
}

static boolean vita_find_prepared_slot(const char *source_path, short scenario_type,
    char *cache_path, size_t cache_path_capacity, short *slot_index)
{
    unsigned first, last, slot;
    char error[160];
    if (!vita_cache_slot_range(scenario_type, &first, &last))
        return FALSE;
    for (slot = first; slot <= last; ++slot) {
        error[0] = 0;
        if (vita_cache_slot_valid(source_path, slot, cache_path, cache_path_capacity,
            error, sizeof(error))) {
            if (slot_index) *slot_index = (short)slot;
            return TRUE;
        }
    }
    return FALSE;
}

static short vita_choose_cache_slot(short scenario_type)
{
    unsigned first, last, slot;
    if (!vita_cache_slot_range(scenario_type, &first, &last))
        return NONE;
    /* Preserve the retail reason for two solo/three multiplayer slots: never
     * overwrite the slot that cache_file_open currently owns. */
    for (slot = first; slot <= last; ++slot)
        if (!vita_direct_cache_open || vita_direct_cache_slot != (short)slot)
            return (short)slot;
    return NONE;
}

size_t halo_vita_cache_direct_tag_size(void)
{
    return vita_direct_cache_open ? (size_t)vita_direct_cache_tag_size : 0;
}

void cache_files_initialize(void)
{
    char ui_path[320], legacy[360];
    vita_direct_cache_open = FALSE;
    vita_direct_cache_tag_size = 0;
    vita_direct_cache_name[0] = 0;
    vita_direct_cache_path[0] = 0;
    vita_direct_cache_slot = NONE;
    /* Retire scratch names used by builds before the upstream cache audit.
     * They are never considered map/cache candidates by this implementation. */
    remove(HALO_VITA_DATA_ROOT "cache0.vita-logical.tmp");
    if (vita_map_path("ui.map", ui_path, sizeof(ui_path)) &&
        _snprintf(legacy, sizeof(legacy), "%s.vita-logical.tmp", ui_path) > 0)
        remove(legacy);
    vita_log("[VITA MAP] cache API preserves retail layout: maps/ source + persistent cache000..005.map; decompression occurs only during precache");
}

void cache_files_dispose(void)
{
    if (vita_direct_cache_open)
        cache_file_close();
}

void cache_files_precache_set_priority(boolean blocking)
{
    /* The Vita adapter performs the copy synchronously, so there is no worker
     * priority to change. Storage ownership and commit ordering remain retail. */
    (void)blocking;
}

boolean cache_files_precache_in_progress(void)
{
    return FALSE;
}

boolean cache_files_precache_is_copying_map(const char *map_name)
{
    (void)map_name;
    return FALSE;
}

boolean cache_files_precache_map_loaded(const char *map_name)
{
    struct cache_file_header header;
    char source_path[320], cache_path[320];
    return vita_direct_cache_header(map_name, &header, source_path, sizeof(source_path)) &&
        vita_find_prepared_slot(source_path, header.scenario_type,
            cache_path, sizeof(cache_path), NULL);
}

boolean cache_files_precache_map_begin(const char *map_name, boolean copy_map)
{
    struct cache_file_header header;
    char source_path[320], cache_path[320], error[160] = {0};
    short slot;
    int reused = 0;
    (void)copy_map;

    if (!vita_direct_cache_header(map_name, &header, source_path, sizeof(source_path))) {
        vita_log("[VITA MAP] precache source missing/invalid: %s", map_name ? map_name : "<null>");
        return FALSE;
    }
    if (vita_find_prepared_slot(source_path, header.scenario_type,
        cache_path, sizeof(cache_path), &slot)) {
        vita_log("[VITA MAP] precache cache%03d already valid for %s checksum=%08lx",
            (int)slot, header.name, (unsigned long)header.checksum);
        return TRUE;
    }
    slot = vita_choose_cache_slot(header.scenario_type);
    if (slot == NONE) {
        vita_log("[VITA MAP] precache has no free retail slot for %s type=%d",
            header.name, (int)header.scenario_type);
        return FALSE;
    }
    vita_log("[VITA MAP] precache begin: source=%s -> cache%03d.map logical=%ld checksum=%08lx",
        source_path, (int)slot, header.file_length, (unsigned long)header.checksum);
    if (!vita_cache_prepare_slot(source_path, (unsigned)slot,
        cache_path, sizeof(cache_path), &reused, NULL, NULL, error, sizeof(error))) {
        vita_log("[VITA MAP] precache cache%03d FAILED for %s: %s",
            (int)slot, header.name, error);
        return FALSE;
    }
    vita_log("[VITA MAP] precache cache%03d %s: map=%s path=%s header committed after payload",
        (int)slot, reused ? "REUSED" : "COMMITTED", header.name, cache_path);
    return TRUE;
}

short cache_files_precache_map_status(real *progress)
{
    if (progress)
        *progress = 1.0f;
    return _cached_map_file_success;
}

void cache_files_precache_map_end(void)
{
    /* Synchronous Vita copy has already waited for payload flush and committed
     * the header before cache_files_precache_map_begin returns. */
}

void cache_files_precache_map_queue_end(void)
{
}

boolean cache_file_open(const char *scenario_name, struct cache_file_header *header)
{
    struct cache_file_header candidate;
    char source_path[320], cache_path[320];
    const char *map_name;
    short slot = NONE;

    match_assert("c:\\halo\\SOURCE\\cache\\cache_files_windows.c", 223, !vita_direct_cache_open);
    if (!vita_direct_cache_header(scenario_name, &candidate, source_path, sizeof(source_path))) {
        vita_log("[VITA MAP] cache_file_open source validation failed: %s",
            scenario_name ? scenario_name : "<null>");
        return FALSE;
    }
    if (!vita_find_prepared_slot(source_path, candidate.scenario_type,
        cache_path, sizeof(cache_path), &slot)) {
        vita_log("[VITA MAP] cache_file_open rejected unprecached map: %s; source=%s",
            scenario_name, source_path);
        return FALSE;
    }
    if (!vita_cache_resource_bind(cache_path, (uint32_t)candidate.file_length)) {
        vita_log("[VITA MAP] cache_file_open cache%03d bind failed: %s: %s",
            (int)slot, cache_path, vita_cache_resource_error());
        return FALSE;
    }

    *header = candidate;
    /* Our imported cache_files.c predates upstream's current native-build
     * acceptance and still checks 01.01.14.2342 unless HALO_LINUX. Keep this
     * compatibility view in RAM only. cacheNNN.map retains the exact retail
     * source header (e.g. 01.10.12.2276) byte-for-byte. */
    if (csstrcmp(header->build, "01.01.14.2342")) {
        vita_log("[VITA MAP] retail cache header kept on disk build=%s; imported engine compatibility view=01.01.14.2342",
            header->build);
        csstrncpy(header->build, "01.01.14.2342", sizeof(header->build) - 1);
        header->build[sizeof(header->build) - 1] = 0;
    }
    map_name = tag_name_strip_path(scenario_name);
    csstrncpy(vita_direct_cache_name, map_name, sizeof(vita_direct_cache_name) - 1);
    vita_direct_cache_name[sizeof(vita_direct_cache_name) - 1] = 0;
    csstrncpy(vita_direct_cache_path, cache_path, sizeof(vita_direct_cache_path) - 1);
    vita_direct_cache_path[sizeof(vita_direct_cache_path) - 1] = 0;
    vita_direct_cache_tag_size = (uint32_t)candidate.tag_data_size;
    vita_direct_cache_slot = slot;
    vita_direct_cache_open = TRUE;
    vita_log("[VITA MAP] original cache_file_open slot PASS: name=%s cache%03d=%s logical_bytes=%ld tag_bytes=%u type=%d",
        vita_direct_cache_name, (int)slot, vita_direct_cache_path, candidate.file_length,
        vita_direct_cache_tag_size, (int)candidate.scenario_type);
    return TRUE;
}

void cache_file_close(void)
{
    if (!vita_direct_cache_open)
        return;
    vita_log("[VITA MAP] original cache_file_close: name=%s cache%03d retained on disk",
        vita_direct_cache_name, (int)vita_direct_cache_slot);
    vita_cache_resource_unbind();
    vita_direct_cache_open = FALSE;
    vita_direct_cache_tag_size = 0;
    vita_direct_cache_name[0] = 0;
    vita_direct_cache_path[0] = 0;
    vita_direct_cache_slot = NONE;
}
int halo_vita_cache_original_range_valid(uint32_t offset, size_t bytes)
{ return vita_cache_resource_range_valid(offset, bytes); }
