/* Preserve the original D3D8 draw state across Vita texture cache work.
 *
 * xbox_textures.c intentionally invalidates d3d8_gl.c's complete GL state
 * shadow whenever it binds/uploads a texture behind that cache.  The normal
 * backend contract is then to establish draw state again before rendering.
 * During a cold Vita UI draw a texture lookup can happen between the state
 * setup and glDraw*.  Keep the original D3D render-state words as the owner:
 * this bridge restores the blend state needed by that same draw.
 *
 * Dynamic D3D8 textures need a second Vita-specific contract. Halo's 128x128
 * hardware character cache is a real mutable Xbox texture: adding a glyph
 * rewrites its CPU backing store and rasterizer_bitmap_changed() publishes the
 * new contents before the glyph is drawn. Recreating vitaGL texture storage
 * for every such change forces a full GPU finish and makes text-heavy menus
 * stall. When xbox_textures.c is compiled through the aliases installed by
 * CMake, the wrappers below preserve the first glTexImage2D allocation and
 * turn later same-shape updates into glTexSubImage2D. The pinned vitaGL build
 * implements sub-image updates with copy-on-write when the old storage is in
 * flight, so an earlier draw keeps seeing the pixels it submitted without a
 * global glFinish. No Halo pixel, alpha, widget or texture-cache semantics are
 * changed: the same decoded BGRA bytes are published to the same GL object.
 */
#include "xgpu.h"
#include "vita_runtime.h"

#include <stdlib.h>

#define HALO_VITA_TRACKED_MIPS 16

struct halo_vita_texture_storage
{
    struct halo_vita_texture_storage *next;
    GLuint texture;
    unsigned int initialized_mask;
    GLsizei width[HALO_VITA_TRACKED_MIPS];
    GLsizei height[HALO_VITA_TRACKED_MIPS];
    GLint internal_format[HALO_VITA_TRACKED_MIPS];
    GLenum format[HALO_VITA_TRACKED_MIPS];
    GLenum type[HALO_VITA_TRACKED_MIPS];
};

static struct halo_vita_texture_storage *halo_vita_texture_storage_list;
static GLuint halo_vita_bound_texture_2d;

static struct halo_vita_texture_storage *halo_vita_texture_storage_find(GLuint texture, int create)
{
    struct halo_vita_texture_storage *entry;

    for (entry = halo_vita_texture_storage_list; entry; entry = entry->next)
        if (entry->texture == texture)
            return entry;
    if (!create || !texture)
        return NULL;
    entry = calloc(1, sizeof(*entry));
    if (!entry)
        vita_fatal("Vita mutable texture tracking allocation failed");
    entry->texture = texture;
    entry->next = halo_vita_texture_storage_list;
    halo_vita_texture_storage_list = entry;
    return entry;
}

static void halo_vita_texture_storage_forget(GLuint texture)
{
    struct halo_vita_texture_storage **link = &halo_vita_texture_storage_list;

    while (*link)
    {
        struct halo_vita_texture_storage *entry = *link;
        if (entry->texture == texture)
        {
            *link = entry->next;
            free(entry);
            return;
        }
        link = &entry->next;
    }
}

/* These four entry points are compile-time aliases only for xbox_textures.c.
 * vita_texture_state.c itself calls the real vitaGL functions below. */
void halo_vita_texture_bind(GLenum target, GLuint texture)
{
    glBindTexture(target, texture);
    if (target == GL_TEXTURE_2D)
        halo_vita_bound_texture_2d = texture;
}

void halo_vita_texture_delete(GLsizei count, const GLuint *textures)
{
    GLsizei index;

    for (index = 0; index < count; ++index)
    {
        if (halo_vita_bound_texture_2d == textures[index])
            halo_vita_bound_texture_2d = 0;
        halo_vita_texture_storage_forget(textures[index]);
    }
    glDeleteTextures(count, textures);
}

