/* Game ABI side of the menu-data boundary. Use the same accessors that
 * ui_widget.c uses; never emulate widget initialization or rendering. */
#include "cseries.h"
#include "vita_runtime.h"
#include "halo_vita_cache.h"
#include "interface/ui_widget_tags.h"
#include "cache/cache_files.h"
#include "cache/texture_cache.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/bitmaps.h"
#include "text/font_group.h"
#include "text/text_group.h"
#include "sound/sound_definitions.h"
#include "interface/ui_widget.h"
#include "interface/player_ui.h"
#include "saved games/saved_game_files.h"
#include "interface/event_manager.h"
#include "interface/virtual_keyboard.h"
static boolean vita_ui_shell_services_initialized;
#include "interface/interface.h"
#include "game/game_globals.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"

#define CHECK_LAYOUT(name, expression) typedef char name[(expression) ? 1 : -1]
CHECK_LAYOUT(vita_sound_size, sizeof(struct sound_definition) == 164);
CHECK_LAYOUT(vita_sound_ranges, offsetof(struct sound_definition, pitch_ranges) == 152);
CHECK_LAYOUT(vita_sound_promotion, offsetof(struct sound_definition, promotion_sound) == 112);
CHECK_LAYOUT(vita_sound_range_size, sizeof(struct sound_pitch_range) == 72);
CHECK_LAYOUT(vita_sound_permutations, offsetof(struct sound_pitch_range, permutations) == 60);
CHECK_LAYOUT(vita_sound_permutation_size, sizeof(struct sound_permutation) == 124);
CHECK_LAYOUT(vita_sound_samples, offsetof(struct sound_permutation, samples) == 64);
CHECK_LAYOUT(vita_loop_size, sizeof(struct looping_sound_definition) == 84);
CHECK_LAYOUT(vita_loop_tracks, offsetof(struct looping_sound_definition, tracks) == 60);
CHECK_LAYOUT(vita_loop_details, offsetof(struct looping_sound_definition, details) == 72);
CHECK_LAYOUT(vita_loop_track_start, offsetof(struct looping_sound_track, start_sound) == 48);
CHECK_LAYOUT(vita_loop_track_alternate, offsetof(struct looping_sound_track, alternate_loop_sound) == 128);
CHECK_LAYOUT(vita_loop_track_size, sizeof(struct looping_sound_track) == 160);
CHECK_LAYOUT(vita_loop_detail_size, sizeof(struct looping_sound_detail) == 104);

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
CHECK_LAYOUT(menu_game_globals_size, sizeof(struct game_globals) == 0x1AC);
CHECK_LAYOUT(menu_game_globals_interface_refs, offsetof(struct game_globals, interface_tag_references) == 0x140);

struct vita_interface_tag_references_definition
{
    struct tag_reference tags[NUMBER_OF_INTERFACE_TAGS];
    byte unused[48];
};
CHECK_LAYOUT(menu_interface_refs_size, sizeof(struct vita_interface_tag_references_definition) == 0x130);

/* The complete scenario module is intentionally not pulled into the UI-only
 * bring-up yet. These pointers reference the real scnr and globals\\globals
 * tags mounted from ui.map. They preserve the original accessors required by
 * ui_widget.c without fabricating a parallel menu/scenario state. */
static struct scenario *vita_ui_scenario;
static struct game_globals *vita_ui_game_globals;
static boolean vita_menu_bitmap_resources_activated;
char *tag_get_name(long tag_index);

enum
{
    /* Private flag value shared by bitmaps.c/xbox_texture_cache.c. */
    _vita_bitmap_cached_bit = 7,
};

#ifndef HALO_VITA_ORIGINAL_RUNTIME
struct scenario *global_scenario_get(void)
{
    match_assert("c:\\halo\\SOURCE\\scenario\\scenario.c", 183, vita_ui_scenario);
    return vita_ui_scenario;
}

struct game_globals *scenario_get_game_globals(void)
{
    return vita_ui_game_globals;
}

#else
extern struct scenario *global_scenario;
extern struct game_globals *global_game_globals;
extern long global_scenario_index;
#endif

