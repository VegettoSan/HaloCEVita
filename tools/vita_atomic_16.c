/* MIT License. Copyright (c) 2026 VegettoSanDev.
 * VitaSDK 2026.08 does not ship libatomic.a. Clang on Ubuntu 24.04 emits
 * these two helpers for the donor's relaxed 16-bit light-flag updates.
 * Cortex-A9 has halfword exclusives; keep the donor's atomic operation.
 */
#include <stdint.h>

uint16_t halo_vita_fetch_or_2(volatile void *address, uint16_t mask, int order)
    __asm__("__atomic_fetch_or_2");
uint16_t halo_vita_fetch_and_2(volatile void *address, uint16_t mask, int order)
    __asm__("__atomic_fetch_and_2");

uint16_t halo_vita_fetch_or_2(volatile void *address, uint16_t mask, int order)
{
    uint32_t old, next, retry;
    if (((uintptr_t)address & 1u) || order != 0) __builtin_trap();
    __asm__ volatile(
        "1: ldrexh %[old], [%[ptr]]\n"
        "orr %[next], %[old], %[mask]\n"
        "strexh %[retry], %[next], [%[ptr]]\n"
        "cmp %[retry], #0\n"
        "bne 1b\n"
        : [old] "=&r" (old), [next] "=&r" (next), [retry] "=&r" (retry)
        : [ptr] "r" (address), [mask] "r" ((uint32_t)mask)
        : "cc", "memory");
    return (uint16_t)old;
}

uint16_t halo_vita_fetch_and_2(volatile void *address, uint16_t mask, int order)
{
    uint32_t old, next, retry;
    if (((uintptr_t)address & 1u) || order != 0) __builtin_trap();
    __asm__ volatile(
        "1: ldrexh %[old], [%[ptr]]\n"
        "and %[next], %[old], %[mask]\n"
        "strexh %[retry], %[next], [%[ptr]]\n"
        "cmp %[retry], #0\n"
        "bne 1b\n"
        : [old] "=&r" (old), [next] "=&r" (next), [retry] "=&r" (retry)
        : [ptr] "r" (address), [mask] "r" ((uint32_t)mask)
        : "cc", "memory");
    return (uint16_t)old;
}
