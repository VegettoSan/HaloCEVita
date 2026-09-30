/* Native filesystem and thread-error primitives for original XDK callers. */
#include "vita_runtime.h"
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

enum {
	VITA_ERROR_SUCCESS = 0,
	VITA_ERROR_FILE_NOT_FOUND = 2,
	VITA_ERROR_PATH_NOT_FOUND = 3,
	VITA_ERROR_ACCESS_DENIED = 5,
	VITA_ERROR_INVALID_HANDLE = 6,
	VITA_ERROR_INVALID_PARAMETER = 87,
	VITA_ERROR_ALREADY_EXISTS = 183,
	VITA_FILE_ATTRIBUTE_READONLY = 1,
	VITA_FILE_ATTRIBUTE_DIRECTORY = 16,
	VITA_FILE_ATTRIBUTE_NORMAL = 128
};

enum {
	VITA_GENERIC_READ = 0x80000000u,
	VITA_GENERIC_WRITE = 0x40000000u,
	VITA_CREATE_NEW = 1,
	VITA_CREATE_ALWAYS = 2,
	VITA_OPEN_EXISTING = 3,
	VITA_OPEN_ALWAYS = 4,
	VITA_TRUNCATE_EXISTING = 5,
	VITA_FILE_BEGIN = 0,
	VITA_FILE_CURRENT = 1,
	VITA_FILE_END = 2
};

#define VITA_INVALID_HANDLE ((void *)(intptr_t)-1)
#define VITA_INVALID_SET_FILE_POINTER 0xffffffffu

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

static int append_component(char *path, size_t capacity, const char *name)
{
	size_t used = strlen(path), length = strlen(name);
	if (used + length + 1 > capacity) return 0;
	memcpy(path + used, name, length + 1);
	return 1;
}

static int prepare_drive_root(char drive, int create, char *path, size_t capacity,
	uint32_t *error)
{
	char drive_path[sizeof(HALO_VITA_DATA_ROOT) + 2];
	if (drive == 'd' || drive == 'D') {
		if (strlen(HALO_VITA_DATA_ROOT) + 1 > capacity) {
			*error = VITA_ERROR_INVALID_PARAMETER;
			return 0;
		}
		strcpy(path, HALO_VITA_DATA_ROOT);
		return 1;
	}
	/* Match the native port's Xbox-drive contract: writable Z/U/T drives live
	 * below the persistent save root rather than beside host paths. Vita uses
	 * one persistent ux0:data root, with a directory per emulated drive. */
	if (drive != 'z' && drive != 'Z' && drive != 'u' && drive != 'U' &&
		drive != 't' && drive != 'T') {
		*error = VITA_ERROR_INVALID_PARAMETER;
		return 0;
	}
	snprintf(drive_path, sizeof(drive_path), "%s%c", HALO_VITA_DATA_ROOT,
		(char)(drive >= 'A' && drive <= 'Z' ? drive + ('a' - 'A') : drive));
	if (create && sceIoMkdir(drive_path, 0777) < 0) {
		SceIoStat stat;
		if (sceIoGetstat(drive_path, &stat) < 0 || !SCE_S_ISDIR(stat.st_mode)) {
			*error = VITA_ERROR_ACCESS_DENIED;
			return 0;
		}
	}
	if (strlen(drive_path) + 2 > capacity) {
		*error = VITA_ERROR_INVALID_PARAMETER;
		return 0;
	}
	strcpy(path, drive_path);
	strcat(path, "/");
	return 1;
}

/* Resolve an Xbox path component-by-component. Existing components preserve
 * FATX-style case-insensitive lookup. For CREATE_* callers only the final
 * component may be absent; its requested spelling is then retained. */
