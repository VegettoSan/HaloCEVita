/* Native Vita services use the SDK ABI, never the game's short-wchar CRT. */
#include "vita_runtime.h"
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <time.h>
#include <SDL3/SDL.h>

unsigned int _newlib_heap_size_user = 64 * 1024 * 1024;
unsigned int _newlib_heap_size_user_default = 64 * 1024 * 1024;
int sceUserMainThreadStackSize = 2 * 1024 * 1024;
static int logging_ready;
static uint32_t previous_buttons;

static int16_t stick_axis(unsigned char value, int invert)
{
	int axis = (int)value - 128;
	/* Full endpoints, Xbox positive Y upwards. No additional dead zone yet:
	 * input_abstraction will apply the original game's dead-zone policy. */
	axis = axis < 0 ? axis * 256 : axis * 32767 / 127;
	if (invert) axis = axis == -32768 ? 32767 : -axis;
	return (int16_t)axis;
}
int vita_read_gamepad(struct vita_gamepad_sample *sample)
{
	SceCtrlData pad = {0}; uint32_t b;
	memset(sample, 0, sizeof(*sample));
	if (sceCtrlPeekBufferPositive(0, &pad, 1) <= 0) return 0;
	b = pad.buttons;
	if (b & SCE_CTRL_UP) sample->buttons |= 0x0001;
	if (b & SCE_CTRL_DOWN) sample->buttons |= 0x0002;
	if (b & SCE_CTRL_LEFT) sample->buttons |= 0x0004;
	if (b & SCE_CTRL_RIGHT) sample->buttons |= 0x0008;
	if (b & SCE_CTRL_START) sample->buttons |= 0x0010;
	if (b & SCE_CTRL_SELECT) sample->buttons |= 0x0020;
	sample->analog[0] = b & SCE_CTRL_CROSS ? 255 : 0;
	sample->analog[1] = b & SCE_CTRL_CIRCLE ? 255 : 0;
	sample->analog[2] = b & SCE_CTRL_SQUARE ? 255 : 0;
	sample->analog[3] = b & SCE_CTRL_TRIANGLE ? 255 : 0;
	sample->analog[6] = b & SCE_CTRL_LTRIGGER ? 255 : 0;
	sample->analog[7] = b & SCE_CTRL_RTRIGGER ? 255 : 0;
	/* White/Black and L3/R3 intentionally unassigned until ergonomic tests. */
	sample->lx = stick_axis(pad.lx, 0); sample->ly = stick_axis(pad.ly, 1);
	sample->rx = stick_axis(pad.rx, 0); sample->ry = stick_axis(pad.ry, 1);
	return 1;
}

uint64_t vita_time_us(void) { return sceKernelGetProcessTimeWide(); }
/* VitaSDK lacks POSIX clock_nanosleep. Keep the absolute CLOCK_MONOTONIC
 * deadline used by the original presentation worker, rechecking after every
 * native relative delay so early wakeups cannot advance its vblank counters. */
