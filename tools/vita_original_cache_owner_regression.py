#!/usr/bin/env python3
"""Execute original slot selection/open/request/completion owners on host types."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def definition(source, name):
    pattern = (r'(?m)^(?:static\s+)?(?:struct\s+\w+\s*\*\s*|'
               r'(?:void|boolean|short|long)(?:\s+CALLBACK)?\s+)'
               + re.escape(name) + r'\s*\([^;{}]*\)\s*\{')
    match = re.search(pattern, source)
    assert match, name
    start = match.start()
    brace = source.index('{', match.start())
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


def main():
    source = (ROOT / 'source/cache/cache_files_windows.c').read_text()
    header = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
typedef void *HANDLE;
typedef unsigned char boolean, byte;
typedef uint32_t DWORD;
typedef uint64_t FILETIME;
typedef struct { DWORD Internal,InternalHigh,Offset,OffsetHigh; HANDLE hEvent; } OVERLAPPED;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define CALLBACK
#define ERROR_SUCCESS 0
#define ERROR_ALREADY_EXISTS 183
#define GENERIC_READ 1
#define GENERIC_WRITE 2
#define OPEN_ALWAYS 4
#define FILE_FLAG_NO_BUFFERING 8
#define FILE_FLAG_OVERLAPPED 16
#define INVALID_SET_FILE_POINTER -1
#define FILE_BEGIN 0
#define CACHE_FILE_BUILD_STRING "01.01.14.2342"
#define INVALID_HANDLE_VALUE ((void *)-1)
#define _stricmp strcasecmp
#define match_assert(f,l,c) assert(c)
#define match_vassert(f,l,c,m) assert(c)
enum { NUMBER_OF_CACHED_MAP_FILES=6, MAXIMUM_SIMULTANEOUS_CACHE_REQUESTS=512,
 CACHE_FILE_SECTOR_SIZE=512, SOLO_CACHE_FILE_MAXIMUM_SIZE=0x11600000,
 MAIN_MENU_CACHE_FILE_MAXIMUM_SIZE=0x02300000, MULTIPLAYER_CACHE_FILE_MAXIMUM_SIZE=0x02F00000,
 _scenario_type_solo=0,_scenario_type_multiplayer=1,_scenario_type_main_menu=2 };
struct cache_file_header {long file_length,tag_data_size; char name[32],build[32];unsigned checksum;};
'''
    a = source.index('struct cached_map_file\n')
    b = source.index('typedef BOOL', a)
    structs = source[a:b]
    a = source.index('struct cache_file_runtime_globals\n')
    b = source.index('typedef char verify_', a)
    structs += source[a:b] + 'static struct cache_file_runtime_globals cache_file_globals;\n'
    helpers = r'''
static int wakes;
static void vita_log(const char *format,...){(void)format;}
static long CompareFileTime(const FILETIME *a,const FILETIME *b){return (*a>*b)-(*a<*b);}
static void cache_file_windows_thread_wake(void){++wakes;}
static short cache_request_next_free_index(void){return 0;}
static struct cache_file_request *cache_request_get(short i){return &cache_file_globals.requests[i];}

static struct cache_file_header disk_headers[6], dvd_header;
static long cached_map_file_get_size(short i);
static void cached_map_files_delete(short i){(void)i;}
static void cached_map_file_get_path(short i,char *path){sprintf(path,"z:\\cache%03d.map",i);}
static HANDLE CreateFileA(const char *path,long access,long share,void *sec,long creation,long flags,HANDLE tpl){return (HANDLE)(uintptr_t)(atoi(strstr(path,"cache")+5)+1);}
static long GetLastError(void){return ERROR_ALREADY_EXISTS;}
static long GetFileSize(HANDLE file,void *high){return cached_map_file_get_size((short)((uintptr_t)file-1));}
static long SetFilePointer(HANDLE file,long offset,void *high,long origin){return offset;}
static boolean SetEndOfFile(HANDLE file){return TRUE;}
static void CloseHandle(HANDLE file){(void)file;}
static void cached_map_file_read_header(short i){cache_file_globals.cached_map_files[i].header=disk_headers[i];}
static boolean cache_file_read_header_from_dvd(const char *name,struct cache_file_header *header){if(strcmp(name,"ui"))return FALSE;*header=dvd_header;return TRUE;}
static void cache_file_blocking_io_completion_routine(unsigned long e,unsigned long n,OVERLAPPED *o){*(volatile boolean *)o->hEvent=TRUE;}
static void cached_map_issue_async_write(HANDLE f,OVERLAPPED *o,void *b,long n,long offset,volatile boolean *done,void (*cb)(unsigned long,unsigned long,OVERLAPPED *)){o->hEvent=(HANDLE)done;cb(0,n,o);}
static void cached_map_block_on_async_request(volatile boolean *done){assert(*done);}
'''
    names = ['cached_map_file_get', 'cached_map_file_get_size',
             'cached_map_files_find_free_map', 'cached_map_files_find_map',
             'cache_file_open', 'cache_file_read', 'cache_file_read_io_completion_routine',
             'cache_files_open_cache_files']
    functions = ''.join(definition(source, name) for name in names)
    test = r'''
int main(void){
 struct cache_file_request requests[512];struct cache_file_header header;
 cache_file_globals.requests=requests;cache_file_globals.open_map_file_index=NONE;
 strcpy(dvd_header.name,"ui");strcpy(dvd_header.build,"01.10.12.2276");dvd_header.checksum=0x123456;
 disk_headers[2]=dvd_header;
 cache_files_open_cache_files();
 assert((cache_file_globals.cached_map_files[2].header.name[0]!=0)==WARM_ALLOWED);
 if(WARM_ALLOWED)assert(!strcmp(cache_file_globals.cached_map_files[2].header.build,dvd_header.build));
 disk_headers[2].checksum++;
 cache_files_open_cache_files();
 assert(cache_file_globals.cached_map_files[2].header.name[0]==0);

 for(int i=0;i<6;i++)cache_file_globals.cached_map_files[i].last_modification_date=i+1;
 assert(cached_map_files_find_free_map(4096,_scenario_type_main_menu)==2);
 assert(cached_map_files_find_free_map(4096,_scenario_type_solo)==0);
 cache_file_globals.open_map_file_index=0;
 assert(cached_map_files_find_free_map(4096,_scenario_type_solo)==1);
 cache_file_globals.open_map_file_index=3;
 cache_file_globals.cached_map_files[4].last_modification_date=100;
 assert(cached_map_files_find_free_map(4096,_scenario_type_multiplayer)==5);
 cache_file_globals.open_map_file_index=NONE;
 strcpy(cache_file_globals.cached_map_files[2].header.name,"ui");
 strcpy(cache_file_globals.cached_map_files[2].header.build,"01.10.12.2276");
 cache_file_globals.cached_map_files[2].header.checksum=0xabcdef;
 assert(cached_map_files_find_map("UI")==2 && cached_map_files_find_map("missing")==NONE);
 assert(cache_file_open("UI",&header));
 assert(header.checksum==0xabcdef && !strcmp(header.build,"01.10.12.2276"));
 assert(cache_file_globals.open_map_file_index==2);
 byte bytes[1024];boolean complete=TRUE;
 assert(cache_file_read(NONE,2048,513,bytes,&complete,FALSE)==0);
 assert(!complete && wakes==1 && requests[0].pending && !requests[0].running);
 assert(requests[0].size==1024 && requests[0].overlapped.Offset==2048);
 assert(requests[0].overlapped.hEvent==&complete && requests[0].buffer==bytes);
 requests[0].running=TRUE;
 cache_file_read_io_completion_routine(0,1024,&requests[0].overlapped);
 assert(complete && !requests[0].pending && !requests[0].running);
 return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='halo-cache-owners-') as tmp:
        work = Path(tmp)
        (work / 'test.c').write_text(header + structs + helpers + functions + test)
        for native in (False, True):
            flags = ['-DWARM_ALLOWED=' + str(int(native)), '-DHALO_VITA']
            if native: flags.append('-DHALO_VITA_ORIGINAL_RUNTIME')
            subprocess.run(['gcc', '-std=c11', '-Wno-unused-parameter', *flags, str(work / 'test.c'), '-o', str(work / 'test')], check=True)
            subprocess.run([str(work / 'test')], check=True, timeout=10)
    print('PASS: actual Halo six-slot/LRU policy, active-slot protection, native header preservation, native warm-slot recovery/checksum rejection, original request queue/sector rounding and completion ownership')


if __name__ == '__main__':
    main()
