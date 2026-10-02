#!/usr/bin/env python3
"""Static regression gate for Vita UI renderer contracts.

This test protects behavior already verified in HaloCEVita.  It does not render
or emulate a frame; it checks that high-risk ordering and coordinate contracts
remain present in the shared D3D8/OpenGL backend.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RENDERER = ROOT / "port/linux/src/d3d8_gl.c"
XGPU = ROOT / "port/linux/src/xgpu.h"
XDK_PDB = ROOT / "port/include/xdk/xdk_pdb.h"
VITA_GRAPHICS = ROOT / "port/vita/include/halo_vita_graphics.h"
VITA_GL = ROOT / "port/vita/include/halo_vita_gl.h"
VITA_GL_COMPAT = ROOT / "port/vita/src/vita_gl_compat.c"


def require(condition, message):
    if not condition:
        raise SystemExit(f"FAIL: {message}")


renderer = RENDERER.read_text(encoding="utf-8")
xgpu = XGPU.read_text(encoding="utf-8")
xdk_pdb = XDK_PDB.read_text(encoding="utf-8")
vita_graphics = VITA_GRAPHICS.read_text(encoding="utf-8")
vita_gl = VITA_GL.read_text(encoding="utf-8")
vita_gl_compat = VITA_GL_COMPAT.read_text(encoding="utf-8")

# The menu-correctness path now keeps both Halo author space and the game render
# target at the original Xbox 640x480. Presentation to the 960x544 Vita panel
# remains a separate final blit, so no widget/tag coordinates are rewritten.
for declaration in (
    "#define HALO_VITA_GAME_WIDTH 640",
    "#define HALO_VITA_GAME_HEIGHT 480",
    "#define HALO_VITA_RENDER_WIDTH 640",
    "#define HALO_VITA_RENDER_HEIGHT 480",
):
    require(declaration in vita_graphics,
            f"Vita 640x480 game/render contract changed: {declaration}")
require("*width = HALO_VITA_GAME_WIDTH;" in renderer,
        "Vita screen mode must expose Halo's logical 640-wide author space")
require("(float)HALO_VITA_RENDER_WIDTH / (float)HALO_VITA_GAME_WIDTH" in renderer,
        "Vita X scaling must remain isolated at the render-target boundary")
require("(float)HALO_VITA_RENDER_HEIGHT / (float)SCREEN_HEIGHT" in renderer,
        "Vita Y scaling must remain isolated at the render-target boundary")

# UI widgets from the retail cache legitimately submit triangle fans. Losing
# this mapping silently turns four-vertex widget quads into the wrong topology.
fan_case = renderer.find("case D3DPT_TRIANGLEFAN:")
fan_gl = renderer.find("GL_TRIANGLE_FAN", fan_case)
require(fan_case >= 0 and fan_gl > fan_case,
        "D3DPT_TRIANGLEFAN must map to GL_TRIANGLE_FAN")

# Halo's viewport constants use top-left D3D coordinates. The GL shader bridge
# performs the Y inversion in viewport_scale; author-space widget coordinates
# must not be rewritten to compensate a second time.
require("device.viewport_scale[1] = -(float)device.viewport.Height * 0.5f;" in renderer,
        "viewport Y scale must remain negative for D3D top-left semantics")
require("device.viewport_offset[1] = device.viewport.Y + device.viewport.Height * 0.5f;" in renderer,
        "viewport Y offset must remain the positive D3D half-height offset")

# The Xbox/NV2A blend-factor values recovered in xdk_pdb.h deliberately match
# the OpenGL enumerants consumed directly by glBlendFunc. Do not insert a
# second translation table unless the underlying XDK contract changes.
for declaration in (
    "D3DBLEND_ZERO = 0,",
    "D3DBLEND_ONE = 1,",
    "D3DBLEND_SRCCOLOR = 768,",
    "D3DBLEND_INVSRCCOLOR = 769,",
    "D3DBLEND_SRCALPHA = 770,",
    "D3DBLEND_INVSRCALPHA = 771,",
    "D3DBLEND_DESTALPHA = 772,",
    "D3DBLEND_INVDESTALPHA = 773,",
    "D3DBLEND_DESTCOLOR = 774,",
    "D3DBLEND_INVDESTCOLOR = 775,",
    "D3DBLEND_SRCALPHASAT = 776,",
    "D3DBLEND_CONSTANTCOLOR = 32769,",
    "D3DBLEND_INVCONSTANTCOLOR = 32770,",
    "D3DBLEND_CONSTANTALPHA = 32771,",
    "D3DBLEND_INVCONSTANTALPHA = 32772,",
):
    require(declaration in xdk_pdb,
            f"Xbox blend enum changed or disappeared: {declaration}")
require("glBlendFunc(gl_state.blend_source, gl_state.blend_destination);" in renderer,
        "renderer must keep the verified Xbox/NV2A-to-GL blend-factor contract")

# A Vita cache miss can upload/composite a texture while bind_textures is
# walking the four Xbox texture stages. The upload uses whichever GL texture
# unit is currently active, so each stage must select its unit before calling
# xgpu_texture_get/mip composition. Otherwise a cold upload can overwrite an
# earlier stage even though a warm draw appears correct.
stage_comment = renderer.find("Uploads and mip composition bind on the active GL unit")
stage_select = renderer.find("glActiveTexture(gl_state.active_texture);", stage_comment)
texture_get = renderer.find("xgpu_texture_get(", stage_comment)
require(stage_comment >= 0 and stage_select > stage_comment and texture_get > stage_select,
        "Vita must select each GL texture unit before a texture cache miss/upload")

# DXT3/DXT5 are already decoded by xbox_textures.c on Vita. DXT1 was the last
# native compressed path. Xbox cache blocks must not cross the vitaGL boundary
# under an assumed GPU compressed layout: the Vita shim decodes DXT1 to BGRA,
# retaining c0<=c1 punch-through alpha, then performs an ordinary TexImage2D.
require("#define glCompressedTexImage2D halo_vita_glCompressedTexImage2D" in vita_gl,
        "Vita compressed uploads must pass through the checked DXT boundary")
require("internal_format != GL_COMPRESSED_RGBA_S3TC_DXT1_EXT" in vita_gl_compat,
        "Vita compressed boundary must explicitly admit only the expected DXT1 path")
require("palette[3] = 0;" in vita_gl_compat,
        "DXT1 punch-through alpha must remain transparent")
require("glTexImage2D(target, level, GL_RGBA8" in vita_gl_compat and
        "GL_BGRA, GL_UNSIGNED_BYTE, pixels" in vita_gl_compat,
        "DXT1 must be decoded to ordinary BGRA before vitaGL sampling")

# Vita resource uploads can mutate GL bindings/state behind the device's state
# cache. On Vita, the full raster contract must be applied after textures have
# been resolved/uploaded, immediately before the draw state is consumed.
bind = renderer.find("bind_textures(&key, uniforms.texture_scale);")
require(bind >= 0, "prepare_draw must bind/resolve textures")
vita_guard = renderer.find("#ifdef HALO_VITA", bind)
raster = renderer.find("apply_raster_state(has_depth);", bind)
require(vita_guard >= 0 and bind < vita_guard < raster,
        "Vita raster state must be re-applied after texture preparation")
require("Resource uploads/mip composition invalidate or change GL state" in renderer,
        "post-upload raster-state rationale must remain documented in code")

# Any lower layer that changes GL state behind d3d8_gl must invalidate the
# shared shadow cache. Keeping the explicit boundary prevents warm/cold draw
# behavior from diverging again.
require("void xgpu_gl_state_invalidate(void);" in xgpu,
        "xgpu GL-state invalidation boundary must remain declared")
require("code that\nchanges GL state behind it" in xgpu,
        "xgpu invalidation contract must remain documented")

print("PASS: Vita UI renderer contracts: 640x480 game target, fan topology, "
      "viewport transform, Xbox/GL blend enums, texture-stage selection, "
      "DXT1-to-BGRA boundary, post-resource raster restore and GL shadow "
      "invalidation")