static int halo_vita_ui_game_globals_initialize(void)
{
    struct tag_iterator scenario_iterator;
    long scenario_index;
    long index = tag_loaded(GAME_GLOBALS_TAG, "globals\\globals");
    struct game_globals *globals;
    struct vita_interface_tag_references_definition *references;
    struct tag_reference *plasma;

    tag_iterator_new(&scenario_iterator, SCENARIO_TAG);
    scenario_index = tag_iterator_next(&scenario_iterator);
    if (scenario_index == NONE) {
        vita_log("MAIN MENU BLOCKED: ui.map contains no scenario tag");
        return 0;
    }
    vita_ui_scenario = tag_get(SCENARIO_TAG, scenario_index);
    if (!vita_ui_scenario) {
        vita_log("MAIN MENU BLOCKED: ui.map scenario datum=%08lx did not resolve",
            (unsigned long)scenario_index);
        return 0;
    }
    if (index == NONE) {
        vita_log("MAIN MENU BLOCKED: ui.map globals\\globals matg not loaded");
        vita_ui_scenario = NULL;
        return 0;
    }
    globals = game_globals_definition_get(index);
    if (!globals || globals->interface_tag_references.count != 1 ||
        !globals->interface_tag_references.address) {
        vita_log("MAIN MENU BLOCKED: real matg interface_tag_references invalid count=%ld address=%p",
            globals ? globals->interface_tag_references.count : -1L,
            globals ? globals->interface_tag_references.address : NULL);
        vita_ui_scenario = NULL;
        return 0;
    }
    references = TAG_BLOCK_GET_ELEMENT(&globals->interface_tag_references, 0,
        struct vita_interface_tag_references_definition);
    plasma = &references->tags[_interface_bitmap_iface_map3];
    if (plasma->index == NONE || tag_get_group_tag(plasma->index) != BITMAP_GROUP_TAG) {
        vita_log("MAIN MENU BLOCKED: original iface_map3 plasma reference missing/invalid datum=%08lx",
            (unsigned long)plasma->index);
        vita_ui_scenario = NULL;
        return 0;
    }
    vita_ui_game_globals = globals;
#ifdef HALO_VITA_ORIGINAL_RUNTIME
    global_scenario = vita_ui_scenario;
    global_game_globals = globals;
    global_scenario_index = scenario_index;
#endif
    vita_log("[VITA 031] real ui scenario/globals mounted: scnr=%08lx matg=%08lx interface_refs=%ld iface_map3=%08lx path=%s",
        (unsigned long)scenario_index, (unsigned long)index,
        globals->interface_tag_references.count,
        (unsigned long)plasma->index, tag_get_name(plasma->index));
    return 1;
}

