/* Vita keeps Halo's cache-file API and original scenario/game callers, but the
 * Xbox HDD cache implementation is not a valid storage backend here. Retail
 * copies DVD maps into fixed z:\\cacheNNN.map slots before scenario_tags_load;
 * Vita user maps can already be compressed Xbox-v5 files and are exposed as a
 * checked logical stream by vita_cache_read.c.
 *
 * Compile the original unit so its GPU registration helpers and implementation
 * remain available for comparison, but rename the retail cache lifecycle and
 * request entry points. Public callers below keep the exact Halo API while
 * adapting only the storage boundary to ux0:data/HaloCE/maps/<name>.map.
 */
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

static boolean vita_direct_cache_header(
    const char *scenario_name,
    struct cache_file_header *header,
    char *path,
    size_t path_capacity)
{
    char map_file[48];
    const char *map_name;
    FILE *file;

    if (!scenario_name || !header || !path || !path_capacity)
        return FALSE;
    map_name = tag_name_strip_path(scenario_name);
    if (!map_name || !map_name[0] || csstrlen(map_name) >= 32)
        return FALSE;
    if (_snprintf(map_file, sizeof(map_file), "%s.map", map_name) < 0)
        return FALSE;
    map_file[sizeof(map_file) - 1] = 0;
    if (!vita_map_path(map_file, path, path_capacity))
        return FALSE;

    file = fopen(path, "rb");
    if (!file)
        return FALSE;
    if (fread(header, 1, sizeof(*header), file) != sizeof(*header)) {
        fclose(file);
        return FALSE;
    }
    fclose(file);

    /* The original Xbox executable rejects cache files whose build string is
     * not its January beta identifier. Native ports intentionally accept other
     * Xbox-v5 builds after checking the actual format contract. The user's
     * retail maps are 01.10.12.2276, so do not reject them merely for that
     * informational string; keep all structural/version/range checks. */
    if (header->header_signature != 'head' || header->footer_signature != 'foot' ||
        header->version != 5 || header->file_length < 0 ||
        header->file_length > 0x11600000 || header->tag_data_offset < 0x800 ||
        header->tag_data_size < 36 || (uint32_t)header->tag_data_size > HALO_VITA_TAG_CAPACITY ||
        header->tag_data_offset > header->file_length ||
        header->tag_data_size > header->file_length - header->tag_data_offset ||
        !memchr(header->name, 0, sizeof(header->name)) ||
        !memchr(header->build, 0, sizeof(header->build)))
        return FALSE;
    if (_stricmp(header->name, map_name)) {
        vita_log("[VITA MAP] cache identity mismatch requested=%s header=%s path=%s",
            map_name, header->name, path);
        return FALSE;
    }
    return TRUE;
}

size_t halo_vita_cache_direct_tag_size(void)
{
    return vita_direct_cache_open ? (size_t)vita_direct_cache_tag_size : 0;
}

void cache_files_initialize(void)
{
    vita_direct_cache_open = FALSE;
    vita_direct_cache_tag_size = 0;
    vita_direct_cache_name[0] = 0;
    vita_direct_cache_path[0] = 0;
    vita_log("[VITA MAP] original cache API uses direct logical-map backend; Xbox z:\\cacheNNN.map disabled");
}

void cache_files_dispose(void)
{
    if (vita_direct_cache_open)
        cache_file_close();
}

void cache_files_precache_set_priority(boolean blocking)
{
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
    char path[320];
    return vita_direct_cache_header(map_name, &header, path, sizeof(path));
}

boolean cache_files_precache_map_begin(const char *map_name, boolean copy_map)
{
    struct cache_file_header header;
    char path[320];
    boolean available = vita_direct_cache_header(map_name, &header, path, sizeof(path));
    (void)copy_map;
    if (available)
        vita_log("[VITA MAP] original precache request satisfied by direct map: %s logical_bytes=%ld",
            path, header.file_length);
    else
        vita_log("[VITA MAP] original precache request missing/invalid: %s", map_name ? map_name : "<null>");
    return available;
}

short cache_files_precache_map_status(real *progress)
{
    if (progress)
        *progress = 1.0f;
    return _cached_map_file_success;
}

void cache_files_precache_map_end(void)
{
}

void cache_files_precache_map_queue_end(void)
{
}

boolean cache_file_open(const char *scenario_name, struct cache_file_header *header)
{
    struct cache_file_header candidate;
    char path[320];
    const char *map_name;

    match_assert("c:\\halo\\SOURCE\\cache\\cache_files_windows.c", 223, !vita_direct_cache_open);
    if (!vita_direct_cache_header(scenario_name, &candidate, path, sizeof(path))) {
        vita_log("[VITA MAP] cache_file_open failed validation: %s", scenario_name ? scenario_name : "<null>");
        return FALSE;
    }
    if (!vita_cache_resource_bind(path, (uint32_t)candidate.file_length)) {
        vita_log("[VITA MAP] cache_file_open logical bind failed: %s: %s", path, vita_cache_resource_error());
        return FALSE;
    }

    *header = candidate;
    /* cache_files.c retains the original executable's build-string assertion.
     * Normalize only this in-memory compatibility field after the Vita boundary
     * has already validated the real v5 header; source map bytes stay untouched. */
    if (csstrcmp(header->build, "01.01.14.2342")) {
        vita_log("[VITA MAP] native v5 build accepted: %s (compatibility view=01.01.14.2342)",
            header->build);
        csstrncpy(header->build, "01.01.14.2342", sizeof(header->build) - 1);
        header->build[sizeof(header->build) - 1] = 0;
    }
    map_name = tag_name_strip_path(scenario_name);
    csstrncpy(vita_direct_cache_name, map_name, sizeof(vita_direct_cache_name) - 1);
    vita_direct_cache_name[sizeof(vita_direct_cache_name) - 1] = 0;
    csstrncpy(vita_direct_cache_path, path, sizeof(vita_direct_cache_path) - 1);
    vita_direct_cache_path[sizeof(vita_direct_cache_path) - 1] = 0;
    vita_direct_cache_tag_size = (uint32_t)candidate.tag_data_size;
    vita_direct_cache_open = TRUE;
    vita_log("[VITA MAP] original cache_file_open direct PASS: name=%s path=%s logical_bytes=%ld tag_bytes=%u type=%d",
        vita_direct_cache_name, vita_direct_cache_path, candidate.file_length,
        vita_direct_cache_tag_size, (int)candidate.scenario_type);
    return TRUE;
}

void cache_file_close(void)
{
    if (!vita_direct_cache_open)
        return;
    vita_log("[VITA MAP] original cache_file_close direct: name=%s", vita_direct_cache_name);
    vita_cache_resource_unbind();
    vita_direct_cache_open = FALSE;
    vita_direct_cache_tag_size = 0;
    vita_direct_cache_name[0] = 0;
    vita_direct_cache_path[0] = 0;
}