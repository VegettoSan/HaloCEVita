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


def require(condition, message):
    if not condition:
        raise SystemExit(f"FAIL: {message}")


renderer = RENDERER.read_text(encoding="utf-8")
xgpu = XGPU.read_text(encoding="utf-8")

# UI widgets from the retail cache legitimately submit triangle fans.  Losing
# this mapping silently turns four-vertex widget quads into the wrong topology.
fan_case = renderer.find("case D3DPT_TRIANGLEFAN:")
fan_gl = renderer.find("GL_TRIANGLE_FAN", fan_case)
require(fan_case >= 0 and fan_gl > fan_case,
        "D3DPT_TRIANGLEFAN must map to GL_TRIANGLE_FAN")

# Halo's viewport constants use top-left D3D coordinates.  The GL shader bridge
# performs the Y inversion in viewport_scale; author-space widget coordinates
# must not be rewritten to compensate a second time.
require("device.viewport_scale[1] = -(float)device.viewport.Height * 0.5f;" in renderer,
        "viewport Y scale must remain negative for D3D top-left semantics")
require("device.viewport_offset[1] = device.viewport.Y + device.viewport.Height * 0.5f;" in renderer,
        "viewport Y offset must remain the positive D3D half-height offset")

# Vita resource uploads can mutate GL bindings/state behind the device's state
# cache.  On Vita, the full raster contract must be applied *after* textures
# have been resolved/uploaded, immediately before the draw state is consumed.
bind = renderer.find("bind_textures(&key, uniforms.texture_scale);")
require(bind >= 0, "prepare_draw must bind/resolve textures")
vita_guard = renderer.find("#ifdef HALO_VITA", bind)
raster = renderer.find("apply_raster_state(has_depth);", bind)
require(vita_guard >= 0 and bind < vita_guard < raster,
        "Vita raster state must be re-applied after texture preparation")
require("Resource uploads/mip composition invalidate or change GL state" in renderer,
        "post-upload raster-state rationale must remain documented in code")

# Any lower layer that changes GL state behind d3d8_gl must invalidate the
# shared shadow cache.  Keeping the explicit boundary prevents warm/cold draw
# behavior from diverging again.
require("void xgpu_gl_state_invalidate(void);" in xgpu,
        "xgpu GL-state invalidation boundary must remain declared")
require("code that\nchanges GL state behind it" in xgpu,
        "xgpu invalidation contract must remain documented")

print("PASS: Vita UI renderer contracts: fan topology, viewport transform, "
      "post-resource raster restore and GL shadow invalidation")
