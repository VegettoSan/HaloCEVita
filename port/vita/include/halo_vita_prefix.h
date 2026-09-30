/* Native ARM32 adaptation of the upstream MSVC/XDK compile environment. */
#ifndef HALO_VITA_PREFIX_H
#define HALO_VITA_PREFIX_H
/* Read newlib before MSVC inline/type macros. _WCHAR_T_DEFINED would
 * otherwise suppress GCC's wchar_t typedef, and newlib uses static __inline. */
#include <stddef.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
/* newlib already supplies wint_t (32 bits); game storage uses wchar_t. */
#define __wint_t_defined 1
#include <wchar.h>
#include <ctype.h>
/* hs.c declares a function with this standard name. */
#undef isspace
#undef LONG_MAX
#undef LONG_MIN
#undef CHAR_MAX
#undef CHAR_MIN
#undef CHAR_BIT
#define HALO_VITA 1
#undef __fastcall
#undef FD_SETSIZE
#define __cdecl
#define __stdcall
#define __fastcall
#define __int8 char
#define __int16 short
#define __int32 int
#define __int64 long long
#define HALO_DECLSPEC_align(n) __attribute__((aligned(n)))
#define HALO_DECLSPEC_selectany __attribute__((weak))
#define HALO_DECLSPEC_noreturn __attribute__((noreturn))
#define HALO_DECLSPEC_noinline __attribute__((noinline))
#define HALO_DECLSPEC_naked __attribute__((naked))
#define HALO_DECLSPEC_dllimport
#define HALO_DECLSPEC_dllexport
#define HALO_DECLSPEC_novtable
#define __declspec(x) HALO_DECLSPEC_##x
#include "../../linux/include/halo_linux_prefix.h"
/* GCC rejects clang's tolerated `static static` and the change from a
 * prototype's external linkage to static inline. gnu89 inline definitions
 * instead export copies; the generated #pragma weak list provides COMDAT. */
#undef __inline
#undef _inline
#undef __forceinline
#define __inline __inline__
#define _inline __inline__
#define __forceinline __inline__ __attribute__((always_inline))
/* ARM hard-float must see the real variadic ABI, even at call sites
 * reconstructed without declarations. Reuse the upstream ARM audit fixes. */
#include "../../android/include/halo_android_variadic_prototypes.h"
#endif
