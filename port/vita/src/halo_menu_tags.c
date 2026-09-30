/* Game ABI side of the menu-data boundary. Use the same accessors that
 * ui_widget.c uses; never emulate widget initialization or rendering. */
#include "cseries.h"
#include "vita_runtime.h"
#include "halo_vita_cache.h"
#include "interface/ui_widget_tags.h"
#include "cache/cache_files.h"
#include "bitmaps/bitmap_group.h"
#include "text/font_group.h"
#include "text/text_group.h"

#define CHECK_LAYOUT(name, expression) typedef char name[(expression) ? 1 : -1]
CHECK_LAYOUT(menu_bitmap_size, sizeof(struct bitmap_group) == 108);
CHECK_LAYOUT(menu_bitmap_sequence, offsetof(struct bitmap_group, sequences) == 84);
CHECK_LAYOUT(menu_bitmap_data, offsetof(struct bitmap_group, bitmaps) == 96);
CHECK_LAYOUT(menu_bitmap_resource_size, sizeof(struct bitmap_data) == 48);
CHECK_LAYOUT(menu_sequence_sprites, offsetof(struct bitmap_group_sequence, sprites) == 52);
CHECK_LAYOUT(menu_font_size, sizeof(struct font_header) == 156);
CHECK_LAYOUT(menu_font_table, offsetof(struct font_header, character_tables) == 48);
CHECK_LAYOUT(menu_font_styles, offsetof(struct font_header, style_fonts) == 60);
CHECK_LAYOUT(menu_font_chars, offsetof(struct font_header, characters) == 124);
CHECK_LAYOUT(menu_font_pixels, offsetof(struct font_header, pixels) == 136);
CHECK_LAYOUT(menu_string_entry, sizeof(struct string_list_entry) == 20);
CHECK_LAYOUT(menu_tag_data_address, offsetof(struct tag_data, address) == 12);
CHECK_LAYOUT(menu_tag_reference_index, offsetof(struct tag_reference, index) == 12);
CHECK_LAYOUT(menu_tag_block_address, offsetof(struct tag_block, address) == 4);

int halo_vita_menu_tags_probe(uint32_t menu_index, const struct vita_menu_stats *stats)
{
    struct ui_widget_definition *menu;
    struct tag_iterator iterator;
    long index, bitmaps = 0, strings = 0, fonts = 0;
    long found = tag_loaded('DeLa', "ui\\shell\\main_menu\\main_menu");
    if (found == NONE || (uint32_t)found != menu_index) {
        vita_log("menu original tag_loaded FAILED"); return 0;
    }
    menu = tag_get('DeLa', found);
    if (menu->child_widgets.count) {
        struct ui_widget_child_reference *child = TAG_BLOCK_GET_ELEMENT(
            &menu->child_widgets, 0, struct ui_widget_child_reference);
        if (child->widget_tag.index != NONE && !tag_get('DeLa', child->widget_tag.index)) return 0;
    }
    vita_log("[VITA 018] original Main Menu tag resolved: datum=%08lx type=%d children=%ld conditional=%ld reachable=%u; widget state not initialized",
        (unsigned long)found, menu->type, menu->child_widgets.count,
        menu->conditional_widgets.count, stats->menu_widgets);
    tag_iterator_new(&iterator, NONE);
    while ((index = tag_iterator_next(&iterator)) != NONE) {
        long group = tag_get_group_tag(index);
        if (group == BITMAP_GROUP_TAG) {
            struct bitmap_group *bitmap = bitmap_group_get(index);
            if (bitmap->bitmaps.count) {
                struct bitmap_data *expected = TAG_BLOCK_GET_ELEMENT(&bitmap->bitmaps, 0, struct bitmap_data);
                if (bitmap->sequences.count) {
                    struct bitmap_group_sequence *sequence = TAG_BLOCK_GET_ELEMENT(&bitmap->sequences, 0, struct bitmap_group_sequence);
                    if (sequence->bitmap_count > 0) {
                        if (sequence->first_bitmap_index < 0 || sequence->first_bitmap_index >= bitmap->bitmaps.count) return 0;
                        expected = TAG_BLOCK_GET_ELEMENT(&bitmap->bitmaps, sequence->first_bitmap_index, struct bitmap_data);
                    } else if (sequence->sprites.count) {
                        struct bitmap_group_sprite *sprite = TAG_BLOCK_GET_ELEMENT(&sequence->sprites, 0, struct bitmap_group_sprite);
                        /* Original accessor falls back to frame0 for NONE. */
                        if (sprite->bitmap_index != NONE) {
                            if (sprite->bitmap_index < 0 || sprite->bitmap_index >= bitmap->bitmaps.count) return 0;
                            expected = TAG_BLOCK_GET_ELEMENT(&bitmap->bitmaps, sprite->bitmap_index, struct bitmap_data);
                        }
                    }
                }
                vita_log("menu original bitmap accessor begin tag=%08lx", (unsigned long)index);
                if (bitmap_group_get_bitmap_from_sequence(index, 0, 0) != expected) return 0;
                ++bitmaps;
            }
        } else if (group == UNICODE_STRING_LIST_TAG) {
            struct string_list *list = unicode_string_list_definition_get(index);
            if (list->strings.count) {
                struct string_list_entry *entry = TAG_BLOCK_GET_ELEMENT(&list->strings, 0, struct string_list_entry);
                if (entry->string.size) {
                    vita_log("menu original Unicode accessor begin tag=%08lx", (unsigned long)index);
                    if (unicode_string_list_get_string(index, 0) != entry->string.address) return 0;
                }
                ++strings;
            }
        } else if (group == FONT_GROUP_TAG) {
            struct font_header *font = font_definition_get(index);
            if (font->character_tables.count) {
                struct font_character_table *table = TAG_BLOCK_GET_ELEMENT(&font->character_tables, 0, struct font_character_table);
                if (table->character_indices.count == 256) {
                    short *indices = table->character_indices.address;
                    long c;
                    for (c = 0; c < 256; ++c) if (indices[c] != NONE) {
                        void *expected = tag_block_get_element_with_size(&font->characters, indices[c], FONT_CHARACTER_SIZE);
                        vita_log("menu original font accessor begin tag=%08lx code=%lu", (unsigned long)index, (unsigned long)c);
                        if (font_get_character_by_ascii_code(font, (word)c) != expected) return 0;
                        ++fonts; break;
                    }
                }
            }
        }
    }
    if (!bitmaps || !strings || !fonts) {
        vita_log("menu accessor checkpoint FAILED: no nonempty bitmap/string/font coverage"); return 0;
    }
    vita_log("[VITA 019] original menu bitmap/string/font accessors PASS: bitmap_groups=%ld string_lists=%ld fonts=%ld; pixels/GPU/widget events not activated",
        bitmaps, strings, fonts);
    return 1;
}
