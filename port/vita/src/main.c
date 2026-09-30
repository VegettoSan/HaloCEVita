#include "vita_runtime.h"
#include "halo_vita_memory.h"
#include "halo_vita_cache.h"
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <stdlib.h>

int main(void)
{
	int platform, core, maps, graphics, shaders = 0, command, arena = 0, memory = 0, menu_cache = 0;
	char *vertex, *fragment;
	uint32_t user, cdram, phycont;
	platform = vita_platform_initialize();
	vita_free_memory(&user, &cdram, &phycont);
	vita_log("free memory user=%u cdram=%u phycont=%u; newlib heap cap=64MiB", user, cdram, phycont);
	vita_log("[VITA 007] Halo memory init begin");
	core = halo_vita_core_initialize();
	vita_log("Halo core result=%s", core ? "PASS" : "FAIL");
	maps = vita_maps_verify();
	vita_services_probe();
	vita_log("[VITA 005] graphics init begin");
	graphics = vita_graphics_initialize();
	if (graphics) {
		vita_log("[VITA 006] vitaGL ready");
		/* Replace vitaGL's splash before any GPU-copy/compiler experiment.
		 * A later failure is then localized by the next begin/result log. */
		vita_log("initial diagnostic frame begin");
		vita_graphics_frame(maps, core, -1);
		vita_log("initial diagnostic frame returned; GPU framebuffer/blit probe begin");
		vita_graphics_copy_probe();
		vita_log("NV2A shader generation begin");
		vertex = halo_vita_vertex_shader(); fragment = halo_vita_pixel_shader();
		vita_log("NV2A shader generation returned vertex=%s pixel=%s; runtime compile begin",
			vertex ? "OK" : "NULL", fragment ? "OK" : "NULL");
		if (vertex && fragment) shaders = vita_graphics_shader_probe(vertex, fragment);
		vita_log("NV2A runtime compile returned result=%d", shaders);
		free(vertex); free(fragment);
	}
	vita_log("[VITA 011] native Halo core probe complete; platform=%d core=%d maps=%d graphics=%d shaders=%d", platform, core, maps, graphics, shaders);
	arena = vita_memory_initialize();
	if (arena) memory = halo_vita_memory_initialize();
	vita_free_memory(&user, &cdram, &phycont);
	vita_log("engine memory checkpoint=%d; free user=%u cdram=%u phycont=%u", memory, user, cdram, phycont);
	if (!memory || !maps) {
		vita_graphics_cache_status(0);
		vita_log("cache checkpoint skipped: memory=%d maps=%d", memory, maps);
	}
	if (memory && maps) menu_cache = vita_cache_probe(graphics, core, shaders);
	if (menu_cache < 0) goto cleanup;
	vita_log("Full Halo main NOT ENTERED (milestone 010 withheld): UI cache/runtime-init checkpoint=%d; Main Menu root/events, scenario/BSP/resources and original renderer pending", menu_cache);
	/* Keep the process available even without maps or runtime shader compiler. */
	for (;;) {
		uint64_t begin = vita_time_us(), elapsed;
		command = vita_controls_poll();
		if (command < 0) break;
		if (command > 0) {
			maps = vita_maps_verify();
			if (memory && maps && vita_cache_probe(graphics, core, shaders) < 0) break;
		}
		if (graphics) vita_graphics_frame(maps, core, shaders);
		elapsed = vita_time_us() - begin;
		if (elapsed < 33333) sceKernelDelayThread((unsigned)(33333 - elapsed));
	}
cleanup:
	if (halo_vita_ui_runtime_dispose()) {
		if (!halo_vita_cache_unmount_menu()) vita_log("UI cache restoration failed during shutdown");
	} else vita_log("UI cache retained during shutdown because widget disposal failed");
	if (memory || arena) halo_vita_memory_dispose();
	if (arena) vita_memory_shutdown();
	if (graphics) vita_graphics_shutdown();
	if (core) halo_vita_core_dispose();
	vita_platform_shutdown();
	sceKernelExitProcess(0);
	return 0;
}
