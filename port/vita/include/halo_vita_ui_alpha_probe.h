/* First-frame diagnostic of the actual original UI shader/vertex inputs.
 * Included only by the Vita D3D8 backend after the original draw setup.
 * No new shader compilation or bitmap mutation. The caller invalidates its
 * shadow after all native state is restored. Never runs during steady frames.
 */
#ifndef HALO_VITA_UI_ALPHA_PROBE_H
#define HALO_VITA_UI_ALPHA_PROBE_H
static void halo_vita_ui_alpha_probe(const float *vertices, unsigned long stride,
    GLuint mask, const struct xgpu_texture_description *description,
    const float constants[][4], const float ps[][4])
{
    static unsigned probes;
    GLuint temporary_texture = 0, temporary_fbo = 0, source_fbo = 0;
    GLint previous_read, previous_draw, active, binding, src, dst, equation;
    GLint color_mask[4], program, stage_textures[4];
    GLfloat clear_color[4];
    GLboolean blend, depth, stencil, scissor, cull;
    unsigned char storage[4] = {0}, corner[4] = {0}, center[4] = {0};
    unsigned i;
    int x, y, cx, cy;
    GLenum error, source_status, output_status;
    if (probes >= 2 || !mask || description->width < 32 || description->height < 32)
        return;
    ++probes;
    /* Native queries are authoritative; gl_state can contain invalidation
     * sentinels after a cold texture upload, which are not driver state. */
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
    glGetIntegerv(GL_BLEND_SRC, &src);
    glGetIntegerv(GL_BLEND_DST, &dst);
    glGetIntegerv(GL_BLEND_EQUATION, &equation);
    glGetIntegerv(GL_COLOR_WRITEMASK, color_mask);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clear_color);
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    blend = glIsEnabled(GL_BLEND);
    depth = glIsEnabled(GL_DEPTH_TEST);
    stencil = glIsEnabled(GL_STENCIL_TEST);
    scissor = glIsEnabled(GL_SCISSOR_TEST);
    cull = glIsEnabled(GL_CULL_FACE);
    for (i = 0; i < 4; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &stage_textures[i]);
    }
    glActiveTexture((GLenum)active);
    error = glGetError();
    if (error != GL_NO_ERROR)
        vita_fatal("Vita UI alpha diagnostic native state query failed");
    vita_log("[VITA UI GPU] probe=%u program=%d mask=%u dims=%lux%lu native_blend=%d/%x/%x/%x tex=%d,%d,%d,%d write=%d%d%d%d",
        probes, program, mask, description->width, description->height,
        (int)blend, src, dst, equation, stage_textures[0], stage_textures[1],
        stage_textures[2], stage_textures[3], color_mask[0], color_mask[1], color_mask[2], color_mask[3]);
    for (i = 32; i <= 38; ++i)
        vita_log("[VITA UI GPU] probe=%u c%u=%g,%g,%g,%g", probes, i,
            constants[i][0], constants[i][1], constants[i][2], constants[i][3]);
    vita_log("[VITA UI GPU] probe=%u ps0=%g,%g,%g,%g ps1=%g,%g,%g,%g ps6=%g,%g,%g,%g",
        probes, ps[0][0], ps[0][1], ps[0][2], ps[0][3],
        ps[1][0], ps[1][1], ps[1][2], ps[1][3], ps[6][0], ps[6][1], ps[6][2], ps[6][3]);

    /* Inspect decoded storage separately from shader sampling. Bottom-left
     * is a transparent authored corner for the audited logo/label masks. */
    glGenFramebuffers(1, &source_fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, source_fbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mask, 0);
    source_status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
    if (source_status == GL_FRAMEBUFFER_COMPLETE) {
        glFinish();
        glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, storage);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
    glDeleteFramebuffers(1, &source_fbo);

    /* Reuse the actual linked program, uniforms, sampler bindings and streamed
     * attributes. Only the destination and raster state change for this copy. */
    glGenTextures(1, &temporary_texture);
    glBindTexture(GL_TEXTURE_2D, temporary_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, HALO_VITA_RENDER_WIDTH,
        HALO_VITA_RENDER_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, (GLuint)binding);
    glGenFramebuffers(1, &temporary_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, temporary_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, temporary_texture, 0);
    output_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (output_status == GL_FRAMEBUFFER_COMPLETE) {
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_CULL_FACE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearColor(1.0f, 0.0f, 1.0f, 0.5f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glFinish();
        x = (int)(vertices[0] * HALO_VITA_RENDER_WIDTH / HALO_VITA_GAME_WIDTH) + 1;
        y = (int)(vertices[1] * HALO_VITA_RENDER_HEIGHT / HALO_VITA_GAME_HEIGHT) + 1;
        cx = (int)((vertices[0] + vertices[2 * stride]) * 0.5f * HALO_VITA_RENDER_WIDTH / HALO_VITA_GAME_WIDTH);
        cy = (int)((vertices[1] + vertices[2 * stride + 1]) * 0.5f * HALO_VITA_RENDER_HEIGHT / HALO_VITA_GAME_HEIGHT);
        /* Bound every read even for clipped/cropped original geometry. */
        x = x < 0 ? 0 : x >= HALO_VITA_RENDER_WIDTH ? HALO_VITA_RENDER_WIDTH - 1 : x;
        y = y < 0 ? 0 : y >= HALO_VITA_RENDER_HEIGHT ? HALO_VITA_RENDER_HEIGHT - 1 : y;
        cx = cx < 0 ? 0 : cx >= HALO_VITA_RENDER_WIDTH ? HALO_VITA_RENDER_WIDTH - 1 : cx;
        cy = cy < 0 ? 0 : cy >= HALO_VITA_RENDER_HEIGHT ? HALO_VITA_RENDER_HEIGHT - 1 : cy;
        glReadPixels(x, HALO_VITA_RENDER_HEIGHT - 1 - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, corner);
        glReadPixels(cx, HALO_VITA_RENDER_HEIGHT - 1 - cy, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, center);
    }
    error = glGetError();
    vita_log("[VITA UI GPU] probe=%u source_fbo=%x storage_corner=%u,%u,%u,%u output_fbo=%x shader_corner=%u,%u,%u,%u shader_center=%u,%u,%u,%u gl_error=%x",
        probes, source_status, storage[0], storage[1], storage[2], storage[3], output_status,
        corner[0], corner[1], corner[2], corner[3], center[0], center[1], center[2], center[3], error);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
    if (blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (depth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (stencil) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
    if (scissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (cull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    glColorMask(color_mask[0], color_mask[1], color_mask[2], color_mask[3]);
    glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    glDeleteFramebuffers(1, &temporary_fbo);
    glDeleteTextures(1, &temporary_texture);
    glActiveTexture((GLenum)active);
    glBindTexture(GL_TEXTURE_2D, (GLuint)binding);
    if (error != GL_NO_ERROR || glGetError() != GL_NO_ERROR)
        vita_fatal("Vita UI alpha diagnostic GPU/readback failed");
}
#endif