int halo_vita_menu_tags_probe(uint32_t menu_index, const struct vita_menu_stats *stats)
{
    struct ui_widget_definition *menu;
    struct tag_iterator iterator;
    long index, bitmaps = 0, strings = 0, fonts = 0;
    int bitmap_logged = 0, string_logged = 0, font_logged = 0;
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
                if (!bitmap_logged++) vita_log("menu original bitmap accessor begin tag=%08lx", (unsigned long)index);
                if (bitmap_group_get_bitmap_from_sequence(index, 0, 0) != expected) return 0;
                ++bitmaps;
            }
        } else if (group == UNICODE_STRING_LIST_TAG) {
            struct string_list *list = unicode_string_list_definition_get(index);
            if (list->strings.count) {
                struct string_list_entry *entry = TAG_BLOCK_GET_ELEMENT(&list->strings, 0, struct string_list_entry);
                if (entry->string.size) {
                    if (!string_logged++) vita_log("menu original Unicode accessor begin tag=%08lx", (unsigned long)index);
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
                        if (!font_logged++) vita_log("menu original font accessor begin tag=%08lx code=%lu", (unsigned long)index, (unsigned long)c);
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

int halo_vita_ui_runtime_initialize(void)
{
    if (!halo_vita_ui_game_globals_initialize()) return 0;
    if (!halo_vita_ui_widgets_initialized()) ui_widgets_initialize();
    if (!halo_vita_ui_widgets_initialized()) {
        vita_log("MAIN MENU BLOCKED: original ui_widgets_initialize did not establish widget globals/pool");
        vita_ui_scenario = NULL;
        vita_ui_game_globals = NULL;
        return 0;
    }
    vita_log("[VITA 022] original ui_widgets_initialize PASS: widget globals and pool ready");
    return 1;
}

int halo_vita_ui_runtime_dispose(void)
{
    if (halo_vita_ui_widgets_initialized()) {
        if (!halo_vita_ui_widgets_dispose_checkpoint()) {
            vita_log("MAIN MENU BLOCKED: checkpoint disposal found active widgets");
            return 0;
        }
        vita_log("[VITA 023] original widget pool freed; globals reset");
    }
    if (vita_ui_shell_services_initialized) {
        virtual_keyboard_dispose();
        event_manager_dispose();
        saved_game_files_dispose();
        vita_ui_shell_services_initialized = FALSE;
    }
    vita_ui_scenario = NULL;
    vita_ui_game_globals = NULL;
    vita_menu_bitmap_resources_activated = FALSE;
    return 1;
}

int halo_vita_menu_bitmap_resources_activate(void)
{
    struct tag_iterator iterator;
    long bitmap_group_index;
    long group_count = 0;
    long bitmap_count = 0;
    long serialized_pointer_count = 0;

    if (vita_menu_bitmap_resources_activated)
        return 1;
    if (!vita_ui_scenario || !vita_ui_game_globals) {
        vita_log("MAIN MENU BLOCKED: bitmap resource activation requested without mounted ui scenario/globals");
        return 0;
    }

    tag_iterator_new(&iterator, BITMAP_GROUP_TAG);
    while ((bitmap_group_index = tag_iterator_next(&iterator)) != NONE) {
        struct bitmap_group *group = bitmap_group_get(bitmap_group_index);
        long bitmap_index;

        if (!group || group->bitmaps.count < 0 ||
            (group->bitmaps.count && !group->bitmaps.address)) {
            vita_log("MAIN MENU BLOCKED: invalid mounted bitmap group datum=%08lx count=%ld address=%p",
                (unsigned long)bitmap_group_index,
                group ? group->bitmaps.count : -1L,
                group ? group->bitmaps.address : NULL);
            return 0;
        }

        for (bitmap_index = 0; bitmap_index < group->bitmaps.count; ++bitmap_index) {
            struct bitmap_data *bitmap = TAG_BLOCK_GET_ELEMENT(
                &group->bitmaps, bitmap_index, struct bitmap_data);

            /* scenario_tags_load reads compiled records directly. Their cached
             * bit, absolute logical pixels_offset, size and tag_index are already
             * prepared by cache construction. Calling bitmap_new again would
             * assert and add pixel_data.file_offset a second time. Do not alter
             * flags/layout or fabricate a new registration for retail caches. */
            if (!bitmap_verify(bitmap, FALSE)) {
                vita_log("MAIN MENU BLOCKED: original bitmap_verify failed tag=%08lx path=%s bitmap=%ld format=%d dims=%dx%dx%d flags=%04x",
                    (unsigned long)bitmap_group_index,
                    tag_get_name(bitmap_group_index), bitmap_index,
                    (int)bitmap->format, (int)bitmap->width, (int)bitmap->height,
                    (int)bitmap->depth, (unsigned)bitmap->flags);
                return 0;
            }
            /* Cached records may retain serialized Xbox pointer values. The
             * original cache lookup uses cache_block_index, overwrites base
             * with its own LRU allocation before Register/read, and ignores
             * hardware_format on this branch. Neither pointer is a resident
             * Vita address; preserve it without validating/dereferencing it. */
            if (!TEST_FLAG(bitmap->flags, _vita_bitmap_cached_bit) ||
                bitmap->tag_index != bitmap_group_index ||
                bitmap->cache_block_index != NONE || bitmap->pixels_offset < 0 ||
                bitmap->pixels_size <= 0 ||
                !vita_cache_resource_range_valid((uint32_t)bitmap->pixels_offset,
                    (size_t)bitmap->pixels_size)) {
                vita_log("MAIN MENU BLOCKED: invalid compiled bitmap tag=%08lx path=%s bitmap=%ld flags=%04x owner=%08lx block=%08lx base=%p hardware=%p offset=%ld size=%ld",
                    (unsigned long)bitmap_group_index, tag_get_name(bitmap_group_index),
                    bitmap_index, (unsigned)bitmap->flags, (unsigned long)bitmap->tag_index,
                    (unsigned long)bitmap->cache_block_index, bitmap->base_address,
                    bitmap->hardware_format, bitmap->pixels_offset, bitmap->pixels_size);
                return 0;
            }
            if (bitmap->base_address || bitmap->hardware_format)
                ++serialized_pointer_count;
            ++bitmap_count;
        }
        ++group_count;
    }

    if (!bitmap_count) {
        vita_log("MAIN MENU BLOCKED: mounted ui.map exposed no bitmap resources to original texture cache");
        return 0;
    }
    vita_menu_bitmap_resources_activated = TRUE;
    vita_log("[VITA 039] compiled bitmap cache state validated: groups=%ld bitmaps=%ld serialized_pointer_records=%ld; absolute pixel offsets preserved; original on-demand texture load ready",
        group_count, bitmap_count, serialized_pointer_count);
    return 1;
}

int halo_vita_menu_root_checkpoint(void)
{
    if (!vita_ui_game_globals || !vita_ui_scenario) {
        vita_log("MAIN MENU BLOCKED: ui scenario/game globals not mounted before root creation");
        return 0;
    }
#ifdef HALO_VITA_ORIGINAL_RUNTIME
    main_menu_active(TRUE);
    main_screen_shell_load();
    return halo_vita_ui_original_root_ready();
#else
    halo_vita_ui_render_clock_update();
    if (!vita_ui_shell_services_initialized) {
        saved_game_files_initialize();
        event_manager_initialize();
        vita_ui_shell_services_initialized = TRUE;
        if (!virtual_keyboard_initialize()) {
            vita_log("MAIN MENU BLOCKED: original virtual keyboard missing");
            return 0;
        }
    }
    vita_log("[VITA UI SAVES] original saved-game and profile owners initialized");
    vita_log("original player_ui_initialize begin");
    player_ui_initialize();
    vita_log("original player_ui_initialize PASS");
    return halo_vita_menu_root_load() ? 1 : 0;
#endif
}

int halo_vita_menu_update_checkpoint(void)
{
    vita_log("[VITA 029] original UI update begin");
    process_ui_widgets();
    vita_log("[VITA 029] original UI update PASS");
    return 1;
}

int halo_vita_menu_render_checkpoint(void)
{
    /* Original render.c passes the rasterizer camera's viewport bounds.
     * The full device/camera path is not initialized in this UI checkpoint;
     * this only probes the original widget render closure. */
    rectangle2d bounds = {0, 0, 480, 640};
    vita_log("[VITA 030] original UI render begin");
    render_ui_widgets(0, &bounds);
    vita_log("[VITA 030] original UI render returned");
    return 1;
}
