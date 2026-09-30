#!/usr/bin/env python3
"""Exercise the real Vita Xbox-path/last-error implementation with mocked I/O."""
import ctypes as C
import os
from pathlib import Path
import subprocess
import tempfile
import threading

ROOT = Path(__file__).resolve().parents[1]
DIR_HEADER = '''#include <stdint.h>
typedef struct { char d_name[256]; } SceIoDirent;
int sceIoDopen(const char *path);
int sceIoDread(int descriptor, SceIoDirent *entry);
int sceIoDclose(int descriptor);
'''
STAT_HEADER = '''#include <sys/stat.h>
typedef struct { unsigned st_mode; } SceIoStat;
#define SCE_S_ISDIR(mode) S_ISDIR(mode)
#define SCE_S_IWUSR S_IWUSR
int sceIoGetstat(const char *path, SceIoStat *stat);
'''
MOCK = r'''
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "psp2/io/dirent.h"
#include "psp2/io/stat.h"
static DIR *open_dirs[16];
static int translate(const char *path, char *target, size_t size) {
    const char *prefix = "ux0:data/HaloCE/";
    const char *root = getenv("HALO_XAPI_TEST_ROOT");
    if (!root || strncmp(path, prefix, strlen(prefix))) return 0;
    return snprintf(target, size, "%s/%s", root, path + strlen(prefix)) < (int)size;
}
int sceIoDopen(const char *path) {
    char target[1024];
    if (!translate(path, target, sizeof(target))) return -1;
    for (int i = 1; i < 16; ++i) if (!open_dirs[i]) {
        open_dirs[i] = opendir(target);
        return open_dirs[i] ? i : -1;
    }
    return -1;
}
int sceIoDread(int descriptor, SceIoDirent *entry) {
    struct dirent *item;
    if (descriptor < 1 || descriptor >= 16 || !open_dirs[descriptor]) return -1;
    item = readdir(open_dirs[descriptor]);
    if (!item) return 0;
    snprintf(entry->d_name, sizeof(entry->d_name), "%s", item->d_name);
    return 1;
}
int sceIoDclose(int descriptor) {
    if (descriptor < 1 || descriptor >= 16 || !open_dirs[descriptor]) return -1;
    closedir(open_dirs[descriptor]); open_dirs[descriptor] = NULL; return 0;
}
int sceIoGetstat(const char *path, SceIoStat *result) {
    char target[1024]; struct stat source;
    if (!translate(path, target, sizeof(target)) || stat(target, &source)) return -1;
    result->st_mode = source.st_mode; return 0;
}
void vita_fatal(const char *reason) { fprintf(stderr, "fatal: %s\n", reason); abort(); }
'''


def main():
    with tempfile.TemporaryDirectory(prefix='halo-vita-xapi-') as temporary:
        work = Path(temporary)
        headers = work / 'psp2' / 'io'
        headers.mkdir(parents=True)
        (headers / 'dirent.h').write_text(DIR_HEADER)
        (headers / 'stat.h').write_text(STAT_HEADER)
        (work / 'mock.c').write_text(MOCK)
        library = work / 'xapi.so'
        subprocess.run(['gcc', '-std=c11', '-D_DEFAULT_SOURCE', '-shared', '-fPIC',
                        '-I', str(work), '-I', str(ROOT / 'port/vita/include'),
                        str(ROOT / 'port/vita/src/vita_xapi_files.c'), str(work / 'mock.c'),
                        '-lpthread', '-o', str(library)], check=True)
        data = work / 'data'
        (data / 'maps').mkdir(parents=True)
        (data / 'maps' / 'Ui.Map').write_bytes(b'halo map fixture')
        os.environ['HALO_XAPI_TEST_ROOT'] = str(data)
        xapi = C.CDLL(str(library))
        xapi.vita_xapi_file_attributes.argtypes = [C.c_char_p, C.POINTER(C.c_uint32), C.POINTER(C.c_uint32)]
        xapi.vita_xapi_file_attributes.restype = C.c_int
        xapi.vita_xapi_last_error_get.restype = C.c_uint32
        xapi.vita_xapi_last_error_set.argtypes = [C.c_uint32]

        def attributes(path):
            value, error = C.c_uint32(), C.c_uint32()
            success = xapi.vita_xapi_file_attributes(path, C.byref(value), C.byref(error))
            return success, value.value, error.value

        assert attributes(b'd:\\MAPS\\UI.MAP') == (1, 128, 0)
        assert attributes(b'D:/maps') == (1, 16, 0)
        assert attributes(b'd:\\maps\\missing.map') == (0, 0, 2)
        assert attributes(b'd:\\missing\\file.map') == (0, 0, 3)
        assert attributes(b'd:\\maps\\Ui.Map\\file') == (0, 0, 3)
        assert attributes(b'd:\\..\\escape') == (0, 0, 87)
        assert attributes(b'e:\\maps\\Ui.Map') == (0, 0, 87)
        assert attributes(b'd:\\') == (1, 16, 0)

        xapi.vita_xapi_last_error_set(0x1357)
        worker_values = []

        def worker():
            worker_values.append(xapi.vita_xapi_last_error_get())
            xapi.vita_xapi_last_error_set(0x2468)
            worker_values.append(xapi.vita_xapi_last_error_get())

        thread = threading.Thread(target=worker)
        thread.start(); thread.join()
        assert worker_values == [0, 0x2468]
        assert xapi.vita_xapi_last_error_get() == 0x1357
        print('PASS: Xbox D: casefold/exact path, directory/file/missing errors, traversal guard, per-thread last-error')


if __name__ == '__main__':
    main()
