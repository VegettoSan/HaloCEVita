/* Compile with VitaSDK GCC, separately from the engine-facing ABI probe. */
#include <stddef.h>
#include <wchar.h>
#include "vita_host.h"
#include "vita_gl_host.h"

enum abi_enum { abi_enum_zero, abi_enum_one };
_Static_assert(sizeof(void *) == 4 && sizeof(long) == 4, "Vita host must be ARM32");
_Static_assert(sizeof(wchar_t) == 4, "SDK wide character ABI must be preserved");
_Static_assert(sizeof(enum abi_enum) == 1, "SDK uses compact enums");
_Static_assert(sizeof(struct vita_host_pad) == 8, "pad boundary size changed");
_Static_assert(offsetof(struct vita_host_pad, lx) == 4, "pad boundary layout changed");
