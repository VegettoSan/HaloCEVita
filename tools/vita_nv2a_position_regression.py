#!/usr/bin/env python3
"""Keep Vita on upstream's precise mobile NV2A position reconstruction.

This is a source-contract regression.  The Vita backend cannot use desktop
GL's glClipControl, so it must retain the same pre-rcc clip position that the
upstream mobile path uses.  Do not replace this with UI-coordinate fixes.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / "port/linux/src/nv2a_vsh.c").read_text()
mobile_guard = "#if defined(HALO_ANDROID) || defined(HALO_VITA)"

required = [
    "vec4 clip_position = vec4(0.0);\\n\\tbool clip_captured = false;",
    "clip_position = oPos;\\n\\tclip_captured = true;",
    "if (clip_captured)\\n",
    "clip_position.xyz * c[%d].xyz",
    "- viewport_offset.xyz) * clip_position.w) / scale, clip_position.w",
    ", XGPU_VERTEX_CONSTANT_BIAS - 38, XGPU_VERTEX_CONSTANT_BIAS - 37",
]
for needle in required:
    assert needle in source, f"missing upstream precise-position contract: {needle}"

# The four upstream mobile ownership points must be active for Vita: generated
# locals, capture at rcc(r12.w), precise epilogue and c[-38]/c[-37] arguments.
for needle in (
    'xgpu_text_append(&text, "\\tvec4 clip_position',
    "the screen-space conversion takes the reciprocal",
    "The conversion is screen = clip * c[-38]",
    ", XGPU_VERTEX_CONSTANT_BIAS - 38",
):
    pos = source.index(needle)
    guard = source.rfind("#if", 0, pos)
    endif = source.rfind("#endif", 0, pos)
    assert guard > endif, f"{needle!r} is not inside a preprocessor guard"
    line = source[guard:source.find("\n", guard)]
    assert line == mobile_guard, f"Vita excluded from {needle!r}: {line}"

# Android still owns its runtime #version line.  Defining Android globally on
# Vita would emit a second version directive and is not an acceptable shortcut.
version = 'xgpu_text_append(&text, "#version %s\\n", xgpu_capabilities.shading_language);'
pos = source.index(version)
guard = source.rfind("#ifdef", 0, pos)
assert source[guard:source.find("\n", guard)] == "#ifdef HALO_ANDROID"

# Vita continues to apply the mobile upper-left / zero-to-one clip convention
ydepth = "gl_Position.y = -gl_Position.y;\\n"
pos = source.index(ydepth)
guard = source.rfind("#if", 0, pos)
assert source[guard:source.find("\n", guard)] == mobile_guard
assert "gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;\\n" in source[pos:pos + 200]

print("PASS Vita NV2A position path: upstream pre-rcc clip capture/reconstruction + Vita Y/depth, Android version prefix remains isolated")
