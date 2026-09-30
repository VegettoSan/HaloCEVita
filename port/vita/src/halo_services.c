/* XDK-facing services needed by the linked Halo-core bring-up.
 * This is deliberately not a replacement for the complete Xbox API layer. */
#include "platform.h"
#include "cseries.h"
#include "cseries_windows.h"
#include "errors.h"
#include "tag_files/files.h"
#include "interface/marketing_and_strategic_business_development.h"
#include "vita_runtime.h"
#include <stdarg.h>
#include <sched.h>

#undef malloc
#undef free
#undef realloc
#undef memset
#undef memcpy
#undef vsnprintf
#undef vsprintf

unsigned long system_milliseconds(void) { return (unsigned long)(vita_time_us() / 1000u); }
void *system_malloc(long size) { return size > 0 ? malloc((size_t)size) : NULL; }
void *system_realloc(void *pointer, long size)
{
	if (size <= 0) { free(pointer); return NULL; }
	return realloc(pointer, (size_t)size);
}
void system_free(void *pointer) { free(pointer); }
void system_exit(long code) { vita_log("Halo system_exit(%ld)", code); vita_fatal("Halo assertion/exit"); }
void halt_and_catch_fire(void) { vita_fatal("Halo halt_and_catch_fire"); }
void stack_walk(short levels)
{
	(void)levels;
	/* Never follow the upstream x86 EBP chain on ARM. */
	vita_log("stack_walk: ARM unwinding unavailable; retain ELF and console crash dump");
}
/* Bring-up diagnostic sink. Full errors.c/UI integration remains pending. */
void error(short priority, const char *format, ...)
{
	char message[2048]; va_list arguments;
	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	vita_log("Halo error[%d]: %s", priority, message);
}
void platform_log(const char *format, ...)
{
	char message[2048]; va_list arguments;
	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	vita_log("%s", message);
}
BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER *counter)
{
	if (!counter) return FALSE;
	counter->QuadPart = (__int64)vita_time_us(); return TRUE;
}
BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER *frequency)
{
	if (!frequency) return FALSE;
	frequency->QuadPart = 1000000; return TRUE;
}
DWORD WINAPI GetTickCount(void) { return (DWORD)(vita_time_us() / 1000); }
/* The Vita port uses the same scheduling contract as the native Linux layer:
 * yield the current thread and report success. Texture-cache waits only need
 * to let resource work make progress; this is not a success-only stub. */
BOOL WINAPI SwitchToThread(void)
{
	sched_yield();
	return TRUE;
}
/* VitaSDK's math headers route the game's standard sin/cos calls through
 * halo_sin/halo_cos. Preserve the C double-precision contract and delegate to
 * the compiler/libm implementation instead of approximating widget motion. */
double halo_sin(double angle) { return __builtin_sin(angle); }
double halo_cos(double angle) { return __builtin_cos(angle); }

/* Main Menu bring-up has no initialized Halo sound manager yet. The original
 * texture/cache waits only consult these functions to decide whether audio
 * needs servicing while a blocking read is in progress. Reporting the current
 * clock keeps that optional service branch dormant until real audio is brought
 * up; an explicit call still yields instead of pretending to process audio. */
long sound_render_time(void)
{
	return (long)system_milliseconds();
}
void sound_idle(void)
{
	static int logged;
	if (!logged) {
		logged = 1;
		vita_log("[VITA AUDIO] sound_idle requested before sound manager initialization; yielding only");
	}
	sched_yield();
}

