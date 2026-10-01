/* Reuse the native port's real pthread handle/event/mutex/thread services.
 * Raw Vita file descriptors remain owned by vita_xapi_files.c; its CloseHandle
 * routes heap-backed objects to this implementation. */
#define CloseHandle halo_vita_kernel_CloseHandle
#include "../../linux/src/xbox_kernel.c"
#undef CloseHandle
