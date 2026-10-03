/*
TAG_GROUPS.C
*/

/* ---------- headers */

#include "cseries.h"
#include "tag_files.h"
#include "byte_swapping.h"
#include "tag_groups.h"
#ifdef HALO_VITA
#include "halo_vita_cache.h"
#endif

/* ---------- public code */

long verify_tag_reference(
	const struct tag_reference *reference)
{
	long index;
	const char *name;

	match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3055, reference);
	name = reference->name;
#ifdef HALO_VITA
	if (name)
		name = halo_vita_cache_resolve_compiled_pointer(
			reference,
			name,
			reference->name_length > 0 ? (size_t)reference->name_length + 1u : 1u);
#endif
	index = tag_loaded(reference->group_tag, name);
	
	match_vassert(
		"c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3061, reference->index==index,
		csprintf(temporary,
			"tag reference \"%s\" and actual index do not match: is %08lX but should be %08lX",
			name,
			reference->index,
			index));

	return index;
}

void* tag_data_get_pointer(
	const struct tag_data *data,
	long offset, 
	long size) 
{
	void *address;

	match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3073, size>=0);
	match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3074, offset>=0 && offset+size<=data->size);
	address = data->address;
#ifdef HALO_VITA
	if (address)
		address = halo_vita_cache_resolve_compiled_pointer(
			data, address, (size_t)offset + (size_t)size);
#endif

	return (void *)((byte *)address + offset);
}

void *tag_block_get_element_with_size(
	const struct tag_block *block,
	long index, 
	long element_size) 
{
	void *address;

	match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3084, block);
	match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3085, block->count>=0);
	match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3086, !block->definition || block->definition->element_size==element_size);

	match_vassert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3089, index>=0 && index<block->count,
		csprintf(temporary,
			"#%d is not a valid %s index in [#0,#%d)",
			index,
			block->definition ? block->definition->name : "<unknown>", block->count));
	match_assert("c:\\halo\\SOURCE\\tag_files\\tag_groups.c", 3090, block->address);
	address = block->address;
#ifdef HALO_VITA
	address = halo_vita_cache_resolve_compiled_pointer(
		block,
		address,
		element_size > 0 ? ((size_t)index + 1u) * (size_t)element_size : 1u);
#endif

	return (void *)((byte *)address + (index * element_size));
}