/* Shared original on-disk widget layouts; no runtime behavior changed. */
#ifndef HALO_UI_WIDGET_TAGS_H
#define HALO_UI_WIDGET_TAGS_H
#include "math/integer_math.h"
#include "math/real_math.h"
#include "tag_files/tag_groups.h"
/* narrow views of the 'DeLa' widget definition tag and of the three block
elements this file walks; only the members this file reaches are named and
every other span is left explicitly unknown */

struct ui_widget_event_handler_reference
{
	long flags;
	short event_type;
	short function;
	struct tag_reference widget_tag;
	struct tag_reference sound_effect;
	char script[32];
};

struct ui_widget_child_reference
{
	struct tag_reference widget_tag;
	char name[32];
	long flags;
	short custom_controller_index;
	short vertical_offset;
	short horizontal_offset;
	byte unknown03A[0x50 - 0x3A];
};

struct ui_widget_conditional_reference
{
	struct tag_reference widget_tag;
	char name[32];
	long flags;
	short custom_controller_index;
	byte unknown036[0x50 - 0x36];
};

struct ui_widget_game_data_input_reference
{
	short function;
	byte unknown002[0x24 - 0x02];
};

struct ui_widget_search_and_replace_reference
{
	char search_string[32];
	short replace_function;
};

struct ui_widget_definition
{
	short type;
	short controller_index;
	char name[32];
	rectangle2d bounds;
	long flags;
	long milliseconds_to_auto_close;
	long auto_close_fade_time;
	struct tag_reference background_bitmap;
	struct tag_block game_data_inputs;
	struct tag_block event_handlers;
	struct tag_block search_and_replace_functions;
	byte unknown06C[0xEC - 0x6C];
	struct tag_reference text_label_string_list;
	struct tag_reference text_font;
	real_argb_color text_color;
	short justification;
	word text_box_flags;
	byte unknown120[0x12E - 0x120];
	short string_list_index;
	short horizontal_offset;
	short vertical_offset;
	byte unknown134[0x150 - 0x134];
	long list_flags;
	struct tag_reference list_header_bitmap;
	struct tag_reference list_footer_bitmap;
	rectangle2d list_header_bounds;
	rectangle2d list_footer_bounds;
	byte unknown184[0x1A4 - 0x184];
	struct tag_reference extended_description_widget;
	byte unknown1B4[0x2D4 - 0x1B4];
	struct tag_block conditional_widgets;
	byte unknown2E0[0x3E0 - 0x2E0];
	struct tag_block child_widgets;
};

typedef char verify_ui_widget_game_data_input_reference_size[
	sizeof(struct ui_widget_game_data_input_reference) == 0x24 ? 1 : -1];
typedef char verify_ui_widget_search_and_replace_reference_size[
	sizeof(struct ui_widget_search_and_replace_reference) == 0x22 ? 1 : -1];
typedef char verify_ui_widget_child_reference_size[
	sizeof(struct ui_widget_child_reference) == 0x50 ? 1 : -1];
typedef char verify_ui_widget_conditional_reference_size[
	sizeof(struct ui_widget_conditional_reference) == 0x50 ? 1 : -1];
typedef char verify_ui_widget_event_handler_reference_size[
	sizeof(struct ui_widget_event_handler_reference) == 0x48 ? 1 : -1];
typedef char verify_ui_widget_definition_bounds_offset[
	offsetof(struct ui_widget_definition, bounds) == 0x24 ? 1 : -1];
typedef char verify_ui_widget_definition_flags_offset[
	offsetof(struct ui_widget_definition, flags) == 0x2C ? 1 : -1];
typedef char verify_ui_widget_definition_game_data_inputs_offset[
	offsetof(struct ui_widget_definition, game_data_inputs) == 0x48 ? 1 : -1];
typedef char verify_ui_widget_definition_search_and_replace_offset[
	offsetof(struct ui_widget_definition, search_and_replace_functions) == 0x60 ? 1 : -1];
typedef char verify_ui_widget_definition_text_font_offset[
	offsetof(struct ui_widget_definition, text_font) == 0xFC ? 1 : -1];
typedef char verify_ui_widget_definition_text_color_offset[
	offsetof(struct ui_widget_definition, text_color) == 0x10C ? 1 : -1];
typedef char verify_ui_widget_definition_justification_offset[
	offsetof(struct ui_widget_definition, justification) == 0x11C ? 1 : -1];
typedef char verify_ui_widget_definition_text_box_flags_offset[
	offsetof(struct ui_widget_definition, text_box_flags) == 0x11E ? 1 : -1];
typedef char verify_ui_widget_definition_string_list_index_offset[
	offsetof(struct ui_widget_definition, string_list_index) == 0x12E ? 1 : -1];
typedef char verify_ui_widget_definition_horizontal_offset_offset[
	offsetof(struct ui_widget_definition, horizontal_offset) == 0x130 ? 1 : -1];
typedef char verify_ui_widget_definition_list_header_bitmap_offset[
	offsetof(struct ui_widget_definition, list_header_bitmap) == 0x154 ? 1 : -1];
typedef char verify_ui_widget_definition_list_header_bounds_offset[
	offsetof(struct ui_widget_definition, list_header_bounds) == 0x174 ? 1 : -1];
typedef char verify_ui_widget_definition_event_handlers_offset[
	offsetof(struct ui_widget_definition, event_handlers) == 0x54 ? 1 : -1];
typedef char verify_ui_widget_definition_background_bitmap_offset[
	offsetof(struct ui_widget_definition, background_bitmap) == 0x38 ? 1 : -1];
typedef char verify_ui_widget_definition_text_label_string_list_offset[
	offsetof(struct ui_widget_definition, text_label_string_list) == 0xEC ? 1 : -1];
typedef char verify_ui_widget_definition_list_flags_offset[
	offsetof(struct ui_widget_definition, list_flags) == 0x150 ? 1 : -1];
typedef char verify_ui_widget_definition_extended_description_offset[
	offsetof(struct ui_widget_definition, extended_description_widget) == 0x1A4 ? 1 : -1];
typedef char verify_ui_widget_definition_conditional_widgets_offset[
	offsetof(struct ui_widget_definition, conditional_widgets) == 0x2D4 ? 1 : -1];
typedef char verify_ui_widget_definition_child_widgets_offset[
	offsetof(struct ui_widget_definition, child_widgets) == 0x3E0 ? 1 : -1];
typedef char verify_ui_widget_definition_size[
	sizeof(struct ui_widget_definition) == 0x3EC ? 1 : -1];


#endif
