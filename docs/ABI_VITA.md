# Native Vita ABI audit

Compiler: existing `/usr/local/vitasdk-hardfp/bin/arm-vita-eabi-gcc` 15.2.0. SDK detection uses compiler macros (`__ARM_PCS_VFP`, pointer-size4), not folder naming. The other `/usr/local/vitasdk` defaults to SoftFP and is rejected by tools/vita_build.py.

| Contract | Handling / evidence | Remaining limit |
|---|---|---|
| Pointers, long, Xbox DWORD | 4 bytes; compile assertions in halo_core.c; final ELF32 ARM EABI5 hard-float | Full Xbox address-window semantics not implemented |
| wchar_t | Game 16-bit via -fshort-wchar; native libc/SDL3/vitaGL SDK wchar retained; narrow bridge only | Full MSVC wide-string layer not yet linked; no native wcs* calls allowed for game data |
| enums / signed char | Game -fno-short-enums and -fsigned-char; enum32 assertion | SDK variable-size enum warning remains visible; never pass SDK enum-containing structs across bridge |
| Packing / alignment | Existing upstream size/offset typedef asserts pass in all 466 units; 64-bit member offset8 assertion, data-array0x38, pool0x38/block0x18, cache-header0x800/footer0x7FC, XInput layout asserted | Not an exhaustive field-by-field proof for all tags/bitfields |
| MS extensions / declspec / Xbox XDK | Existing public upstream XDK declarations, generated forward tags/weak inline copies, Vita-only GCC attribute mappings | GCC inline linkage differs from clang; final verifier rejects missing game weak hooks |
| Struct / union returns | Native AAPCS32 used; 4-byte aggregate results use core registers; larger aggregates use hidden result pointers | Do not import x86 -freg-struct-return; reconstructed function-pointer signatures still require review before full main |
| Variadics / prototypes | Real va_list in terminal_printf; upstream Android variadic declarations force-included; ui error prototype corrected to actual short/const signature; MSVC printf I64/I32 conversion reused | Complete systematic prototype comparison not yet run; android_abi_check.py requires LLVM IR, this build is GCC |
| Floating-point register classes | Hard-float consistently selected for game/native libraries; known hs.c fade float signature fix reused from Android | Successful compiling is not proof every reconstructed function declaration has the right register classes |
| x86 inline asm / intrinsics | Existing HALO_LINUX portable branches compile under GCC; byte-swap uses builtin64; selected profiler uses native clock, not RDTSC | Full _controlfp/FPU-exception emulation and x86-context diagnostics remain outside linked core |
| Frame pointers / stack walking | -fno-omit-frame-pointer; no upstream EBP walker in executable; explicit logged diagnostic limitation | Proper ARM/Thumb unwind and crash context integration pending; retain ELF/map/dump |
| Aliasing / integer overflow | -fno-strict-aliasing, -fwrapv, -fno-delete-null-pointer-checks | Does not fix every legacy UB/shift/unaligned-load assumption |
| FP contraction / deterministic math | -ffp-contract=off; selected core does not substitute fast-math or Android buffers | Full game must retain upstream musl-math deterministic functions and compare ARM/x86 results before multiplayer |
| CRT/native boundary | Actual Vita/newlib APIs compile in normal SDK units; XDK-facing services use typed declarations | GNU ld's wchar/enum attribute warnings are expected and retained, not suppressed; wide values never cross current bridge |

Full-source evidence: `build/vita/audit-abi/report.json`, all 466 per-unit commands/logs and `HaloGame.audit.o`. The relocatable object has 584 unresolved game-to-CRT/platform imports; it is not a runnable full game. The core ELF passes an independent symbol/import/ABI/package verifier.