DWORD WINAPI GetLastError(void) { return (DWORD)vita_xapi_last_error_get(); }
VOID WINAPI SetLastError(DWORD error) { vita_xapi_last_error_set((uint32_t)error); }
DWORD WINAPI GetFileAttributesA(LPCSTR path)
{
	uint32_t attributes, error;
	if (vita_xapi_file_attributes(path, &attributes, &error)) return (DWORD)attributes;
	SetLastError((DWORD)error);
	return (DWORD)-1;
}
int halo_vita_file_contract_probe(void)
{
	struct file_reference reference;
	DWORD directory_attributes, missing_attributes;
	int map_found, missing_rejected, demos_available;
	SetLastError(0x1357);
	file_reference_create_from_path(&reference, "d:\\MAPS\\UI.MAP", FALSE);
	map_found = file_exists(&reference) && GetLastError() == 0x1357;
	directory_attributes = GetFileAttributesA("d:\\MAPS");
	file_reference_create_from_path(&reference, "d:\\maps\\__halo_vita_missing__.map", FALSE);
	missing_rejected = !file_exists(&reference) && GetLastError() == ERROR_FILE_NOT_FOUND;
	missing_attributes = GetFileAttributesA("d:\\maps\\__halo_vita_missing__.map");
	missing_rejected &= missing_attributes == (DWORD)-1 && GetLastError() == ERROR_FILE_NOT_FOUND;
	demos_available = xbox_demos_available();
	vita_log("[VITA 024] original Xbox file contract: casefold_ui=%d maps_directory=%d missing_file=%d XDemos=%s",
		map_found, directory_attributes != (DWORD)-1 &&
			(directory_attributes & FILE_ATTRIBUTE_DIRECTORY) != 0,
		missing_rejected, demos_available ? "present" : "absent");
	return map_found && directory_attributes != (DWORD)-1 &&
		(directory_attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && missing_rejected;
}
VOID WINAPI GlobalMemoryStatus(MEMORYSTATUS *status)
{
	uint32_t user, cdram, phycont;
	vita_free_memory(&user, &cdram, &phycont);
	memset(status, 0, sizeof(*status)); status->dwLength = sizeof(*status);
	/* Report available user RAM; do not advertise GPU memory as general heap. */
	status->dwAvailPhys = user; status->dwTotalPhys = user;
	status->dwAvailVirtual = user; status->dwTotalVirtual = user;
}
/* Reuse port/linux/src/msvc_crt.c's format translation: %I64 -> %ll,
 * %I32 -> ordinary 32-bit conversion. The algorithms/data types stay native. */
static const char *translate_format(const char *format, char *buffer, size_t size)
{
	const char *cursor; size_t length = 0;
	if (!strstr(format, "I64") && !strstr(format, "I32")) return format;
	for (cursor = format; *cursor && length + 3 < size; cursor++) {
		buffer[length++] = *cursor;
		if (*cursor != '%') continue;
		if (cursor[1] == '%') { buffer[length++] = *++cursor; continue; }
		while (cursor[1] && strchr("-+ #0123456789.*", cursor[1]) && length + 3 < size)
			buffer[length++] = *++cursor;
		if (cursor[1] == 'I' && cursor[2] == '6' && cursor[3] == '4') {
			buffer[length++] = 'l'; buffer[length++] = 'l'; cursor += 3;
		} else if (cursor[1] == 'I' && cursor[2] == '3' && cursor[3] == '2') cursor += 3;
	}
	buffer[length] = 0; return buffer;
}
int halo_linux_vsprintf(char *buffer, const char *format, va_list arguments)
{
	char translated[1024];
	/* cseries' legacy API has no destination size. Preserve its contract. */
	return vsprintf(buffer, translate_format(format, translated, sizeof(translated)), arguments);
}
int halo_linux_vsnprintf(char *buffer, size_t count, const char *format, va_list arguments)
{
	char translated[1024];
	return vsnprintf(buffer, count, translate_format(format, translated, sizeof(translated)), arguments);
}
int halo_linux_snprintf(char *buffer, size_t count, const char *format, ...)
{
	int result; va_list arguments;
	va_start(arguments, format); result = halo_linux_vsnprintf(buffer, count, format, arguments);
	va_end(arguments); return result;
}
int halo_linux_fprintf(FILE *stream, const char *format, ...)
{
	int result; va_list arguments; char translated[1024];
	va_start(arguments, format); result = vfprintf(stream, translate_format(format, translated, sizeof(translated)), arguments);
	va_end(arguments); return result;
}
FILE *halo_linux_fopen(const char *path, const char *mode)
{
	char translated[512]; size_t cursor = 0;
	/* The linked guarded heap writes d:\\heap_dump.txt on clean shutdown.
	 * Full Xbox case-insensitive filesystem/XAPI integration is still pending. */
	if (path[0] && path[1] == ':') path += 2;
	while (*path == '/' || *path == '\\') ++path;
	strcpy(translated, HALO_VITA_DATA_ROOT);
	cursor = strlen(translated);
	while (*path && cursor + 1 < sizeof(translated)) {
		translated[cursor++] = *path == '\\' ? '/' : *path; ++path;
	}
	if (*path) { vita_log("Halo fopen: path too long"); return NULL; }
	translated[cursor] = 0;
	return fopen(translated, mode);
}
int _stricmp(const char *a, const char *b)
{
	while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) { ++a; ++b; }
	return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}
const char *config_string(const char *name) { (void)name; return ""; }
int config_boolean(const char *name) { (void)name; return 0; }
