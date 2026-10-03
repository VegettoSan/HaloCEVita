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
		vita_log("cache %s streaming logical position=%u (tags only retained)", state->name, position);
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
		char path[320], error[160] = {0};
		FILE *file;
		struct vita_cache_info info;
		struct progress_state state = { graphics, core, shaders, 0, 0, 0, names[i] };
		uint64_t start = vita_time_us();
		if (!vita_map_path(names[i], path, sizeof(path))) { vita_log("cache %s missing", names[i]); continue; }
		vita_log("[VITA 015] cache %s tag read begin; capacity22MiB, inflate scratch64KiB, maps unchanged", names[i]);
		file = fopen(path, "rb");
		if (!file) { vita_log("cache %s open failed: %s", names[i], strerror(errno)); continue; }
		if (vita_cache_read(file, tags, HALO_VITA_TAG_CAPACITY, &info, cache_progress, &state, error, sizeof(error))) {
			vita_log("cache %s loaded bytes=%u compressed=%d tags=%u vertex_buffers=%u index_buffers=%u tag_crc32=%08x time_us=%llu",
				names[i], info.tag_size, info.compressed, info.tag_count, info.vertices, info.indices, info.tag_crc,
				(unsigned long long)(vita_time_us() - start));
			if (!vita_cache_resource_bind(path, info.logical_size)) {
				vita_log("MAIN MENU BLOCKED: could not bind validated %s to logical resource reader: %s", names[i], vita_cache_resource_error());
			} else if (halo_vita_cache_mount_menu(tags, info.tag_size)) {
				vita_log("cache %s resource reader bound: logical_size=%u seekable logical resource backend ready",
					names[i], info.logical_size);
				if (halo_vita_cache_validate_menu() && halo_vita_ui_runtime_initialize()) passed++;
				else {
					vita_log("MAIN MENU BLOCKED: original tag/accessor validation failed");
					if (!halo_vita_ui_runtime_dispose()) return -1;
					halo_vita_cache_unmount_menu();
					vita_cache_resource_unbind();
				}
			} else {
				vita_cache_resource_unbind();
			}
		} else vita_log("cache %s FAILED: %s", names[i], error);
		fclose(file);
		if (state.cancelled) {
			if (!halo_vita_ui_runtime_dispose()) return -1;
			halo_vita_cache_unmount_menu();
			vita_cache_resource_unbind();
			vita_graphics_cache_status(0); return -1;
		}
	}
	if (passed != 1) vita_cache_resource_unbind();
	vita_log("[VITA 016] real UI menu-tag checkpoint=%d/1; persistent mount=%s; resource logical-range backend=%s; root/events/BSP/GPU not activated",
		passed, passed ? "YES" : "NO", passed ? "BOUND" : "NO");
	vita_graphics_cache_status(passed == 1);
	return passed == 1;
}