static int resolve_xbox_path(const char *xbox_path, char *resolved, size_t capacity,
	int allow_missing_leaf, int create_drive, int *leaf_exists, uint32_t *error)
{
	const char *cursor;
	SceIoStat stat;
	if (leaf_exists) *leaf_exists = 0;
	if (!xbox_path || !resolved || !capacity || !error || strlen(xbox_path) < 3 ||
		xbox_path[1] != ':' || (xbox_path[2] != '\\' && xbox_path[2] != '/')) {
		if (error) *error = VITA_ERROR_INVALID_PARAMETER;
		return 0;
	}
	if (!prepare_drive_root(xbox_path[0], create_drive, resolved, capacity, error))
		return 0;
	cursor = xbox_path + 3;
	if (!*cursor) {
		if (sceIoGetstat(resolved, &stat) < 0) {
			*error = VITA_ERROR_PATH_NOT_FOUND;
			return 0;
		}
		if (leaf_exists) *leaf_exists = 1;
		*error = VITA_ERROR_SUCCESS;
		return 1;
	}
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
		used = strlen(resolved);
		directory = sceIoDopen(resolved);
		if (directory < 0) {
			*error = VITA_ERROR_PATH_NOT_FOUND;
			return 0;
		}
		memset(&entry, 0, sizeof(entry));
		while ((read_result = sceIoDread(directory, &entry)) > 0) {
			if (!strcasecmp(entry.d_name, component)) {
				if (!found || !strcmp(entry.d_name, component)) {
					if (!append_component(resolved, capacity, entry.d_name)) {
						sceIoDclose(directory);
						*error = VITA_ERROR_INVALID_PARAMETER;
						return 0;
					}
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
			if (!*cursor && allow_missing_leaf) {
				if (used >= capacity) {
					*error = VITA_ERROR_INVALID_PARAMETER;
					return 0;
				}
				resolved[used] = 0;
				if (!append_component(resolved, capacity, component)) {
					*error = VITA_ERROR_INVALID_PARAMETER;
					return 0;
				}
				if (leaf_exists) *leaf_exists = 0;
				*error = VITA_ERROR_SUCCESS;
				return 1;
			}
			*error = *cursor ? VITA_ERROR_PATH_NOT_FOUND : VITA_ERROR_FILE_NOT_FOUND;
			return 0;
		}
		if (*cursor) {
			if (sceIoGetstat(resolved, &stat) < 0 || !SCE_S_ISDIR(stat.st_mode)) {
				*error = VITA_ERROR_PATH_NOT_FOUND;
				return 0;
			}
			if (strlen(resolved) + 2 > capacity) {
				*error = VITA_ERROR_INVALID_PARAMETER;
				return 0;
			}
			strcat(resolved, "/");
			++cursor;
			if (!*cursor) break;
		}
	}
	if (sceIoGetstat(resolved, &stat) < 0) {
		*error = VITA_ERROR_PATH_NOT_FOUND;
		return 0;
	}
	if (leaf_exists) *leaf_exists = 1;
	*error = VITA_ERROR_SUCCESS;
	return 1;
}

/* Xbox D: is rooted at the user's Vita data directory. */
int vita_xapi_file_attributes(const char *xbox_path, uint32_t *attributes, uint32_t *error)
{
	char current[512];
	SceIoStat stat;
	int exists;
	if (!attributes || !error ||
		!resolve_xbox_path(xbox_path, current, sizeof(current), 0, 0, &exists, error))
		return 0;
	if (sceIoGetstat(current, &stat) < 0) {
		*error = VITA_ERROR_PATH_NOT_FOUND;
		return 0;
	}
	*attributes = 0;
	if (SCE_S_ISDIR(stat.st_mode)) *attributes |= VITA_FILE_ATTRIBUTE_DIRECTORY;
	if (!(stat.st_mode & SCE_S_IWUSR)) *attributes |= VITA_FILE_ATTRIBUTE_READONLY;
	if (!*attributes) *attributes = VITA_FILE_ATTRIBUTE_NORMAL;
	*error = VITA_ERROR_SUCCESS;
	return 1;
}

/* Minimal Win32/XDK file contract required by original game_state_xbox.c.
 * HANDLE is opaque to Halo; on 32-bit Vita we carry the native SceUID through
 * it and normalize all native failures to INVALID_HANDLE_VALUE. */
void *CreateFileA(const char *file_name, unsigned long desired_access,
	unsigned long share_mode, void *security_attributes,
	unsigned long creation_disposition, unsigned long flags_and_attributes,
	void *template_file)
{
	char path[512];
	uint32_t error;
	int exists = 0, open_flags = 0, fd;
	int creating = creation_disposition == VITA_CREATE_NEW ||
		creation_disposition == VITA_CREATE_ALWAYS ||
		creation_disposition == VITA_OPEN_ALWAYS;
	(void)share_mode;
	(void)security_attributes;
	(void)flags_and_attributes;
	(void)template_file;

	if ((desired_access & (VITA_GENERIC_READ | VITA_GENERIC_WRITE)) ==
		(VITA_GENERIC_READ | VITA_GENERIC_WRITE)) open_flags |= SCE_O_RDWR;
	else if (desired_access & VITA_GENERIC_WRITE) open_flags |= SCE_O_WRONLY;
	else if (desired_access & VITA_GENERIC_READ) open_flags |= SCE_O_RDONLY;
	else {
		vita_xapi_last_error_set(VITA_ERROR_INVALID_PARAMETER);
		return VITA_INVALID_HANDLE;
	}
	if (creation_disposition < VITA_CREATE_NEW ||
		creation_disposition > VITA_TRUNCATE_EXISTING) {
		vita_xapi_last_error_set(VITA_ERROR_INVALID_PARAMETER);
		return VITA_INVALID_HANDLE;
	}
	if (!resolve_xbox_path(file_name, path, sizeof(path), creating, creating,
		&exists, &error)) {
		vita_xapi_last_error_set(error);
		return VITA_INVALID_HANDLE;
	}
	if (creation_disposition == VITA_CREATE_NEW && exists) {
		vita_xapi_last_error_set(VITA_ERROR_ALREADY_EXISTS);
		return VITA_INVALID_HANDLE;
	}
	if ((creation_disposition == VITA_OPEN_EXISTING ||
		creation_disposition == VITA_TRUNCATE_EXISTING) && !exists) {
		vita_xapi_last_error_set(VITA_ERROR_FILE_NOT_FOUND);
		return VITA_INVALID_HANDLE;
	}
	if (creating) open_flags |= SCE_O_CREAT;
	if (creation_disposition == VITA_CREATE_ALWAYS ||
		creation_disposition == VITA_TRUNCATE_EXISTING) open_flags |= SCE_O_TRUNC;
	if (creation_disposition == VITA_CREATE_NEW) open_flags |= SCE_O_EXCL;
	fd = sceIoOpen(path, open_flags, 0666);
	if (fd < 0) {
		vita_xapi_last_error_set(VITA_ERROR_ACCESS_DENIED);
		return VITA_INVALID_HANDLE;
	}
	vita_xapi_last_error_set(exists && (creation_disposition == VITA_CREATE_ALWAYS ||
		creation_disposition == VITA_OPEN_ALWAYS) ? VITA_ERROR_ALREADY_EXISTS :
		VITA_ERROR_SUCCESS);
	return (void *)(intptr_t)fd;
}

unsigned long SetFilePointer(void *handle, long distance_low, long *distance_high,
	unsigned long move_method)
{
	SceOff distance, position;
	int whence;
	SceUID fd = (SceUID)(intptr_t)handle;
	if (handle == VITA_INVALID_HANDLE || fd < 0) {
		vita_xapi_last_error_set(VITA_ERROR_INVALID_HANDLE);
		return VITA_INVALID_SET_FILE_POINTER;
	}
	if (distance_high)
		distance = ((SceOff)(*distance_high) << 32) | (uint32_t)distance_low;
	else
		distance = (SceOff)distance_low;
	switch (move_method) {
	case VITA_FILE_BEGIN: whence = SCE_SEEK_SET; break;
	case VITA_FILE_CURRENT: whence = SCE_SEEK_CUR; break;
	case VITA_FILE_END: whence = SCE_SEEK_END; break;
	default:
		vita_xapi_last_error_set(VITA_ERROR_INVALID_PARAMETER);
		return VITA_INVALID_SET_FILE_POINTER;
	}
	position = sceIoLseek(fd, distance, whence);
	if (position < 0) {
		vita_xapi_last_error_set(VITA_ERROR_INVALID_PARAMETER);
		return VITA_INVALID_SET_FILE_POINTER;
	}
	if (distance_high) *distance_high = (long)((uint64_t)position >> 32);
	vita_xapi_last_error_set(VITA_ERROR_SUCCESS);
	return (unsigned long)((uint64_t)position & 0xffffffffu);
}

int SetEndOfFile(void *handle)
{
	SceIoStat stat;
	SceOff position;
	SceUID fd = (SceUID)(intptr_t)handle;
	if (handle == VITA_INVALID_HANDLE || fd < 0) {
		vita_xapi_last_error_set(VITA_ERROR_INVALID_HANDLE);
		return 0;
	}
	position = sceIoLseek(fd, 0, SCE_SEEK_CUR);
	if (position < 0) {
		vita_xapi_last_error_set(VITA_ERROR_INVALID_PARAMETER);
		return 0;
	}
	memset(&stat, 0, sizeof(stat));
	stat.st_size = position;
	if (sceIoChstatByFd(fd, &stat, SCE_CST_SIZE) < 0) {
		vita_xapi_last_error_set(VITA_ERROR_ACCESS_DENIED);
		return 0;
	}
	vita_xapi_last_error_set(VITA_ERROR_SUCCESS);
	return 1;
}