int halo_vita_wait_monotonic_deadline(uint64_t deadline_ns)
{
	struct timespec now;
	for (;;) {
		uint64_t current_ns, delay_us, remaining_ns;
		if (clock_gettime(CLOCK_MONOTONIC, &now) != 0 || now.tv_sec < 0 ||
			now.tv_nsec < 0 || now.tv_nsec >= 1000000000L) return 0;
		current_ns = (uint64_t)now.tv_sec * 1000000000ULL + (uint64_t)now.tv_nsec;
		if (current_ns >= deadline_ns) return 1;
		remaining_ns = deadline_ns - current_ns;
		delay_us = remaining_ns / 1000 + (remaining_ns % 1000 != 0);
		/* Bound the SDK's 32-bit delay argument even for a distant deadline. */
		if (delay_us > 1000000) delay_us = 1000000;
		if (sceKernelDelayThread((unsigned int)delay_us) < 0) return 0;
	}
}
void vita_log(const char *format, ...)
{
	char message[2304], line[2400]; va_list arguments;
	int length, fd, offset = 0;
	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments); va_end(arguments);
	length = snprintf(line, sizeof(line), "[%llu us] %s\n",
		(unsigned long long)vita_time_us(), message);
	if (length < 0) return;
	if (length >= (int)sizeof(line)) length = sizeof(line) - 1;
	/* Open/close per milestone: completed writes survive a later GPU crash. */
	fd = sceIoOpen(HALO_VITA_DATA_ROOT "debug.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);
	if (fd >= 0) {
		while (offset < length) {
			int written = sceIoWrite(fd, line + offset, length - offset);
			if (written <= 0) break;
			offset += written;
		}
		sceIoClose(fd);
	}
	fputs(line, stderr);
}
void vita_fatal(const char *reason)
{
	vita_log("FATAL: %s", reason);
	sceKernelDelayThread(3000000);
	sceKernelExitProcess(1);
	for (;;) {}
}
void vita_free_memory(uint32_t *user, uint32_t *cdram, uint32_t *phycont)
{
	SceKernelFreeMemorySizeInfo info = {0}; info.size = sizeof(info);
	if (sceKernelGetFreeMemorySize(&info) < 0) {
		*user = *cdram = *phycont = 0; return;
	}
	*user = info.size_user; *cdram = info.size_cdram; *phycont = info.size_phycont;
}
int vita_platform_initialize(void)
{
	int fd, controls;
	sceIoMkdir("ux0:data", 0777);
	sceIoMkdir("ux0:data/HaloCE", 0777);
	sceIoMkdir(HALO_VITA_DATA_ROOT "save", 0777);
	fd = sceIoOpen(HALO_VITA_DATA_ROOT "debug.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);
	logging_ready = fd >= 0;
	if (fd >= 0) sceIoClose(fd);
	vita_log("----- HaloCEVita native core bring-up " HALO_VITA_APP_VERSION " -----");
	vita_log("[VITA 001] process start");
	vita_log("[VITA 002] filesystem ready");
	vita_log("[VITA 003] log opened: %s", logging_ready ? "YES" : "FAILED");
	controls = sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	vita_log("[VITA 004] controls initialized result=0x%08x", controls);
	return logging_ready && controls >= 0;
}
int vita_controls_poll(void)
{
	SceCtrlData pad = {0}; uint32_t pressed;
	if (sceCtrlPeekBufferPositive(0, &pad, 1) <= 0) return 0;
	pressed = pad.buttons & ~previous_buttons; previous_buttons = pad.buttons;
	if (pressed) {
		struct vita_gamepad_sample xbox;
		vita_read_gamepad(&xbox);
		vita_log("input vita=%08x xbox=%04x A/B/X/Y=%u/%u/%u/%u LT/RT=%u/%u sticks=%d,%d/%d,%d",
			pad.buttons, xbox.buttons, xbox.analog[0], xbox.analog[1], xbox.analog[2], xbox.analog[3],
			xbox.analog[6], xbox.analog[7], xbox.lx, xbox.ly, xbox.rx, xbox.ry);
	}
	#ifdef HALO_VITA_MENU_BRINGUP
	/* Start is an authored shell/keyboard action. Select exits this staged VPK. */
	if (pressed & SCE_CTRL_SELECT) return -1;
#else
	if (pressed & SCE_CTRL_START) return -1;
#endif
#ifdef HALO_VITA_MENU_AUDIO
	if (pressed & SCE_CTRL_SQUARE) return 2;
#endif
	return (pressed & SCE_CTRL_CROSS) ? 1 : 0;
}
int vita_maps_verify(void)
{
	SceIoStat stat = {0}; SceIoDirent entry;
	int directory, count = 0, ui_valid = 0, a10_valid = 0;
	if (sceIoGetstat(HALO_VITA_DATA_ROOT, &stat) < 0) { vita_log("data root missing"); return 0; }
	vita_log("[VITA 008] data root found: " HALO_VITA_DATA_ROOT);
	directory = sceIoDopen(HALO_VITA_DATA_ROOT "maps");
	if (directory < 0) { vita_log("maps/ missing error=0x%08x; copy Xbox v5 maps here", directory); return 0; }
	memset(&entry, 0, sizeof(entry));
	while (sceIoDread(directory, &entry) > 0) {
		size_t length = strlen(entry.d_name);
		if (length > 4 && !strcasecmp(entry.d_name + length - 4, ".map")) ++count;
		memset(&entry, 0, sizeof(entry));
	}
	sceIoDclose(directory);
	/* Match the Xbox filesystem case-insensitively for the critical maps. */
	directory = sceIoDopen(HALO_VITA_DATA_ROOT "maps");
	if (directory < 0) return 0;
	memset(&entry, 0, sizeof(entry));
	while (sceIoDread(directory, &entry) > 0) {
		if (!strcasecmp(entry.d_name, "ui.map") || !strcasecmp(entry.d_name, "a10.map")) {
			char path[320]; unsigned char header[0x800]; int fd, got;
			snprintf(path, sizeof(path), HALO_VITA_DATA_ROOT "maps/%s", entry.d_name);
			fd = sceIoOpen(path, SCE_O_RDONLY, 0);
			got = fd >= 0 ? sceIoRead(fd, header, sizeof(header)) : -1;
			if (fd >= 0) sceIoClose(fd);
			if (got == (int)sizeof(header) && halo_vita_verify_map(header, sizeof(header), entry.d_name)) {
				if (!strcasecmp(entry.d_name, "ui.map")) ui_valid = 1;
				else a10_valid = 1;
			} else vita_log("map %s rejected by upstream Halo verifier or short read", entry.d_name);
		}
		memset(&entry, 0, sizeof(entry));
	}
	sceIoDclose(directory);
	vita_log("maps found=%d Xbox-v5 ui=%s a10=%s", count, ui_valid ? "valid" : "missing/invalid", a10_valid ? "valid" : "missing/invalid");
	if (ui_valid && a10_valid) vita_log("[VITA 009] maps verified (headers only; tags not loaded)");
	return ui_valid && a10_valid;
}
int vita_map_path(const char *name, char *path, size_t capacity)
{
	SceIoDirent entry;
	int directory = sceIoDopen(HALO_VITA_DATA_ROOT "maps"), found = 0;
	if (directory < 0) return 0;
	memset(&entry, 0, sizeof(entry));
	while (sceIoDread(directory, &entry) > 0) {
		if (!strcasecmp(entry.d_name, name)) {
			int length = snprintf(path, capacity, HALO_VITA_DATA_ROOT "maps/%s", entry.d_name);
			found = length > 0 && (size_t)length < capacity; break;
		}
		memset(&entry, 0, sizeof(entry));
	}
	sceIoDclose(directory); return found;
}
static pthread_mutex_t probe_lock = PTHREAD_MUTEX_INITIALIZER;
static int probe_value;
static uint32_t probe_worker_error_before, probe_worker_error_after;
static void *thread_probe(void *argument)
{
	(void)argument;
	probe_worker_error_before = vita_xapi_last_error_get();
	vita_xapi_last_error_set(0x2468);
	probe_worker_error_after = vita_xapi_last_error_get();
	pthread_mutex_lock(&probe_lock); probe_value = 42; pthread_mutex_unlock(&probe_lock);
	return NULL;
}
int vita_services_probe(void)
{
	pthread_t thread; int result, join_result = -1;
	probe_value = 0;
	vita_xapi_last_error_set(0x1357);
	result = pthread_create(&thread, NULL, thread_probe, NULL);
	if (!result) join_result = pthread_join(thread, NULL);
	vita_log("pthread create=%d join=%d synchronized value=%d", result, join_result, probe_value);
	vita_log("XAPI per-thread last-error: worker=%08x->%08x main=%08x",
		probe_worker_error_before, probe_worker_error_after, vita_xapi_last_error_get());
	if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		SDL_AudioSpec spec = {SDL_AUDIO_S16, 2, 48000};
		SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
		vita_log("SDL3 audio driver=%s stream=%s%s", SDL_GetCurrentAudioDriver(),
			stream ? "OPEN (paused, diagnostic only)" : "FAILED: ", stream ? "" : SDL_GetError());
		if (stream) SDL_DestroyAudioStream(stream);
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	} else vita_log("SDL3 audio probe failed: %s", SDL_GetError());
	return !result && !join_result && probe_value == 42 &&
		probe_worker_error_before == 0 && probe_worker_error_after == 0x2468 &&
		vita_xapi_last_error_get() == 0x1357;
}
void vita_platform_shutdown(void) { vita_log("clean exit"); }
int halo_vita_audio_device_initialize(void)
{
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		vita_log("[VITA AUDIO] SDL initialization failed: %s", SDL_GetError());
		return 0;
	}
	return 1;
}
