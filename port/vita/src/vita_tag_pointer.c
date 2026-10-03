/* Resolve serialized Xbox pointers only at Halo's original tag accessor
 * boundary. The map/tag structures remain the source of truth; this file does
 * not walk or rewrite arbitrary words and it does not define a parallel tag
 * system.
 *
 * The direct-map backend is the lifetime gate. The initial ui.map bring-up uses
 * a different validated/manual mount, so its already-relocated menu pointers
 * pass through untouched. During original scenario_tags_load the top-level
 * scenario/BSP fields are relocated eagerly by vita_cache_bridge.c. Nested
 * tag_block/tag_data/tag_reference fields remain serialized until an original
 * accessor asks for them; then we translate their validated Xbox arena span to
 * the corresponding address in the Vita arena.
 */
#include "cseries/cseries.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "halo_vita_cache.h"
#include "halo_vita_memory.h"
#include "vita_runtime.h"

#define VITA_SBSP_HEADER_SIGNATURE 0x73627370u

static int vita_span_contains(uintptr_t base, size_t length,
	uintptr_t value, size_t bytes)
{
	size_t offset;
	if (!base || value < base)
		return 0;
	offset = (size_t)(value - base);
	return offset <= length && bytes <= length - offset;
}

static int vita_owner_in_span(const void *owner, uintptr_t base, size_t length)
{
	return owner && vita_span_contains(base, length, (uintptr_t)owner, 1u);
}

static int vita_scenario_bsp_reference_count(struct scenario *scenario)
{
	if (!scenario || scenario->structure_bsp_references.count < 0 ||
		scenario->structure_bsp_references.count > MAXIMUM_STRUCTURE_BSPS_PER_SCENARIO ||
		(scenario->structure_bsp_references.count &&
			!scenario->structure_bsp_references.address))
		return 0;
	return (int)scenario->structure_bsp_references.count;
}

static int vita_owner_was_eagerly_rebased(const void *owner,
	struct scenario *scenario)
{
	int count, i;
	struct scenario_structure_bsp_reference *references;

	/* vita_cache_bridge.c eagerly relocates every tag_block/tag_data/reference
	 * that is physically part of the scenario root itself. Do not reinterpret
	 * those native values as serialized addresses. */
	if (scenario && vita_owner_in_span(owner, (uintptr_t)scenario, sizeof(*scenario)))
		return 1;

	count = vita_scenario_bsp_reference_count(scenario);
	if (!count)
		return 0;
	references = scenario->structure_bsp_references.address;
	for (i = 0; i < count; ++i) {
		unsigned char *payload;
		uintptr_t root;

		/* The structure_bsp reference name is also explicitly relocated before
		 * scenario_structure_bsp_load consumes it. */
		if (owner == &references[i].structure_bsp)
			return 1;

		/* Once a BSP payload has been read, its 24-byte cache header contains the
		 * native root pointer written by vita_cache_bridge.c. Every block/data
		 * field physically inside that root was eagerly relocated as well. */
		if (!references[i].base_address || references[i].file_size < 24)
			continue;
		payload = references[i].base_address;
		if (*(uint32_t *)(payload + 20) != VITA_SBSP_HEADER_SIGNATURE)
			continue;
		root = (uintptr_t)*(void **)payload;
		if (vita_span_contains((uintptr_t)payload, (size_t)references[i].file_size,
			root, sizeof(struct structure_bsp)) &&
			vita_owner_in_span(owner, root, sizeof(struct structure_bsp)))
			return 1;
	}
	return 0;
}

void *halo_vita_cache_resolve_compiled_pointer(const void *owner,
	const void *serialized_pointer, size_t bytes)
{
	size_t tag_size = halo_vita_cache_direct_tag_size();
	uintptr_t arena_base, tag_native, raw;
	size_t span = bytes ? bytes : 1u;
	struct scenario *scenario;
	int owner_is_compiled = 0;
	int count = 0, i;
	struct scenario_structure_bsp_reference *references = NULL;

	if (!serialized_pointer || !owner || !tag_size)
		return (void *)serialized_pointer;

	arena_base = halo_vita_memory_base();
	tag_native = halo_vita_memory_address(HALO_XBOX_TAG_BASE);
	if (!arena_base || !tag_native || tag_size > HALO_VITA_TAG_CAPACITY)
		vita_fatal("compiled cache pointer resolver has invalid Vita arena/tag span");

	scenario = global_scenario;
	if (vita_owner_in_span(owner, tag_native, tag_size))
		owner_is_compiled = 1;

	count = vita_scenario_bsp_reference_count(scenario);
	if (count) {
		references = scenario->structure_bsp_references.address;
		for (i = 0; i < count; ++i) {
			if (references[i].file_size > 0 && references[i].base_address &&
				vita_owner_in_span(owner, (uintptr_t)references[i].base_address,
					(size_t)references[i].file_size)) {
				owner_is_compiled = 1;
				break;
			}
		}
	}

	/* Heap/runtime tag blocks are already ordinary native pointers and retain
	 * original behavior. Only owners physically inside the active compiled map
	 * participate in Xbox-address translation. */
	if (!owner_is_compiled)
		return (void *)serialized_pointer;

	if (vita_owner_was_eagerly_rebased(owner, scenario))
		return (void *)serialized_pointer;

	raw = (uintptr_t)serialized_pointer;
	if (vita_span_contains(HALO_XBOX_TAG_BASE, tag_size, raw, span))
		return (void *)(tag_native + (raw - HALO_XBOX_TAG_BASE));

	for (i = 0; i < count; ++i) {
		uintptr_t native_base = (uintptr_t)references[i].base_address;
		uintptr_t xbox_base;
		if (!native_base || references[i].file_size <= 0 || native_base < arena_base ||
			native_base - arena_base >= HALO_VITA_ARENA_SIZE)
			continue;
		xbox_base = HALO_XBOX_MEMORY_BASE + (native_base - arena_base);
		if (vita_span_contains(xbox_base, (size_t)references[i].file_size, raw, span))
			return (void *)(native_base + (raw - xbox_base));
	}

	vita_log("[VITA TAGPTR] compiled pointer rejected owner=%p raw=%08lx bytes=%lu tag_bytes=%lu BSPs=%d",
		owner, (unsigned long)raw, (unsigned long)span, (unsigned long)tag_size, count);
	vita_fatal("compiled tag accessor pointer is outside active tag/BSP images");
	return NULL;
}
