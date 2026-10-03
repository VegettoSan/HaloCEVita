/* Original Win32 issuing-thread completion semantics over native Vita I/O. */
#include "platform.h"
#include <stdint.h>
#include <string.h>
int vita_xapi_read_at(void *,void *,uint32_t,uint64_t,uint32_t *);
int vita_xapi_write_at(void *,const void *,uint32_t,uint64_t,uint32_t *);
int vita_xapi_fd_times(void *,uint64_t [3]);
int vita_xapi_set_fd_times(void *,const uint64_t *,const uint64_t *,const uint64_t *);
static void file_completion_apc(void *routine,void *overlapped,void *unused)
{
    LPOVERLAPPED request = overlapped;
    (void)unused;
    ((LPOVERLAPPED_COMPLETION_ROUTINE)routine)((DWORD)request->Internal,
        (DWORD)request->InternalHigh,request);
}
BOOL WINAPI ReadFileEx(HANDLE handle,LPVOID buffer,DWORD count,
    LPOVERLAPPED overlapped,LPOVERLAPPED_COMPLETION_ROUTINE routine)
{
    uint32_t done = 0;
    uint64_t offset;
    BOOL result;
    if (!overlapped || !routine) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    offset = ((uint64_t)overlapped->OffsetHigh << 32) | overlapped->Offset;
    result = vita_xapi_read_at(handle,buffer,count,offset,&done);
    overlapped->Internal = result ? ERROR_SUCCESS : GetLastError();
    if (result && !done && count) overlapped->Internal = ERROR_HANDLE_EOF;
    overlapped->InternalHigh = done;
    platform_queue_apc(file_completion_apc,(void *)routine,overlapped,NULL);
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}
BOOL WINAPI WriteFileEx(HANDLE handle,LPCVOID buffer,DWORD count,
    LPOVERLAPPED overlapped,LPOVERLAPPED_COMPLETION_ROUTINE routine)
{
    uint32_t done = 0;
    uint64_t offset;
    BOOL result;
    if (!overlapped || !routine) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    offset = ((uint64_t)overlapped->OffsetHigh << 32) | overlapped->Offset;
    result = vita_xapi_write_at(handle,buffer,count,offset,&done);
    overlapped->Internal = result ? ERROR_SUCCESS : GetLastError();
    overlapped->InternalHigh = done;
    platform_queue_apc(file_completion_apc,(void *)routine,overlapped,NULL);
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}
BOOL WINAPI GetFileTime(HANDLE handle,LPFILETIME creation,LPFILETIME access,LPFILETIME write)
{
    uint64_t times[3];
    if (!vita_xapi_fd_times(handle,times)) return FALSE;
    if (creation) memcpy(creation,&times[0],8);
    if (access) memcpy(access,&times[1],8);
    if (write) memcpy(write,&times[2],8);
    return TRUE;
}
BOOL WINAPI SetFileTime(HANDLE handle,const FILETIME *creation,const FILETIME *access,const FILETIME *write)
{
    uint64_t a,b,c;
    if (creation) memcpy(&a,creation,8);
    if (access) memcpy(&b,access,8);
    if (write) memcpy(&c,write,8);
    return vita_xapi_set_fd_times(handle,creation?&a:NULL,access?&b:NULL,write?&c:NULL);
}
