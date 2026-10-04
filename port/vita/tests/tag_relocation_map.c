/* Real-cache regression for the directly imported donor walker.
 * Build and run as 32-bit code; input is a decompressed user-supplied Xbox map.
 * Only a private memory copy is relocated. This is not an engine/render test.
 */
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tag_relocate.h"

_Static_assert(sizeof(void *) == 4 && sizeof(long) == 4, "run with the 32-bit ABI");
#define XBOX_BASE 0x803A6000u
#define WINDOW_SIZE 0x01600000u

void platform_log(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
}

static uint32_t word(const unsigned char *at)
{
    uint32_t value;
    memcpy(&value, at, sizeof(value));
    return value;
}

static unsigned char *original_pointer(unsigned char *snapshot, uint32_t value, uint32_t size)
{
    assert(value >= XBOX_BASE && value - XBOX_BASE < size);
    return snapshot + value - XBOX_BASE;
}

int main(int argc, char **argv)
{
    FILE *file;
    unsigned char header[2048], *window, *snapshot, *instances;
    uint32_t offset, size, count, i, bias, changed = 0, strings = 0, widgets = 0, bitmaps = 0, bsps = 0;
    assert(argc == 2);
    file = fopen(argv[1], "rb");
    assert(file && fread(header, 1, sizeof(header), file) == sizeof(header));
    assert(word(header) == 0x68656164 && word(header + 4) == 5);
    offset = word(header + 16);
    size = word(header + 20);
    assert(size >= 40 && size <= WINDOW_SIZE);
    window = calloc(1, WINDOW_SIZE);
    snapshot = malloc(size);
    assert(window && snapshot && !fseek(file, offset, SEEK_SET));
    assert(fread(window, 1, size, file) == size);
    memcpy(snapshot, window, size);
    count = word(snapshot + 12);
    instances = original_pointer(snapshot, word(snapshot), size);
    assert(count > 0 && (uint32_t)(instances - snapshot) + count * 32 <= size);
    bias = (uint32_t)(uintptr_t)window - XBOX_BASE;
    setenv("HALO_TAG_RELOCATION_REPORT", "1", 1);
    halo_tag_relocate_tags(window, size);

    /* Every mutation must be the relocation of a window address. All other
       words (including packed data that resemble pointers) must be intact. */
    for (i = 0; i + 4 <= size; i += 4) {
        uint32_t old = word(snapshot + i), now = word(window + i);
        if (old != now) {
            assert(old >= XBOX_BASE && old - XBOX_BASE < WINDOW_SIZE);
            assert(now == old + bias);
            changed++;
        }
    }
    for (i = 0; i < count; i++) {
        unsigned char *old = instances + i * 32;
        unsigned char *now = window + (old - snapshot);
        uint32_t name = word(old + 16), root = word(old + 20), group = word(old);
        assert(word(now) == group && word(now + 12) == word(old + 12));
        assert(word(now + 16) == name + bias);
        assert(!strcmp((const char *)(uintptr_t)word(now + 16),
                       (const char *)original_pointer(snapshot, name, size)));
        if (!root) continue; /* structure BSP loaded separately */
        assert(word(now + 20) == root + bias);
        if (group == 0x44654C61u) {
            unsigned char *a = original_pointer(snapshot, root, size);
            unsigned char *b = (unsigned char *)(uintptr_t)word(now + 20);
            /* Original widget type/name/bounds and flags precede pointers. */
            assert(!memcmp(a, b, 0x38));
            widgets++;
        }
        if (group == 0x6269746Du) bitmaps++;
        if (group == 0x73636E72u) {
            unsigned char *a = original_pointer(snapshot, root, size);
            unsigned char *b = (unsigned char *)(uintptr_t)word(now + 20);
            /* scenario_definitions.h/tag layouts: BSP reference block at
               0x5A4; each reference is 0x20 bytes (offset, size, address). */
            uint32_t references = word(a + 0x5A4), j;
            if (references) {
                unsigned char *refs = original_pointer(snapshot, word(a + 0x5A8), size);
                unsigned char *relocated_refs = (unsigned char *)(uintptr_t)word(b + 0x5A8);
                assert(word(b + 0x5A4) == references);
                for (j = 0; j < references; j++) {
                    unsigned char *ref = refs + j * 0x20;
                    uint32_t file_offset = word(ref), bytes = word(ref + 4), address = word(ref + 8), k;
                    unsigned char *destination, *raw, *first;
                    assert(address >= XBOX_BASE && address - XBOX_BASE >= size);
                    assert(bytes && bytes <= WINDOW_SIZE && address - XBOX_BASE + bytes <= WINDOW_SIZE);
                    destination = window + address - XBOX_BASE;
                    assert(word(relocated_refs + j * 0x20 + 8) == (uint32_t)(uintptr_t)destination);
                    raw = malloc(bytes);
                    first = malloc(bytes);
                    assert(raw && first && !fseek(file, file_offset, SEEK_SET));
                    assert(fread(raw, 1, bytes, file) == bytes);
                    memcpy(destination, raw, bytes);
                    halo_tag_relocate_structure_bsp(window, destination, bytes);
                    for (k = 0; k + 4 <= bytes; k += 4) {
                        uint32_t before = word(raw + k), after = word(destination + k);
                        if (before != after) {
                            assert(before >= XBOX_BASE && before - XBOX_BASE < WINDOW_SIZE);
                            assert(after == before + bias);
                        }
                    }
                    memcpy(first, destination, bytes);
                    /* Original load behavior: same BSP can be reread into the
                       same slot. A second relocation must not double the bias. */
                    memcpy(destination, raw, bytes);
                    halo_tag_relocate_structure_bsp(window, destination, bytes);
                    assert(!memcmp(first, destination, bytes));
                    free(first);
                    free(raw);
                    bsps++;
                }
            }
        }
        if (group == 0x75737472u) {
            unsigned char *a = original_pointer(snapshot, root, size);
            unsigned char *b = (unsigned char *)(uintptr_t)word(now + 20);
            uint32_t entries = word(a), j;
            unsigned char *old_entries, *new_entries;
            assert(word(b) == entries);
            if (!entries) continue;
            old_entries = original_pointer(snapshot, word(a + 4), size);
            new_entries = (unsigned char *)(uintptr_t)word(b + 4);
            for (j = 0; j < entries; j++) {
                uint32_t bytes = word(old_entries + j * 20);
                uint32_t text = word(old_entries + j * 20 + 12);
                if (!bytes) continue;
                assert(bytes < size && text - XBOX_BASE + bytes <= size);
                assert(word(new_entries + j * 20) == bytes);
                assert(word(new_entries + j * 20 + 12) == text + bias);
                assert(!memcmp(original_pointer(snapshot, text, size),
                               (void *)(uintptr_t)(text + bias), bytes));
                strings++;
            }
        }
    }
    assert(changed && widgets && bitmaps && strings);
    printf("PASS %s: %u tag identities/names, %u widgets unchanged, %u bitmap tags, %u UTF-16 strings unchanged, %u relocated words, %u BSP loads/reloads\n",
           argv[1], count, widgets, bitmaps, strings, changed, bsps);
    fclose(file);
    free(snapshot);
    free(window);
    return 0;
}
