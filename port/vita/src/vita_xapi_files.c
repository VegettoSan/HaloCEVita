/* Native filesystem and thread-error primitives for original XDK callers. */
#include "vita_runtime.h"
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

enum {
	VITA_ERROR_FILE_NOT_FOUND = 2,
	VITA_ERROR_PATH_NOT_FOUND = 3,
	VITA_ERROR_INVALID_PARAMETER = 87,
	VITA_FILE_ATTRIBUTE_READONLY = 1,
	VITA_FILE_ATTRIBUTE_DIRECTORY = 16,
	VITA_FILE_ATTRIBUTE_NORMAL = 128
};

static pthread_key_t last_error_key;
static pthread_once_t last_error_once = PTHREAD_ONCE_INIT;

static void create_last_error_key(void)
{
	if (pthread_key_create(&last_error_key, NULL))
		vita_fatal("XAPI per-thread last-error key creation failed");
}

uint32_t vita_xapi_last_error_get(void)
{
	if (pthread_once(&last_error_once, create_last_error_key))
		vita_fatal("XAPI per-thread last-error initialization failed");
	return (uint32_t)(uintptr_t)pthread_getspecific(last_error_key);
}

void vita_xapi_last_error_set(uint32_t error)
{
	if (pthread_once(&last_error_once, create_last_error_key) ||
		pthread_setspecific(last_error_key, (void *)(uintptr_t)error))
		vita_fatal("XAPI per-thread last-error update failed");
}

/* Xbox D: is rooted at the user's Vita data directory. Resolve each name via
 * directory enumeration to preserve Xbox case-insensitive lookup semantics. */
int vita_xapi_file_attributes(const char *xbox_path, uint32_t *attributes, uint32_t *error)
{
	char current[512] = HALO_VITA_DATA_ROOT;
	const char *cursor;
	SceIoStat stat;
	if (!xbox_path || !attributes || !error || strlen(xbox_path) < 3 ||
		(xbox_path[0] != 'd' && xbox_path[0] != 'D') ||
		xbox_path[1] != ':' ||
		(xbox_path[2] != '\\' && xbox_path[2] != '/')) {
		if (error) *error = VITA_ERROR_INVALID_PARAMETER;
		return 0;
	}
	cursor = xbox_path + 3;
	while (*cursor) {
		char component[256];
		SceIoDirent entry;
		const char *start = cursor;
		size_t length, used;
		int directory, read_result, found = 0, exact = 0;
		while (*cursor && *cursor != '\\' && *cursor != '/') ++cursor;
		length = (size_t)(cursor - start);
		if (!length || length >= sizeof(component) ||
			(length == 1 && start[0] == '.') ||
			(length == 2 && start[0] == '.' && start[1] == '.')) {
			*error = VITA_ERROR_INVALID_PARAMETER;
			return 0;
		}
		memcpy(component, start, length);
		component[length] = 0;
		used = strlen(current);
		directory = sceIoDopen(current);
		if (directory < 0) {
			*error = VITA_ERROR_PATH_NOT_FOUND;
			return 0;
		}
		memset(&entry, 0, sizeof(entry));
		while ((read_result = sceIoDread(directory, &entry)) > 0) {
			if (!strcasecmp(entry.d_name, component)) {
				if (!found || !strcmp(entry.d_name, component)) {
					size_t name_length = strlen(entry.d_name);
					if (used + name_length >= sizeof(current)) {
						sceIoDclose(directory);
						*error = VITA_ERROR_INVALID_PARAMETER;
						return 0;
					}
					memcpy(current + used, entry.d_name, name_length + 1);
					found = 1;
					exact = strcmp(entry.d_name, component) == 0;
				}
				if (exact) break;
			}
			memset(&entry, 0, sizeof(entry));
		}
		sceIoDclose(directory);
		if (read_result < 0) {
			*error = VITA_ERROR_PATH_NOT_FOUND;
			return 0;
		}
		if (!found) {
			*error = *cursor ? VITA_ERROR_PATH_NOT_FOUND : VITA_ERROR_FILE_NOT_FOUND;
			return 0;
		}
		if (*cursor) {
			if (sceIoGetstat(current, &stat) < 0 || !SCE_S_ISDIR(stat.st_mode)) {
				*error = VITA_ERROR_PATH_NOT_FOUND;
				return 0;
			}
			if (strlen(current) + 1 >= sizeof(current)) {
				*error = VITA_ERROR_INVALID_PARAMETER;
				return 0;
			}
			strcat(current, "/");
			++cursor;
			if (!*cursor) break;
		}
	}
	if (sceIoGetstat(current, &stat) < 0) {
		*error = VITA_ERROR_PATH_NOT_FOUND;
		return 0;
	}
	*attributes = 0;
	if (SCE_S_ISDIR(stat.st_mode)) *attributes |= VITA_FILE_ATTRIBUTE_DIRECTORY;
	if (!(stat.st_mode & SCE_S_IWUSR)) *attributes |= VITA_FILE_ATTRIBUTE_READONLY;
	if (!*attributes) *attributes = VITA_FILE_ATTRIBUTE_NORMAL;
	*error = 0;
	return 1;
}
