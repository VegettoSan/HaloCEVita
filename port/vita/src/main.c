#include "vita_runtime.h"
#include "halo_vita_memory.h"
#include "halo_vita_cache.h"
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <stdlib.h>
#include <string.h>

#ifdef HALO_VITA_MENU_UPDATE_PROBE
int halo_vita_menu_update_checkpoint(void);
#endif

int main(void)
{
	int platform, core, maps, services, graphics, shaders = 0, command, arena = 0, memory = 0, menu_cache = 0;
	int root_active = 0, ui_disposed, renderer = 0;
	int menu_input_sample_valid = 0;
	char *vertex, *fragment;
	char menu_map_path[320];
	uint32_t user, cdram, phycont;
	struct vita_gamepad_sample previous_menu_pad = {0};
	platform = vita_platform_initialize();
	vita_free_memory(&user, &cdram, &phycont);
	vita_log("free memory user=%u cdram=%u phycont=%u; newlib heap cap=64MiB", user, cdram, phycont);
	vita_log("[VITA 007] Halo memory init begin");
	core = halo_vita_core_initialize();
	vita_log("Halo core result=%s", core ? "PASS" : "FAIL");
	#ifdef HALO_VITA_MENU_BRINGUP
	maps = vita_map_path("ui.map", menu_map_path, sizeof(menu_map_path));
	vita_log("[VITA 008] menu map path %s", maps ? menu_map_path : "MISSING");
	services = 0;
	#else
	maps = vita_maps_verify();
	services = vita_services_probe();
	#endif
#ifndef HALO_VITA_MENU_BRINGUP
	if (!services) vita_log("MAIN MENU BLOCKED: thread/last-error service contract failed");
	if (maps && services && !halo_vita_file_contract_probe())
		vita_log("MAIN MENU BLOCKED: original Xbox file-reference contract failed");
#endif
	vita_log("[VITA 005] graphics init begin");
	graphics = vita_graphics_initialize();
	if (!graphics) {
		vita_log("MAIN MENU BLOCKED: vitaGL context did not initialize; stopping before UI activation");
		goto cleanup;
	}
	if (graphics) {
		vita_log("[VITA 006] vitaGL ready");
#ifdef HALO_VITA_MENU_BRINGUP
		/* Present one neutral application frame so vitaGL's startup image cannot
		 * masquerade as an engine hang before the original rasterizer takes over. */
		if (!vita_graphics_handoff_frame()) {
			vita_log("MAIN MENU BLOCKED: failed to present the first application frame");
			goto cleanup;
		}
#else
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
#endif
	}
#ifdef HALO_VITA_MENU_BRINGUP
	vita_log("HALO_VITA_MENU_BRINGUP=1: proven GPU/file probes skipped");
#endif
	vita_log("[VITA 011] native Halo core probe complete; platform=%d core=%d maps=%d graphics=%d shaders=%d", platform, core, maps, graphics, shaders);
#ifdef HALO_VITA_ORIGINAL_RUNTIME
    if (!halo_vita_original_shell_initialize()) goto cleanup;
#endif
	arena = vita_memory_initialize();
	if (arena) memory = halo_vita_memory_initialize();
	vita_free_memory(&user, &cdram, &phycont);
	vita_log("engine memory checkpoint=%d; free user=%u cdram=%u phycont=%u", memory, user, cdram, phycont);
	if (!memory || !maps) {
		vita_graphics_cache_status(0);
		vita_log("cache checkpoint skipped: memory=%d maps=%d", memory, maps);
	}
#ifdef HALO_VITA_ORIGINAL_RUNTIME
    if (!memory || !maps || !halo_vita_original_game_initialize()) goto cleanup;
#endif
	#ifdef HALO_VITA_MENU_BRINGUP
	if (memory && maps) menu_cache = vita_cache_probe(0, core, 0);
	#else
	if (memory && maps) menu_cache = vita_cache_probe(graphics, core, shaders);
	#endif
	if (menu_cache < 0) goto cleanup;
#ifdef HALO_VITA_MENU_BRINGUP
#ifdef HALO_VITA_MENU_RENDER_PROBE
	if (menu_cache) renderer = halo_vita_renderer_initialize();
	if (!renderer) {
		vita_log("MAIN MENU BLOCKED: original rasterizer runtime did not initialize");
		goto cleanup;
	}
#endif
	if (menu_cache) root_active = halo_vita_menu_root_checkpoint();
	if (!root_active) {
		vita_log("MAIN MENU BLOCKED: original root creation did not complete");
		goto cleanup;
	}
	if (!halo_vita_ui_activate_main_menu_state()) {
		vita_log("MAIN MENU BLOCKED: original Main Menu state transition did not complete");
		goto cleanup;
	}
	vita_log("[VITA 032] original Main Menu root active; original renderer=%s",
		renderer ? "READY" : "DISABLED");
#if defined(HALO_VITA_MENU_UPDATE_PROBE) && !defined(HALO_VITA_ORIGINAL_RUNTIME)
	if (!halo_vita_menu_update_checkpoint()) goto cleanup;
#endif
#else
	vita_log("Full Halo main NOT ENTERED (milestone 010 withheld): UI cache/runtime-init checkpoint=%d; Main Menu root/events, scenario/BSP/resources and original renderer pending", menu_cache);
#endif
	/* Keep the process available and let the original renderer own visible menu
	 * frames once the root and D3D8/vitaGL bridge are ready. */
	for (;;) {
		uint64_t begin = vita_time_us(), elapsed;
		command = vita_controls_poll();
		if (command < 0) break;
#ifdef HALO_VITA_MENU_BRINGUP
#ifndef HALO_VITA_ORIGINAL_RUNTIME
		if (root_active) {
			struct vita_gamepad_sample menu_pad;
			if (vita_read_gamepad(&menu_pad)) {
				if (menu_input_sample_valid) {
					uint16_t pressed = menu_pad.buttons & (uint16_t)~previous_menu_pad.buttons;
					if (pressed & 0x0001) halo_vita_ui_process_menu_action(3);
					if (pressed & 0x0002) halo_vita_ui_process_menu_action(4);
					if (pressed & 0x0004) halo_vita_ui_process_menu_action(5);
					if (pressed & 0x0008) halo_vita_ui_process_menu_action(6);
					if (menu_pad.analog[0] && !previous_menu_pad.analog[0])
						halo_vita_ui_process_menu_action(1);
					if (menu_pad.analog[1] && !previous_menu_pad.analog[1])
						halo_vita_ui_process_menu_action(7);
					if (menu_pad.analog[2] && !previous_menu_pad.analog[2]) halo_vita_ui_process_menu_action(2);
					if (menu_pad.analog[3] && !previous_menu_pad.analog[3]) halo_vita_ui_process_menu_action(11);
					if (pressed & 0x0010) halo_vita_ui_process_menu_action(8);
					if (menu_pad.analog[6] && !previous_menu_pad.analog[6]) halo_vita_ui_process_menu_action(9);
					if (menu_pad.analog[7] && !previous_menu_pad.analog[7]) halo_vita_ui_process_menu_action(10);
				}
				previous_menu_pad = menu_pad;
				menu_input_sample_valid = 1;
			}
		}
#endif
#endif
#ifdef HALO_VITA_MENU_AUDIO
		/* Square now belongs to the original UI/keyboard, not the audio probe. */
#endif
		if (command == 1) {
#ifndef HALO_VITA_MENU_BRINGUP
			maps = vita_maps_verify();
			if (memory && maps && vita_cache_probe(graphics, core, shaders) < 0) break;
#endif
		}
#ifdef HALO_VITA_MENU_BRINGUP
#ifdef HALO_VITA_MENU_RENDER_PROBE
		if (root_active) halo_vita_ui_process_shell_frame();
		if (renderer && root_active && !halo_vita_renderer_render_menu_frame()) break;
#endif
#else
		if (graphics) vita_graphics_frame(maps, core, shaders);
#endif
		elapsed = vita_time_us() - begin;
		if (elapsed < 33333) sceKernelDelayThread((unsigned)(33333 - elapsed));
	}
cleanup:
#ifdef HALO_VITA_MENU_AUDIO
	halo_vita_menu_audio_dispose();
#endif
#ifdef HALO_VITA_MENU_RENDER_PROBE
    if (!root_active) halo_vita_renderer_dispose_before_root();
#endif
	if (root_active) {
		#ifdef HALO_VITA_ORIGINAL_RUNTIME
        vita_log("[VITA ORIGINAL] test shell exits with active lifetime owners; native process releases retained cache/arena");
#else
        vita_log("UI root/cache/arena retained until process exit; active-widget teardown not yet linked");
#endif
		ui_disposed = 0;
	} else ui_disposed = halo_vita_ui_runtime_dispose();
	if (ui_disposed) {
		if (!halo_vita_cache_unmount_menu()) vita_log("UI cache restoration failed during shutdown");
	} else vita_log("UI cache and arena retained until process exit because widgets remain active");
	if (ui_disposed && (memory || arena)) halo_vita_memory_dispose();
	if (ui_disposed && arena) vita_memory_shutdown();
	if (graphics) vita_graphics_shutdown();
	if (ui_disposed && core) halo_vita_core_dispose();
	vita_platform_shutdown();
	sceKernelExitProcess(0);
	return 0;
}