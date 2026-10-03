# Original UI bitmap/render path audit — 2026-10-03

## Scope

This audit was started from real Vita 00.37 evidence where the Main Menu still
showed misplaced UI/text and white rectangular bitmap backgrounds even after
the cache subsystem had been aligned with the original persistent cache-slot
lifecycle. The user also observed that the white rectangles become visually
hidden during the profile/settings fade and can disappear after extensive
keyboard navigation.

The goal is deliberately **not** to edit widget coordinates, retail bitmap
pixels, authored alpha, or `ui.map`. The goal is to preserve the original
Halo UI draw path and correct only platform/backend differences required for
vitaGL.

## Authority

Primary reference inspected directly:

```text
https://github.com/cybersecurity/halo-ce-universal
commit 80d30410c8db28f4008b92f4e012a1b046ece14e
```

Important files compared directly against current HaloCEVita `main`:

- `source/interface/ui_widget.c`
- `source/rasterizer/xbox/rasterizer_xbox_dynavobgeom.c`
- `port/linux/src/d3d8_gl.c`
- `port/linux/src/nv2a_vsh.c`
- `port/linux/src/nv2a_psh.c`
- `port/linux/src/xbox_textures.c`

The current VitaSDK vitaGL package recipe was also checked against
`vitasdk/packages` and the corresponding vitaGL texture implementation.

## Hardware evidence that moves the fault above the cache boundary

The supplied `cache002.map` is 33,582,080 bytes and has SHA-256
`8556b647b82742d07282fe4e6db0cf847b542c046484e91e71ec31fea5f5fd7e`,
the same exact logical image independently reconstructed from the supplied
compressed `ui.map` during the cache audit. The runtime log also reports the
expected tag CRC `e22586e4`, 983 tags, and shared tag/resource consumption from
the committed `cache002.map`.

Therefore this test supplies no evidence of map/cache corruption. Rendering
must be investigated after the original resource/cache boundary.

The first DXT3 UI mask in the log contains authored alpha 0..119 with 14,537
zero-alpha texels. The exact visible-shader probe reports a transparent corner
(alpha 0) and a semitransparent centre (alpha 118) while the native blend state
is `SRC_ALPHA / ONE_MINUS_SRC_ALPHA` and GL reports no error. This proves that
at least this sampled mask reaches the pixel-shader/blend boundary with its
transparency still present. It does not prove every visible draw is correct.

## Original widget/bitmap path is already the owner

Direct comparison of `draw_bitmap_in_rect()` with upstream confirms that the
current original-runtime target already keeps Halo's authored widget path:

1. the widget supplies its authored bounds/clip rectangle;
2. `draw_bitmap_in_rect()` constructs the same four screen-space vertices;
3. the same bitmap index/UV rectangle is used;
4. non-plasma images bind the authored bitmap as map 0;
5. plasma images use the original interface map stages and authored tint/time;
6. the draw remains `rasterizer_psuedo_dynamic_screen_quad_draw()`;
7. primitive topology remains the original four-vertex `D3DPT_TRIANGLEFAN`.

No Vita correction in this audit rescales retail widget coordinates, changes
bitmap UVs, removes white RGB pixels, forces widget alpha, or invents a
replacement UI renderer.

## The observed 4x4 white bitmap is an intentional Halo asset

The hardware log loads a 4x4 DXT1 bitmap immediately before a 256x64 menu mask.
Resolving that datum in the supplied `cache002.map` identifies it as:

```text
ui\shell\bitmaps\white
```

Its DXT1 block decodes to opaque white texels. This is not corruption, a Vita
placeholder, or a failed alpha decode. Original Halo deliberately uses this
bitmap as one of its screen-geometry inputs. The correct renderer must combine
it with Halo's authored vertex color, map tints/fades, plasma maps and NV2A
register combiners. Making the bitmap transparent, deleting it, or replacing
its pixels would diverge from the decomp and is explicitly rejected.

Direct inspection of `_rasterizer_psuedo_dynamic_screen_quad_draw()` confirms
that upstream builds the same pixel-shader definition from `map_tint`,
`map_fade`, plasma constants and texture-stage blend functions, then submits
the same four immediate vertices. HaloCEVita continues to execute this original
owner; Vita-specific work remains below it in D3D8/GL/shader translation.

## What the observed fade actually does

The original UI fade does not repair or rewrite bitmaps. `ui_widget.c` draws
the widget hierarchy and then, when fade is nonzero, draws a full-screen black
quad with increasing alpha. Therefore the user's observation that white
rectangles disappear while the save/settings window fades darker is compatible
with the original black overlay simply covering/blending over the bad visible
result. It is useful evidence about composition/state, but it is not evidence
that bitmap bytes become correct during the fade.

## Immediate-mode screen vertex path comparison

The menu screen quads are submitted through original immediate D3D8 calls.
Current HaloCEVita and the audited upstream use the same semantics:

- `D3DDevice_SetVertexData2f/4f` update the same NV2A input-register array;
- writing `D3DVSDE_VERTEX` commits the complete current register set as one
  immediate vertex;
- `D3DDevice_End()` uploads the same 16 x vec4 register layout;
- the shader receives position at v0, screen texture coordinates at v4 and
  color at v9.

Vita adds diagnostics and its bounded stream upload, but no alternate UI vertex
packing was found in this path. Therefore this audit does not introduce a
handwritten immediate-vertex conversion.

## Pixel-shader constants / combiner boundary

