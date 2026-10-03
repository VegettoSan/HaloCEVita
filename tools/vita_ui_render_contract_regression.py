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
VITA_PRESENT = ROOT / "port/vita/src/vita_graphics.c"
VITA_GL = ROOT / "port/vita/include/halo_vita_gl.h"
VITA_GL_COMPAT = ROOT / "port/vita/src/vita_gl_compat.c"


def require(condition, message):
    if not condition:
        raise SystemExit(f"FAIL: {message}")


renderer = RENDERER.read_text(encoding="utf-8")
xgpu = XGPU.read_text(encoding="utf-8")
xdk_pdb = XDK_PDB.read_text(encoding="utf-8")
vita_graphics = VITA_GRAPHICS.read_text(encoding="utf-8")
vita_present = VITA_PRESENT.read_text(encoding="utf-8")
vita_gl = VITA_GL.read_text(encoding="utf-8")
vita_gl_compat = VITA_GL_COMPAT.read_text(encoding="utf-8")

# This comparison build keeps both Halo's original Xbox 640x480 author space
# and the Vita screen-sized render target at 640x480. The final presentation
# uses the original D3D8 aspect-preserving blit; widget/tag coordinates
# remain untouched by the resolution policy.
for declaration in (
    "#define HALO_VITA_GAME_WIDTH 640",
    "#define HALO_VITA_GAME_HEIGHT 480",
    "#define HALO_VITA_RENDER_WIDTH 640",
    "#define HALO_VITA_RENDER_HEIGHT 480",
):
    require(declaration in vita_graphics,
            f"Vita logical/internal render contract changed: {declaration}")
require("*width = HALO_VITA_GAME_WIDTH;" in renderer,
        "Vita screen mode must expose Halo's logical 640-wide author space")
require("(float)HALO_VITA_RENDER_WIDTH / (float)HALO_VITA_GAME_WIDTH" in renderer,
        "Vita X scaling must remain isolated at the render-target boundary")
require("(float)HALO_VITA_RENDER_HEIGHT / (float)SCREEN_HEIGHT" in renderer,
        "Vita Y scaling must remain isolated at the render-target boundary")

# The D3D8 backend has already letterboxed its finished 640x480 frame into
# the native framebuffer when the Vita platform hook is called. Re-blitting
# from the source target here distorts the completed image to 960x544.
present = vita_present.split("void platform_video_swap(void)", 1)[1].split(
    "int halo_vita_texture_transfer_finish(void)", 1)[0]
require("glBlitFramebuffer(" in renderer.split("void WINAPI D3DDevice_Present(", 1)[1],
        "original D3D8 presentation blit must remain active")
require("vglSwapBuffers(GL_FALSE);" in present and "glBlitFramebuffer(" not in present,
        "Vita swap must not stretch an already presented Halo frame")

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

# All three DXT formats use the existing upstream decoder and uploader.
# A second Vita DXT1 decoder would duplicate that semantic owner.
textures = (ROOT / "port/linux/src/xbox_textures.c").read_text()
vita_decode = textures.split("#elif defined(HALO_VITA)", 1)[1].split("#endif", 1)[0]
require("decode_compressed = description->compressed;" in vita_decode,
        "Vita must use upstream's original decoder for all DXT formats")
require("halo_vita_glCompressedTexImage2D" not in vita_gl + vita_gl_compat,
        "duplicate Vita DXT1 decoder must remain removed")
require("dxt_decode_level(information.kind, source" in textures and
        "GL_BGRA, GL_UNSIGNED_BYTE, converted" in textures,
        "decoded DXT must retain original mip/face upload ownership")

# The current NV2A fragment translator never reads the legacy B0/B1 vertex
# outputs. Vita has a tight varying budget, so the compiler boundary removes
# only the exact generated xB declarations/assignments while leaving the NV2A
# oB0/oB1 register arithmetic intact. This keeps the active interface at
# D0/D1 + T0..T3 + Fog instead of spending two vectors on dead data.
require("#define glShaderSource halo_vita_glShaderSource" in vita_gl,
        "Vita generated shaders must pass through the varying compaction boundary")
for exact_line in (
    '"varying vec4 xB0;\\n"',
    '"varying vec4 xB1;\\n"',
    '"\\txB0 = clamp(oB0, 0.0, 1.0);\\n"',
    '"\\txB1 = clamp(oB1, 0.0, 1.0);\\n"',
):
    require(exact_line in vita_gl_compat,
            f"Vita dead-varying compaction lost exact source contract: {exact_line}")
require("fragment interface is D0/D1 + T0..T3 + Fog" in vita_gl_compat,
        "Vita dead-varying compaction must remain observable in hardware logs")

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

print("PASS: Vita UI renderer contracts: 640x480 author space into 640x480 target, "
      "fan topology, viewport transform, Xbox/GL blend enums, texture-stage "
      "selection, DXT1-to-BGRA boundary, compact NV2A varyings, post-resource "
      "raster restore and GL shadow invalidation")
