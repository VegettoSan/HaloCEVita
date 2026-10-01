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
#include "stat.h"
typedef struct { char d_name[256]; SceIoStat d_stat; } SceIoDirent;
int sceIoDopen(const char *path);
int sceIoDread(int descriptor, SceIoDirent *entry);
int sceIoDclose(int descriptor);
'''
STAT_HEADER = '''#include <sys/stat.h>
#pragma once
#include <stdint.h>
typedef struct { unsigned st_mode; int64_t st_size; } SceIoStat;
#define SCE_S_ISDIR(mode) S_ISDIR(mode)
#define SCE_S_IWUSR S_IWUSR
#define SCE_S_IWGRP S_IWGRP
#define SCE_S_IWOTH S_IWOTH
#define SCE_CST_SIZE 1
#define SCE_CST_MODE 2
int sceIoGetstatByFd(int fd, SceIoStat *stat);
int sceIoChstatByFd(int fd, SceIoStat *stat, int mask);
int sceIoChstat(const char *path, SceIoStat *stat, int mask);
int sceIoGetstat(const char *path, SceIoStat *stat);
'''
FCNTL_HEADER = """#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
typedef int SceUID;
typedef int64_t SceOff;
#define SCE_O_RDWR O_RDWR
#define SCE_O_RDONLY O_RDONLY
#define SCE_O_WRONLY O_WRONLY
#define SCE_O_CREAT O_CREAT
#define SCE_O_EXCL O_EXCL
#define SCE_O_TRUNC O_TRUNC
#define SCE_SEEK_SET SEEK_SET
#define SCE_SEEK_CUR SEEK_CUR
#define SCE_SEEK_END SEEK_END
int sceIoOpen(const char *path,int flags,int mode);
int sceIoClose(int fd);
int sceIoRead(int fd,void *buffer,unsigned long count);
int sceIoWrite(int fd,const void *buffer,unsigned long count);
SceOff sceIoLseek(int fd,SceOff offset,int whence);
int sceIoMkdir(const char *path,int mode);
int sceIoRemove(const char *path);
int sceIoRmdir(const char *path);
int sceIoRename(const char *from,const char *to);
"""
DEVCTL_HEADER = """#include <stdint.h>
typedef struct {uint64_t max_size,free_size;} SceIoDevInfo;
int sceIoDevctl(const char *device,int cmd,void *input,int input_size,void *output,int output_size);
"""
MOCK = r'''
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "psp2/io/dirent.h"
#include "psp2/io/stat.h"
static DIR *open_dirs[16];
static char dir_paths[16][1024];
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
        snprintf(dir_paths[i],sizeof(dir_paths[i]),"%s",target);
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
    char full[1280]; struct stat st;
    snprintf(full,sizeof(full),"%s/%s",dir_paths[descriptor],entry->d_name);
    if(stat(full,&st)) return -1;
    entry->d_stat.st_mode=st.st_mode;entry->d_stat.st_size=st.st_size;
    return 1;
}
int sceIoDclose(int descriptor) {
    if (descriptor < 1 || descriptor >= 16 || !open_dirs[descriptor]) return -1;
    closedir(open_dirs[descriptor]); open_dirs[descriptor] = NULL; return 0;
}
int sceIoGetstat(const char *path, SceIoStat *result) {
    char target[1024]; struct stat source;
    if (!translate(path, target, sizeof(target)) || stat(target, &source)) return -1;
    result->st_mode = source.st_mode; result->st_size=source.st_size; return 0;
}
#include "psp2/io/fcntl.h"
#include "psp2/io/devctl.h"
#include <sys/statvfs.h>
int sceIoOpen(const char *p,int flags,int mode) {char t[1024];return translate(p,t,sizeof(t))?open(t,flags,mode):-1;}
int sceIoClose(int fd){return close(fd);}
int sceIoRead(int fd,void *p,unsigned long n){return read(fd,p,n);}
int sceIoWrite(int fd,const void *p,unsigned long n){return write(fd,p,n);}
SceOff sceIoLseek(int fd,SceOff o,int w){return lseek(fd,o,w);}
int sceIoMkdir(const char *p,int mode){char t[1024];return translate(p,t,sizeof(t))?mkdir(t,mode):-1;}
int sceIoRemove(const char *p){char t[1024];return translate(p,t,sizeof(t))?unlink(t):-1;}
int sceIoRmdir(const char *p){char t[1024];return translate(p,t,sizeof(t))?rmdir(t):-1;}
int sceIoRename(const char *a,const char *b){char x[1024],y[1024];return translate(a,x,sizeof(x))&&translate(b,y,sizeof(y))?rename(x,y):-1;}
int sceIoGetstatByFd(int fd,SceIoStat *s){struct stat st;if(fstat(fd,&st))return -1;s->st_mode=st.st_mode;s->st_size=st.st_size;return 0;}
int sceIoChstatByFd(int fd,SceIoStat *s,int m){return m==SCE_CST_SIZE?ftruncate(fd,s->st_size):fchmod(fd,s->st_mode);}
int sceIoChstat(const char *p,SceIoStat *s,int m){char t[1024];if(!translate(p,t,sizeof(t)))return -1;return m==SCE_CST_MODE?chmod(t,s->st_mode):truncate(t,s->st_size);}
int sceIoDevctl(const char *d,int cmd,void *in,int ins,void *out,int outs){
 (void)in;(void)ins;struct statvfs st;SceIoDevInfo info;
 if(strcmp(d,"ux0:")||cmd!=0x3001||outs!=sizeof(info)||statvfs(getenv("HALO_XAPI_TEST_ROOT"),&st))return -1;
 info.max_size=(uint64_t)st.f_blocks*st.f_frsize;info.free_size=(uint64_t)st.f_bavail*st.f_frsize;
 memcpy(out,&info,sizeof(info));return 0;}
