/* Halo owns precache, slot policy, handles, requests and resource completion.
 * Only Xbox GPU-address registration is replaced for Vita's movable arena.
 * Recovery builds retain the old manager in halo_cache_windows_recovery.c. */
#include "vita_runtime.h"
#include "halo_vita_cache.h"
#include "halo_vita_memory.h"
#define tags_header_register_vertex_and_index_buffers halo_vita_xbox_tags_register
#define structure_bsp_header_register_vertex_buffers halo_vita_xbox_bsp_register
#include "../../../source/cache/cache_files_windows.c"
#undef tags_header_register_vertex_and_index_buffers
#undef structure_bsp_header_register_vertex_buffers

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


size_t halo_vita_cache_direct_tag_size(void)
{
    short index = cache_file_globals.open_map_file_index;
    return index == NONE ? 0 : (size_t)cached_map_file_get(index)->header.tag_data_size;
}

int halo_vita_cache_original_range_valid(uint32_t offset, size_t bytes)
{
    short index = cache_file_globals.open_map_file_index;
    long length;
    if (index == NONE) return 0;
    length = cached_map_file_get(index)->header.file_length;
    return length >= 0 && offset <= (uint32_t)length && bytes <= (uint32_t)length - offset;
}
