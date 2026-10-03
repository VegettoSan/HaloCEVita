#include "vita_runtime.h"
#include "halo_vita_cache.h"
#include "halo_vita_memory.h"
#include <string.h>
#include <errno.h>

struct progress_state {
	int graphics, core, shaders, cancelled;
	uint64_t previous_frame, previous_log;
	const char *name;
};
static int cache_progress(uint32_t position, void *context)
{
	struct progress_state *state = context;
	uint64_t now = vita_time_us();
	if (vita_controls_poll() < 0) { state->cancelled = 1; return 0; }
	if (now - state->previous_frame >= 100000) {
		if (state->graphics) vita_graphics_frame(1, state->core, state->shaders);
		state->previous_frame = now;
	}
	if (now - state->previous_log >= 1000000) {
		vita_log("cache %s precache/logical position=%u", state->name, position);
		state->previous_log = now;
	}
	return 1;
}
int vita_cache_probe(int graphics, int core, int shaders)
{
	const char *names[1] = { "ui.map" };
	int i, passed = 0;
	void *tags = (void *)halo_vita_memory_address(HALO_XBOX_TAG_BASE);
	if (!tags) return 0;
	/* A recheck replaces the same arena bytes. Restore the old image and its
	 * resource-file binding first. */
#ifndef HALO_VITA_ORIGINAL_RUNTIME
	if (!halo_vita_ui_runtime_dispose()) return -1;
#endif
	if (!halo_vita_cache_unmount_menu()) return 0;
	vita_cache_resource_unbind();
	vita_graphics_cache_status(-1);
	for (i = 0; i < 1; ++i) {
		char source_path[320], cache_path[320], error[160] = {0};
		FILE *file;
		struct vita_cache_info info;
		struct progress_state state = { graphics, core, shaders, 0, 0, 0, names[i] };
		uint64_t start = vita_time_us();
		int reused = 0;
		if (!vita_map_path(names[i], source_path, sizeof(source_path))) {
			vita_log("cache %s missing", names[i]);
			continue;
		}
		vita_log("[VITA 015] cache %s upstream-style precache begin; source=%s slot=cache002.map header commits last",
			names[i], source_path);
		/* Xbox reserves slot 2 for _scenario_type_main_menu. Build/reuse the
		 * committed logical cache before any tag or resource consumer sees it. */
		if (!vita_cache_prepare_slot(source_path, 2, cache_path, sizeof(cache_path), &reused,
			cache_progress, &state, error, sizeof(error))) {
			vita_log("MAIN MENU BLOCKED: cache002.map prepare failed: %s", error);
			if (state.cancelled) {
				vita_graphics_cache_status(0);
				return -1;
			}
			continue;
		}
		vita_log("[VITA CACHE] main-menu cache slot %s: source=%s slot=%s",
			reused ? "REUSED" : "REBUILT", source_path, cache_path);
		file = fopen(cache_path, "rb");
		if (!file) {
			vita_log("cache %s committed slot open failed: %s", names[i], strerror(errno));
			continue;
		}
		if (vita_cache_read(file, tags, HALO_VITA_TAG_CAPACITY, &info,
			cache_progress, &state, error, sizeof(error))) {
			vita_log("cache %s loaded from committed slot bytes=%u compressed=%d tags=%u vertex_buffers=%u index_buffers=%u tag_crc32=%08x time_us=%llu",
				names[i], info.tag_size, info.compressed, info.tag_count, info.vertices, info.indices, info.tag_crc,
				(unsigned long long)(vita_time_us() - start));
			if (info.compressed) {
				vita_log("MAIN MENU BLOCKED: cache002.map unexpectedly remained compressed");
			} else if (!vita_cache_resource_bind(cache_path, info.logical_size)) {
				vita_log("MAIN MENU BLOCKED: could not bind committed cache002.map: %s", vita_cache_resource_error());
			} else if (halo_vita_cache_mount_menu(tags, info.tag_size)) {
				vita_log("cache %s tag/resource consumers share committed cache002.map logical_size=%u",
					names[i], info.logical_size);
				if (halo_vita_cache_validate_menu() && halo_vita_ui_runtime_initialize()) passed++;
				else {
					vita_log("MAIN MENU BLOCKED: original tag/accessor validation failed");
					if (!halo_vita_ui_runtime_dispose()) { fclose(file); return -1; }
					halo_vita_cache_unmount_menu();
					vita_cache_resource_unbind();
				}
			} else {
				vita_cache_resource_unbind();
			}
		} else vita_log("cache %s FAILED from committed slot: %s", names[i], error);
		fclose(file);
		if (state.cancelled) {
			if (!halo_vita_ui_runtime_dispose()) return -1;
			halo_vita_cache_unmount_menu();
			vita_cache_resource_unbind();
			vita_graphics_cache_status(0); return -1;
		}
	}
	if (passed != 1) vita_cache_resource_unbind();
	vita_log("[VITA 016] real UI menu-tag checkpoint=%d/1; persistent mount=%s; upstream-style cache002 backend=%s; root/events/BSP/GPU not activated",
		passed, passed ? "YES" : "NO", passed ? "BOUND" : "NO");
	vita_graphics_cache_status(passed == 1);
	return passed == 1;
}