int halo_vita_kernel_CloseHandle(void *handle){(void)handle;return 0;}
void vita_fatal(const char *reason) { fprintf(stderr, "fatal: %s\n", reason); abort(); }
'''


def main():
    with tempfile.TemporaryDirectory(prefix='halo-vita-xapi-') as temporary:
        work = Path(temporary)
        headers = work / 'psp2' / 'io'
        headers.mkdir(parents=True)
        (headers / 'dirent.h').write_text(DIR_HEADER)
        (headers / 'stat.h').write_text(STAT_HEADER)
        (headers / 'fcntl.h').write_text(FCNTL_HEADER)
        (headers / 'devctl.h').write_text(DEVCTL_HEADER)
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
        xapi.CreateDirectoryA.argtypes = [C.c_char_p, C.c_void_p]
        assert xapi.CreateDirectoryA(b'z:\\saved', None)
        assert xapi.CreateDirectoryA(b'z:\\saved\\profiles\\', None)
        assert not xapi.CreateDirectoryA(b'z:\\saved\\profiles', None)
        assert xapi.vita_xapi_last_error_get() == 183
        xapi.CreateFileA.argtypes = [C.c_char_p, C.c_ulong, C.c_ulong, C.c_void_p,
                                    C.c_ulong, C.c_ulong, C.c_void_p]
        xapi.CreateFileA.restype = C.c_void_p
        xapi.CloseHandle.argtypes = [C.c_void_p]
        xapi.SetFilePointer.argtypes = [C.c_void_p, C.c_long, C.POINTER(C.c_long), C.c_ulong]
        xapi.SetFilePointer.restype = C.c_ulong
        xapi.SetEndOfFile.argtypes = [C.c_void_p]
        xapi.GetFileSize.argtypes = [C.c_void_p, C.POINTER(C.c_ulong)]
        xapi.GetFileSize.restype = C.c_ulong
        for function in (xapi.ReadFile, xapi.WriteFile):
            function.argtypes = [C.c_void_p, C.c_void_p, C.c_ulong,
                                 C.POINTER(C.c_ulong), C.c_void_p]
        filename = b'z:\\saved\\profiles\\Name.bin'
        invalid = C.c_void_p(-1).value
        assert xapi.CreateFileA(filename, 0xC0000000, 0, None, 3, 0, None) == invalid
        handle = xapi.CreateFileA(filename, 0xC0000000, 0, None, 1, 0, None)
        assert handle != invalid
        assert xapi.CreateFileA(filename, 0xC0000000, 0, None, 1, 0, None) == invalid
        count = C.c_ulong()
        assert xapi.WriteFile(handle, C.create_string_buffer(b'abcdef'), 6, C.byref(count), None)
        assert count.value == 6
        assert xapi.SetFilePointer(handle, 2, None, 0) == 2
        assert xapi.WriteFile(handle, C.create_string_buffer(b'XY'), 2, C.byref(count), None)
        assert xapi.SetFilePointer(handle, 0, None, 0) == 0
        buffer = C.create_string_buffer(16)
        assert xapi.ReadFile(handle, buffer, 16, C.byref(count), None)
        assert count.value == 6 and buffer.raw[:6] == b'abXYef'
        assert xapi.ReadFile(handle, buffer, 16, C.byref(count), None) and count.value == 0
        assert not xapi.ReadFile(handle, buffer, 1, C.byref(count), C.c_void_p(1))
        assert count.value == 0 and xapi.vita_xapi_last_error_get() == 87
        assert xapi.SetFilePointer(handle, 3, None, 0) == 3 and xapi.SetEndOfFile(handle)
        high = C.c_ulong(123)
        assert xapi.GetFileSize(handle, C.byref(high)) == 3 and high.value == 0
        assert xapi.CloseHandle(handle)
        handle = xapi.CreateFileA(filename.upper(), 0x80000000, 0, None, 3, 0, None)
        assert handle != invalid and xapi.CloseHandle(handle)
        assert (data / 'z/saved/profiles/Name.bin').read_bytes() == b'abX'
        xapi.vita_xapi_directory_open.argtypes = [C.c_char_p]
        xapi.vita_xapi_directory_next.argtypes = [C.c_int, C.c_void_p,
                                                 C.POINTER(C.c_uint32), C.POINTER(C.c_uint64)]
        directory = xapi.vita_xapi_directory_open(b'z:\\saved\\profiles')
        assert directory >= 0
        name, flags, size = C.create_string_buffer(256), C.c_uint32(), C.c_uint64()
        assert xapi.vita_xapi_directory_next(directory, name, C.byref(flags), C.byref(size))
        assert name.value == b'Name.bin' and flags.value == 128 and size.value == 3
        assert not xapi.vita_xapi_directory_next(directory, name, C.byref(flags), C.byref(size))
        assert xapi.vita_xapi_last_error_get() == 18
        xapi.vita_xapi_directory_close(directory)
        xapi.MoveFileA.argtypes = [C.c_char_p, C.c_char_p]
        renamed = b'z:\\saved\\profiles\\renamed.bin'
        assert xapi.MoveFileA(filename, renamed)
        assert not xapi.MoveFileA(renamed, renamed) and xapi.vita_xapi_last_error_get() == 183
        xapi.GetDiskFreeSpaceExA.argtypes = [C.c_char_p, C.c_void_p, C.c_void_p, C.c_void_p]
        available, total, free = C.c_uint64(), C.c_uint64(), C.c_uint64()
        assert xapi.GetDiskFreeSpaceExA(b'z:\\', C.byref(available), C.byref(total), C.byref(free))
        assert 0 < available.value <= total.value and free.value == available.value
        xapi.DeleteFileA.argtypes = [C.c_char_p]
        xapi.RemoveDirectoryA.argtypes = [C.c_char_p]
        assert not xapi.RemoveDirectoryA(b'z:\\saved\\profiles')
        assert xapi.DeleteFileA(renamed) and xapi.RemoveDirectoryA(b'z:\\saved\\profiles')
        print('PASS: actual Xbox path/TLS, save directory/create/casefold, counted I/O/EOF, seek/overwrite/truncate, enumerate/rename/delete/free-space contracts')


if __name__ == '__main__':
    main()
