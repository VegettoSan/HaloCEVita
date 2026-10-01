/* XDK directory records for the original saved-game owners. Native SDK
 * layouts stay across the scalar-only boundary in vita_xapi_files.c. */
#include "platform.h"
#include "vita_runtime.h"
#include <string.h>
#include <ctype.h>

extern int vita_xapi_directory_open(const char *name);
extern int vita_xapi_directory_next(int fd, char *name, uint32_t *attributes, uint64_t *size);
extern void vita_xapi_directory_close(int fd);

struct vita_file_search { int fd; char pattern[256]; };
static int wildcard_match(const char *p,const char *s)
{
    const char *star=NULL,*retry=NULL;
    while(*s) {
        if(*p=='?' || tolower((unsigned char)*p)==tolower((unsigned char)*s)) {p++;s++;}
        else if(*p=='*') {star=p++;retry=s;}
        else if(star) {p=star+1;s=++retry;}
        else return FALSE;
    }
    while(*p=='*') p++;
    return !*p;
}
static void search_destroy(struct platform_handle *handle)
{
    struct vita_file_search *search=handle->data;
    vita_xapi_directory_close(search->fd);free(search);
}
static BOOL search_next(struct vita_file_search *search,WIN32_FIND_DATAA *data)
{
    char name[256];uint32_t attributes;uint64_t size;
    while(vita_xapi_directory_next(search->fd,name,&attributes,&size)) {
        if(!wildcard_match(search->pattern,name)) continue;
        memset(data,0,sizeof(*data));
        data->dwFileAttributes=attributes;
        data->nFileSizeLow=(DWORD)size;data->nFileSizeHigh=(DWORD)(size>>32);
        strcpy(data->cFileName,name);SetLastError(0);return TRUE;
    }
    return FALSE;
}
HANDLE WINAPI FindFirstFileA(LPCSTR pattern,LPWIN32_FIND_DATAA data)
{
    char directory[512];char *leaf;struct vita_file_search *search;struct platform_handle *handle;
    if(!pattern || !data || strlen(pattern)>=sizeof(directory)) {SetLastError(ERROR_INVALID_PARAMETER);return INVALID_HANDLE_VALUE;}
    strcpy(directory,pattern);leaf=strrchr(directory,'\\');
    {char *slash=strrchr(directory,'/');if(slash && (!leaf || slash>leaf))leaf=slash;}
    if(!leaf || !leaf[1]) {SetLastError(ERROR_INVALID_PARAMETER);return INVALID_HANDLE_VALUE;}
    search=calloc(1,sizeof(*search));if(!search){SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE;}
    strcpy(search->pattern,leaf+1);leaf[1]=0;
    search->fd=vita_xapi_directory_open(directory);
    if(search->fd<0 || !search_next(search,data)) {
        if(search->fd>=0)vita_xapi_directory_close(search->fd);free(search);return INVALID_HANDLE_VALUE;
    }
    handle=platform_handle_new(_platform_handle_find,search,search_destroy);
    if(!handle){vita_xapi_directory_close(search->fd);free(search);SetLastError(ERROR_NOT_ENOUGH_MEMORY);return INVALID_HANDLE_VALUE;}
    return handle;
}
BOOL WINAPI FindNextFileA(HANDLE find,LPWIN32_FIND_DATAA data)
{
    struct platform_handle *handle=platform_handle_get(find,_platform_handle_find);
    if(!handle || !data) {SetLastError(ERROR_INVALID_HANDLE);return FALSE;}
    return search_next(handle->data,data);
}
BOOL WINAPI FindClose(HANDLE find) {return CloseHandle(find);}