The original screen-quad owner writes map colors into the Xbox PS constant
render states and constructs the original combiner words. HaloCEVita converts
those same `D3DRS_PSCONSTANT*` values with the same `color_to_vec4()` path.
The Vita-only difference is uniform-array reflection/upload because the linked
vitaGL uniform ABI exposes a whole array base rather than independently useful
array-element handles.

For the faulty run, program 1 reports an eight-vector `ps_c0` span and the
bounded GPU probe observes the expected first map tint value near 0.9. The
sampled menu-mask shader output also preserves transparent/sem透明 alpha. No
source-confirmed index shift or missing PS constant was found in this review.
Do not replace the original combiners or hardcode a tint merely because the
intentional white bitmap is visible.

## Source-confirmed Vita divergence: precise mobile clip position was excluded

Upstream's mobile vertex-shader path cannot use desktop
`glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE)`. To avoid reconstructing the
Xbox clip position from an already screen-space/reciprocal-transformed `oPos`,
it records the pre-`rcc(r12.w)` position in `clip_position`, then uses the
original `c[-38]`/`c[-37]` viewport constants to reconstruct `gl_Position`
without the lossy divide/multiply round trip. The mobile path then applies the
upper-left Y convention and 0..1-to-OpenGL depth conversion.

HaloCEVita had previously widened only the final Y/depth conversion to Vita.
The four precise-position ownership points remained `HALO_ANDROID` only:

- generated `clip_position` / `clip_captured` locals;
- capture when the program executes `rcc(r12.w)`;
- the precise `clip_position` epilogue;
- the `c[-38]` / `c[-37]` format arguments required by that epilogue.

The user's generated `halo_vertex_00.glsl`, `halo_vertex_01.glsl`, and
`halo_vertex.glsl` from the faulty hardware run contain no `clip_position`,
confirming that Vita was taking the less precise fallback rather than the
upstream mobile reconstruction.

### Applied correction

Commit `9c8d1d1001bb7408f1a410da963ddb00f13b4a4c` widens exactly those four
upstream mobile guards to:

```c
#if defined(HALO_ANDROID) || defined(HALO_VITA)
```

It does **not** define `HALO_ANDROID` globally on Vita. Android's runtime
`#version` prefix remains Android-only, while Vita retains its existing GLSL
syntax/prologue and vitaGL/ShaccCg adaptations. The original NV2A arithmetic,
registers, Halo vertices, UI coordinates, viewport constants and bitmap data
remain unchanged.

`tools/vita_nv2a_position_regression.py` protects all four ownership points,
the upstream reconstruction expression, the Vita Y/depth conversion, and the
Android-only version prefix. The Vita CI workflow now runs this contract.

## Dynamic font texture refresh investigation

The faulty runtime log repeatedly shows the same dynamic font texture being
drawn and then refreshed by `glTexSubImage2D`. HaloCEVita logs this as a
vitaGL copy-on-write (COW) hazard boundary.

The actual vitaGL implementation was inspected rather than assuming that log
message was true. With normal texture semantics, if the texture was used in a
recent frame, `_glTexSubImage2D` allocates new GPU-visible storage, copies the
old texture into it, marks/frees the old storage through vitaGL ownership, and
then writes the subimage to the new storage. This preserves already submitted
draws without a global `glFinish()`.

The official VitaSDK package recipe builds vitaGL with `NO_DEBUG=1` only. It
does **not** enable `TEXTURES_SPEEDHACK`; vitaGL documents that speedhack as the
mode that makes `glTexSubImage2D` non-fully-compliant and bypasses this normal
protection. Consequently this audit does not add a per-glyph `glFinish()` or
rewrite the font atlas lifetime. Doing so would change scheduling without a
source-confirmed defect in the library actually linked by CI.

## CI result

Vita Build run 264 (`37100721566`) on executable source commit
`2bcba55d90c45f8a44afb203ebcbef6729f18caf` passes:

- all required menu/audio/cache/renderer host contracts, including the new
  precise-position regression;
- native ARM link;
- ELF layout inspection;
- SELF/VPK generation;
- native package verification;
- artifact upload and prerelease publication.

The produced package remains `LINKS` until this exact shader-position change is
observed on real Vita hardware.

## Current conclusions

### Verified facts

- The persistent `cache002.map` bytes are exact for the supplied run.
- Original `draw_bitmap_in_rect()` remains the owner of bitmap geometry/UVs.
- `ui\shell\bitmaps\white` is an intentional opaque-white Halo bitmap.
- Sampled DXT mask alpha survives CPU decode, shader and blend probes.
- Original fade is a black overlay; it does not mutate the underlying bitmap.
- Immediate UI vertex register packing matches upstream semantics.
- No missing/shifted PS constant was demonstrated by the sampled program.
- Vita was missing upstream's precise mobile `clip_position` reconstruction.
- The official VitaSDK vitaGL build does not enable `TEXTURES_SPEEDHACK`.

### Applied

- Reuse upstream's precise mobile NV2A clip-position path on Vita.
- Add a regression that rejects a future return to the old Vita fallback.
- Keep original widget/bitmap/tag data and original screen-quad combiners
  unchanged.

### Not yet proven

- That the position correction alone fixes all misplaced UI/text on real Vita.
- That the white rectangles have the same root cause as the misplaced geometry.
- That every UI shader/state combination is correct merely because the sampled
  alpha probe is correct.

A fresh Vita run must check the first cold Main Menu, keyboard screens, and the
profile/settings fade. The newly generated vertex shaders should now contain
`clip_position`. If geometry is corrected but white rectangles remain, the
next investigation stays below the original widget layer and compares the
specific visible `ui\shell\bitmaps\white` draw's texture-stage/pixel-combiner
and raster state against upstream; retail data/coordinates must remain
untouched.
