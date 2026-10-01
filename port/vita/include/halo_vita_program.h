/* Included by original D3D8 after GL/SDK bridge declarations.
 * The installed non-strict vitaGL handle names a whole uniform, not an
 * array element. Reflect compiled spans and upload original bounded prefixes. */
#ifndef HALO_VITA_PROGRAM_H
#define HALO_VITA_PROGRAM_H
#include <stdio.h>
#include <string.h>
struct halo_vita_uniform_array {
    GLint location;
    GLsizei count;
};
static void halo_vita_find_uniform_array(GLuint program, const char *name,
    struct halo_vita_uniform_array *array, unsigned capacity)
{
    GLint active = 0;
    GLuint index;
    size_t name_length = strlen(name);
    array->location = -1;
    array->count = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &active);
    if (active < 0) vita_fatal("Vita shader returned invalid active uniform count");
    for (index = 0; index < (GLuint)active; ++index) {
        char reflected[128];
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;
        glGetActiveUniform(program, index, sizeof(reflected), &length, &size, &type, reflected);
        if (length < 0 || (unsigned)length >= sizeof(reflected) - 1)
            vita_fatal("Vita reflected uniform name exceeds bounded buffer");
        reflected[length] = 0;
        if (strcmp(reflected, name) &&
            !(strncmp(reflected, name, name_length) == 0 && !strcmp(reflected + name_length, "[0]")))
            continue;
        if (type != GL_FLOAT_VEC4 || size <= 0 || (unsigned)size > capacity)
            vita_fatal("Vita compiled vec4 array exceeds original register contract");
        array->location = glGetUniformLocation(program, name);
        if (array->location < 0 && strcmp(reflected, name))
            array->location = glGetUniformLocation(program, reflected);
        if (array->location < 0)
            vita_fatal("Vita active uniform array has no base location");
        array->count = size;
        platform_log("[VITA UNIFORM] program=%u array=%s base=%d compiled_vectors=%d capacity=%u",
            program, name, array->location, size, capacity);
        return;
    }
}
static void halo_vita_upload_uniform_array(const struct halo_vita_uniform_array *array,
    float shadow[][4], const float values[][4])
{
    size_t bytes = (size_t)array->count * sizeof(values[0]);
    if (!array->count || !memcmp(shadow, values, bytes)) return;
    glUniform4fv(array->location, array->count, values[0]);
    memcpy(shadow, values, bytes);
}
static void halo_vita_bind_vertex_inputs(GLuint program)
{
    unsigned long index;
    for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; ++index) {
        char name[16];
        snprintf(name, sizeof(name), "v%lu_in", index);
        glBindAttribLocation(program, (GLuint)index, name);
    }
    platform_log("Vita D3D8 program=%u bound NV2A input registers0..15 before link", program);
}
static void halo_vita_find_vertex_constants(GLuint program, struct halo_vita_uniform_array *array)
{
    halo_vita_find_uniform_array(program, "c", array, XGPU_VERTEX_CONSTANT_COUNT);
}
static void halo_vita_upload_vertex_constants(const struct halo_vita_uniform_array *array,
    const float constants[XGPU_VERTEX_CONSTANT_COUNT][4],
    const unsigned long *register_serials, unsigned long previous_serial)
{
    GLsizei index;
    for (index = 0; index < array->count; ++index)
        if (!previous_serial || register_serials[index] > previous_serial) {
            glUniform4fv(array->location, array->count, constants[0]);
            return;
        }
}
#endif
