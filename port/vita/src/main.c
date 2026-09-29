#include "vita_runtime.h"
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <stdlib.h>

int main(void)
{
	int platform, core, maps, graphics, shaders = 0, command;
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
		vita_graphics_copy_probe();
		vertex = halo_vita_vertex_shader(); fragment = halo_vita_pixel_shader();
		if (vertex && fragment) shaders = vita_graphics_shader_probe(vertex, fragment);
		free(vertex); free(fragment);
	}
	vita_log("[VITA 011] native Halo core probe complete; platform=%d core=%d maps=%d graphics=%d shaders=%d", platform, core, maps, graphics, shaders);
	vita_log("Full Halo main NOT ENTERED (milestone 010 withheld): physical-memory/renderer integration blocked");
	/* Keep the process available even without maps or runtime shader compiler. */
	for (;;) {
		uint64_t begin = vita_time_us(), elapsed;
		command = vita_controls_poll();
		if (command < 0) break;
		if (command > 0) maps = vita_maps_verify();
		if (graphics) vita_graphics_frame(maps, core, shaders);
		elapsed = vita_time_us() - begin;
		if (elapsed < 33333) sceKernelDelayThread((unsigned)(33333 - elapsed));
	}
	if (graphics) vita_graphics_shutdown();
	if (core) halo_vita_core_dispose();
	vita_platform_shutdown();
	sceKernelExitProcess(0);
	return 0;
}