void halo_vita_texture_image_2d(GLenum target, GLint level, GLint internal_format,
    GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type,
    const GLvoid *pixels)
{
    struct halo_vita_texture_storage *entry = NULL;
    unsigned int bit = 0;
    int same_storage = 0;
    static unsigned long refreshes_logged;

    if (target == GL_TEXTURE_2D && halo_vita_bound_texture_2d &&
        level >= 0 && level < HALO_VITA_TRACKED_MIPS)
    {
        entry = halo_vita_texture_storage_find(halo_vita_bound_texture_2d, 1);
        bit = 1U << (unsigned int)level;
        same_storage = pixels && border == 0 && (entry->initialized_mask & bit) &&
            entry->width[level] == width && entry->height[level] == height &&
            entry->internal_format[level] == internal_format &&
            entry->format[level] == format && entry->type[level] == type;
    }

    if (same_storage)
    {
        glTexSubImage2D(target, level, 0, 0, width, height, format, type, pixels);
        if (refreshes_logged < 12)
        {
            ++refreshes_logged;
            vita_log("[VITA TEXTURE REFRESH] id=%u level=%d dims=%dx%d storage=subimage",
                halo_vita_bound_texture_2d, level, width, height);
        }
        return;
    }

    glTexImage2D(target, level, internal_format, width, height, border, format, type, pixels);
    if (entry)
    {
        entry->initialized_mask |= bit;
        entry->width[level] = width;
        entry->height[level] = height;
        entry->internal_format[level] = internal_format;
        entry->format[level] = format;
        entry->type[level] = type;
    }
}

void halo_vita_texture_upload_finish(void)
{
    struct halo_vita_texture_storage *entry =
        halo_vita_texture_storage_find(halo_vita_bound_texture_2d, 0);
    static unsigned long finishes_skipped_logged;

    /* xbox_textures.c keys one GL object to one immutable description. Once
     * level zero exists, a later uncompressed upload is a content refresh,
     * not a resize. glTexSubImage2D below performs vitaGL's safe copy-on-write
     * if earlier draws still reference the old storage. Compressed uploads do
     * not pass through halo_vita_texture_image_2d and therefore never create
     * one of these records. */
    if (entry && (entry->initialized_mask & 1U))
    {
        if (finishes_skipped_logged < 12)
        {
            ++finishes_skipped_logged;
            vita_log("[VITA TEXTURE REFRESH] id=%u prior-draw glFinish skipped; vitaGL COW owns hazard",
                halo_vita_bound_texture_2d);
        }
        return;
    }
    glFinish();
}

static GLenum halo_vita_blend_equation(DWORD operation)
{
    switch (operation)
    {
    case D3DBLENDOP_SUBTRACT:
        return GL_FUNC_SUBTRACT;
    case D3DBLENDOP_REVSUBTRACT:
    case D3DBLENDOP_REVSUBTRACTSIGNED:
        return GL_FUNC_REVERSE_SUBTRACT;
    case D3DBLENDOP_MIN:
        return GL_MIN;
    case D3DBLENDOP_MAX:
        return GL_MAX;
    default:
        return GL_FUNC_ADD;
    }
}

GLuint halo_vita_xgpu_texture_get(
    const DWORD *resource,
    const D3DCOLOR *palette,
    GLenum *target,
    struct xgpu_texture_description *description)
{
    GLuint texture;
    DWORD *rs;
    GLenum error;
    static unsigned long restores_logged;

    texture = xgpu_texture_get(resource, palette, target, description);
    rs = D3D__RenderState;

    if (rs[D3DRS_ALPHABLENDENABLE])
    {
        glEnable(GL_BLEND);
        glBlendFunc((GLenum)rs[D3DRS_SRCBLEND],
            (GLenum)rs[D3DRS_DESTBLEND]);
        glBlendEquation(halo_vita_blend_equation(rs[D3DRS_BLENDOP]));
    }
    else
    {
        glDisable(GL_BLEND);
    }

    error = glGetError();
    if (restores_logged < 12 || error != GL_NO_ERROR)
    {
        restores_logged++;
        vita_log("[VITA BLEND RESTORE] texture=%u enabled=%lu src=%lx dst=%lx op=%lx gl_error=%x",
            texture,
            (unsigned long)(rs[D3DRS_ALPHABLENDENABLE] != 0),
            (unsigned long)rs[D3DRS_SRCBLEND],
            (unsigned long)rs[D3DRS_DESTBLEND],
            (unsigned long)rs[D3DRS_BLENDOP],
            error);
    }
    if (error != GL_NO_ERROR)
        vita_fatal("Original D3D8 blend state restore failed after Vita texture lookup");

    return texture;
}
