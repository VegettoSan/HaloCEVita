# Upstream renderer alignment audit — 2026-10-03

## Scope

This audit follows the 00.38 real-Vita test and uses the current public `cybersecurity/halo-ce-universal` renderer as the behavioral authority.  The goal is not to design a Vita menu renderer.  The required pipeline remains:

`Halo UI/world code -> original rasterizer -> Xbox D3D8/NV2A state -> upstream d3d8_gl/nv2a_vsh/nv2a_psh -> Vita compatibility boundary -> vitaGL -> GXM`.

No retail widget coordinates, bitmap data, UI layout, register-combiner equations, texture-stage equations or draw ordering are to be replaced by Vita-specific equivalents merely to improve a screenshot.

## 00.38 hardware result

The 00.38 package (`ade165ab4fe849667a9e40c7fb2a887765e401d3`) boots the original Main Menu and compiles/links the two observed original NV2A shader pairs, but the user reports no visible improvement in the misplaced UI or white rectangles.  The generated vertex shaders contain the previously added mobile `clip_position` / `clip_captured` path, so that experiment is now hardware-rejected as the explanation for these menu symptoms.  Keep the upstream-compatible path, but do not continue modifying clip geometry to chase this defect.

The same run confirms the upstream-style map cache is not the current visual boundary: `ui.map` is rebuilt into committed `cache002.map`, mounted, and resources are read from that logical stream.  DXT UI masks retain real authored alpha and a sampled 256x64 mask reaches the GPU with transparent and non-zero-alpha samples.  Do not re-open the old temporary-map or forced-alpha hypotheses without new contradictory evidence.

## Original UI draw path checked

The current upstream `source/interface/ui_widget.c::draw_bitmap_in_rect()` remains the semantic owner.  For the normal plasma UI path it submits exactly:

- map 0 = the interface plasma bitmap;
- map 1 = the same plasma bitmap;
- map 2 = the authored widget bitmap;
- map 0 texture scale = 1/311;
- map 1 texture scale = 1/201;
- map 2 texture scale = 1;
- framebuffer blend function = alpha blend;
- the four original dynamic-screen vertices to `rasterizer_psuedo_dynamic_screen_quad_draw()`.

The 00.38 first draw reports three active project2D stages (`PSTEXTUREMODES=0x421`) and hardware bindings `tex0=2, tex1=2, tex2=3`, which is consistent with this original plasma/plasma/widget ownership.  Do not "fix" the duplicate first two texture IDs; they are required by the decomp.

The 4x4 opaque white texture observed in the sequence is the retail `ui\shell\bitmaps\white`.  It is intentional source data and must be modulated through Halo's original texture-stage/register-combiner state.  It must not be deleted, made transparent, color-keyed or replaced with a Vita rectangle.

## Shader system alignment

`nv2a_vsh.c` and `nv2a_psh.c` must remain the owners of NV2A arithmetic.  Their generic GL output intentionally models the complete Xbox register files.  That representation is legal for desktop GL but exceeds vitaGL's reported hardware-facing budgets:

- vitaGL `GL_MAX_VERTEX_UNIFORM_VECTORS`: 128;
- vitaGL `GL_MAX_FRAGMENT_UNIFORM_VECTORS`: 16.

The 00.38 first UI pixel program reflected all of `ps_c0[8]`, `ps_c1[8]` and `texture_scale[4]`: already 20 vec4 uniforms before other uniforms.  Its generated source actually references only `ps_c0[0]`, `ps_c0[1]`, `ps_c0[6]`, `ps_c1[0]` and `texture_scale[0..2]`.  The observed vertex programs declare `c[192]` but use a fixed prefix ending at `c[59]`.

This is a concrete platform-contract divergence.  Successful GLSL compilation is not evidence that asking ShaccCg/GXM for register files larger than the advertised limits preserves every value correctly.

## A117 — GL-boundary uniform-prefix specialization

Commit `1d319797339d83572605d34759965b9dc577367e` implements the first correction at the Vita GL boundary only.  It deliberately does **not** modify `nv2a_vsh.c`, `nv2a_psh.c`, `ui_widget.c`, the screen-quad code or retail data.

The adapter receives the already-generated upstream GLSL and shortens only array declarations to the highest statically referenced original register.  Register numbers and equations are unchanged; for example `c[59]` remains `c[59]`, and `ps_c0[6]` remains `ps_c0[6]`.  Existing reflected-array upload code then uploads the same prefix from Halo's original CPU register shadows.

The adapter queries vitaGL's actual reported vertex/fragment limits and fails explicitly if a shader still cannot fit.  Vertex shaders using relative `c[clamp(a0 + ...)]` addressing are rejected rather than silently remapped; they require a separately proven bounded-window strategy if encountered later.

The adapter logs the effective spans on hardware:

- `[VITA SHADER ABI] upstream vertex c[] prefix=.../192 vitaGL_limit=...; register indices unchanged`
- `[VITA SHADER ABI] upstream pixel prefixes c0=... c1=... bump=... lum=... tex=... active_vec4<=... vitaGL_limit=...`

This is a representation adaptation required by vitaGL, not a new Halo shader system.

## Screen / presentation alignment

The source comparison continues to preserve Halo's 640x480 author/render coordinate space.  The original `main_pregame_render()` / `render_frame_pregame()` / `render_frame_present()` path owns frame construction.  `d3d8_gl.c` owns the D3D render target, viewport, raster state and final aspect-preserving presentation.  The Vita platform swap must remain a swap only; it must not add a second fullscreen scaling/blit pass.

A numerical 622x402 interpretation considered during the audit was rejected: the logged `c32/c33` values belong to dynamic-screen map/texture constants, not the D3D viewport.  Do not use that discarded coincidence to rescale the UI or title-safe frame.

## Current classification

**Verified facts**

- 00.38 executes the original UI/game runtime and original pregame rendering caller.
- `clip_position` is present in the generated 00.38 vertex shaders and did not change the user's visual result.
- the first plasma draw's texture-stage ownership matches upstream plasma/plasma/widget behavior.
- authored mask alpha survives cache read, CPU decode and observed GPU sampling.
- vitaGL reports 128 vertex and 16 fragment uniform vectors.
- the 00.38 reflected pixel arrays exceed that fragment-vector budget in their generic form.

**Hardware-pending correction**

- A117 prefix specialization keeps original register indices/equations but brings the generated interface inside vitaGL's declared uniform budget for fixed-index shaders.

**Not established**

- A117 is not yet proven to be the sole or final cause of white rectangles or misplaced UI.
- no new Campaign/gameplay acceptance follows from this renderer audit.

## Continuation rule

If A117 still renders identically on hardware, continue downward through the original pipeline in order: reflected uniform values -> sampler objects / texture coordinates -> actual register-combiner output -> FBO destination -> final Present.  At each step compare against upstream state before adding a Vita adaptation.  Do not replace the original shader equations, widget renderer, bitmap data or UI coordinates.