/* Included only by the actual D3D8 backend, after its GL declarations.
 * Keep register numbers intact; GLSL1.20 has no layout(location=N), and
 * vitaGL uniform locations must never be inferred by adding an index. */
#ifndef HALO_VITA_PROGRAM_H
#define HALO_VITA_PROGRAM_H
#include <stdio.h>
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
static void halo_vita_find_vertex_constants(GLuint program, GLint *locations)
{
    unsigned long index;
    for (index = 0; index < XGPU_VERTEX_CONSTANT_COUNT; ++index) {
        char name[16];
        snprintf(name, sizeof(name), "c[%lu]", index);
        locations[index] = glGetUniformLocation(program, name);
    }
}
static void halo_vita_upload_vertex_constants(const GLint *locations,
    const float constants[XGPU_VERTEX_CONSTANT_COUNT][4],
    const unsigned long *register_serials, unsigned long previous_serial)
{
    unsigned long index;
    for (index = 0; index < XGPU_VERTEX_CONSTANT_COUNT; ++index)
        if (locations[index] >= 0 && (!previous_serial || register_serials[index] > previous_serial))
            glUniform4fv(locations[index], 1, constants[index]);
}
#endif
