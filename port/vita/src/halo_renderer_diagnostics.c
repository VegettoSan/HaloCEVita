#ifndef HALO_VITA_ORIGINAL_RUNTIME
/* Diagnostic-only contracts reached by the original Xbox texture cache.
 * Keep failure information useful on Vita without pulling the PC terminal,
 * console, BSP, player and object-debug subsystems into Main Menu bring-up. */
#include "cseries.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_groups.h"
#include "cache/cache_files.h"
#include "vita_runtime.h"
#include <stdarg.h>
#include <stdio.h>

char *tag_get_name(long tag_index);

void terminal_printf(real_argb_color const *color, char const *format, ...)
{
    char buffer[1024];
    va_list arguments;
    (void)color;
    va_start(arguments, format);
    vsnprintf(buffer, sizeof(buffer), format, arguments);
    va_end(arguments);
    buffer[sizeof(buffer) - 1] = '\0';
    vita_log("Halo terminal: %s", buffer);
}

void console_warning(const char *format, ...)
{
    char buffer[1024];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(buffer, sizeof(buffer), format, arguments);
    va_end(arguments);
    buffer[sizeof(buffer) - 1] = '\0';
    vita_log("Halo warning: %s", buffer);
}

void scenario_debug_to_file(FILE *stream)
{
    struct tag_iterator iterator;
    long scenario_index;

    if (!stream) return;
    tag_iterator_new(&iterator, SCENARIO_TAG);
    scenario_index = tag_iterator_next(&iterator);
    if (scenario_index != NONE) {
        fprintf(stream,
            "\"%s\" <Vita Main Menu bring-up: scenario mounted; BSP/player/object debug not initialized>\n",
            tag_get_name(scenario_index));
    } else {
        fprintf(stream, "<no scenario loaded>\n");
    }
}

#endif
