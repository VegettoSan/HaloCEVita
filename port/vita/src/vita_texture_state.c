/* Preserve the original D3D8 draw state across Vita texture cache work.
 *
 * xbox_textures.c intentionally invalidates d3d8_gl.c's complete GL state
 * shadow whenever it binds/uploads a texture behind that cache.  The normal
 * backend contract is then to establish draw state again before rendering.
 * During a cold Vita UI draw, however, prepare_draw() applies raster state
 * before bind_textures(), so an upload can happen after the alpha blend was
 * established and before glDraw*.  Keep the original D3D render-state words
 * as the owner: this bridge restores only the blend state needed by that same
 * draw after xgpu_texture_get() returns.  It does not alter texture pixels,
 * widget alpha, shaders or menu-specific values.
 */
#include "xgpu.h"
#include "vita_runtime.h"

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
