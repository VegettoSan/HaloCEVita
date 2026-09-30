# HaloCEVita — vitaGL renderer and NV2A adaptation plan

Last research pass: 2026-09-29

This document is the renderer/shader engineering plan for the native PS Vita port. It is intentionally separate from `STATUS.md`: the local working tree and hardware logs may be ahead of the public GitHub branch. Before changing renderer code, inspect the current local tree first.

## 1. Goal and non-goals

The target renderer path is:

```text
Halo original renderer
  -> Xbox D3D8/NV2A state
  -> upstream d3d8_gl.c / d3d8_resources.c / xbox_textures.c
  -> nv2a_vsh.c + nv2a_psh.c
  -> Vita-compatible GLSL subset
  -> vitaGL / VitaShaRK / libshacccg
  -> SceGxm
```

The goal is to preserve Halo's original rendering semantics and adapt the already-existing D3D8/NV2A translation layer to the capabilities of vitaGL. Do not replace this with a simplified hand-written renderer, fixed material system, fake D3D8 success layer, or map viewer.

A diagnostic triangle proves only that vitaGL, runtime shader compilation and the basic GPU path work. It does not prove the Halo renderer works.

Graphics milestones remain:

- `DIAGNOSTIC RENDERS`: authored test geometry/shader path works.
- `HALO DRAW REACHED`: original Halo code issued the real state/draw sequence.
- `HALO DRAW RENDERS`: that original draw produced the expected visible output.

## 2. Sources of truth used for this plan

Halo-side files:

- `port/linux/src/d3d8_gl.c`
- `port/linux/src/d3d8_resources.c`
- `port/linux/src/xbox_textures.c`
- `port/linux/src/nv2a_vsh.c`
- `port/linux/src/nv2a_psh.c`
- `port/linux/src/xgpu.h`
- `port/linux/src/memory_watch.c`
- `port/vita/include/halo_vita_graphics.h`
- `docs/GRAPHICS_COMPATIBILITY.md`

vitaGL-side reference checked during this research:

- `Rinnegatamante/vitaGL` master commit `464876a79cdd00650bb0eb9629b85d81f9908fd8` (2026-09-26).

The installed VitaSDK/vitaGL headers and archive used by the project are still the runtime/build authority. If installed vitaGL and current upstream vitaGL differ, record the exact installed hashes/version before changing behavior.

## 3. What the upstream Halo renderer actually does

The GL backend is not a thin set of draw wrappers. It reconstructs important NV2A behavior:

1. Xbox vertex declarations are parsed into streams, offsets and data formats.
2. Xbox NV2A vertex microcode is translated dynamically to GLSL.
3. Xbox pixel texture-shader stages and register combiners are converted dynamically to a GLSL fragment shader.
4. Render-state tables are applied immediately before each draw.
5. Textures are decoded from Xbox layouts/formats and cached as GL objects.
6. Render targets are keyed by Xbox physical resource addresses.
7. Render-to-texture and mip-level composition are supported.
8. Static guest vertex/index memory can be mirrored to GPU buffers, while volatile data is streamed.
9. Visibility queries are used by game systems such as lens flares.
10. Shader/program caches avoid recompiling equivalent NV2A state repeatedly.

Therefore a correct Vita port must adapt the backend as a system. Implementing only `Draw*`, `SetTexture` and a few states is insufficient.

## 4. Important vitaGL limits confirmed by source

Current vitaGL reports/defines approximately:

| Capability | Current vitaGL value / state | Halo consequence |
| --- | --- | --- |
| Vertex attributes | 16 | Matches NV2A's 16 input registers well. |
| Combined texture image units | 16 | Halo pixel path needs at most four stages; not a blocker. |
| Vertex uniform vectors | 128 | Halo exposes 192 NV2A vertex constant registers; fixed `c[192]` is unsafe. |
| Fragment uniform vectors | 16 | Current generic pixel GLSL declarations can exceed this; shader specialization is required. |
| Varying vectors | 8 | Current Vita VS declares more output data than the pixel translator actually needs; compact varyings. |
| Runtime GLSL compiler | VitaShaRK / `libshacccg.suprx` | Required on hardware for generated Halo shaders. |
| 2D DXT1/3/5 | Supported by vitaGL/SceGxm | Prefer direct compressed upload after validation. |
| 3D texture upload/sampler path | Not exposed as ordinary `glTexImage3D`/`sampler3D` path | Requires fallback. |
| `glBlendColor` | Not exposed | Only emulate if Halo actually uses constant blend factors. |
| `glDrawBuffers` | Not exposed | Halo backend only needs a narrow single-color/depth FBO contract; specialize it. |
| `glCopyImageSubData` | Not required | Use FBO + `glBlitFramebuffer` fallback. |
| Persistent `glBufferStorage` query path | Not suitable | Use ordinary buffer/stream paths and direct query reads. |
| Integer vertex attributes | Not part of the safe Vita path | CPU-unpack Xbox `NORMPACKED3`. |

Do not infer full OpenGL compatibility from the presence of a token or function prototype. Validate the exact behavior Halo relies on.

## 5. Critical issue: attribute locations must be explicit on Vita

Desktop shaders use `layout(location = N)` for Xbox vertex register `vN`. The current Vita GLSL 1.20 path uses names such as:

```glsl
attribute vec4 v0_in;
attribute vec4 v1_in;
```

GLSL 1.20 does not provide `layout(location=N)`. Meanwhile `d3d8_gl.c` points vertex data to the original Xbox register indices with `glVertexAttribPointer(N, ...)`.

Therefore every Vita program must bind each used attribute before link:

```text
glCreateProgram
  glAttachShader(vertex)
  glAttachShader(fragment)
  glBindAttribLocation(program, N, "vN_in")   for every used N
  glLinkProgram
```

Do not rely on the GLSL linker assigning attributes in the same order. This should be one of the first corrections when the real Halo draw path is connected.

Acceptance test: log the shader/program ID, declaration register and bound Vita attribute location for the first real Halo draw, then verify that the first vertex values reach the expected shader registers.

## 6. Vertex shader plan

### 6.1 Keep the existing NV2A translator

`nv2a_vsh.c` already decodes the original four-word NV2A instructions, including:

- MAC: MOV, MUL, ADD, MAD, DP3, DPH, DP4, DST, MIN, MAX, SLT, SGE, ARL;
- ILU: MOV, RCP, RCC, RSQ, EXP, LOG, LIT;
- swizzles and negation;
- temporary/input/constant sources;
- relative constant addressing through `a0`;
- output registers `oPos`, diffuse/specular values, fog, point size and texture coordinates.

Do not replace this with per-material Vita shaders.

### 6.2 Clip-space conversion

The desktop backend uses `glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE)` to mimic D3D conventions. The Vita/Android shader path already converts this in the generated vertex shader by flipping Y and remapping Z from D3D 0..1 to GL -1..1.

Keep this shader-side approach on Vita. Validate it with real Halo UI geometry and then 3D geometry. Do not add a second flip in presentation/FBO code unless hardware evidence proves it is required.

Tests:

- UI quad orientation and winding;
- scissor/viewport top-left behavior;
- depth test on two overlapping triangles;
- culling on a known clockwise/counter-clockwise mesh;
- render-to-texture orientation.

### 6.3 Xbox has 192 vertex constants; Vita reports 128 vectors

The current Vita prologue declares `uniform vec4 c[192]`. This is not a safe long-term contract when vitaGL reports 128 vertex uniform vectors.

Preferred solution: **per-shader constant compaction**.

For each translated NV2A program, collect metadata while decoding instructions:

```text
used constant registers
uses relative addressing?
relative base register immediates
used input registers
written output registers
```

If the program has no relative constant addressing:

1. Create a compact mapping `xbox_constant -> vita_uniform_slot`.
2. Emit only the constants the shader actually references.
3. Store the reverse mapping in the compiled vertex-program object.
4. Upload only changed original Halo constants that map to an active Vita slot.

Do not assume 192 declarations will be optimized away reliably by the runtime compiler.

For shaders using `a0` relative addressing, do not silently clamp to a smaller table. First instrument which real shaders use it and what range they require. Possible fallbacks, in preferred order:

1. compact a proven bounded contiguous constant window for that shader/use;
2. create a specialized variant if the actual allowed range can be derived from the Halo draw/model contract;
3. investigate a confirmed Vita-supported alternate storage/fetch path only if real shaders exceed the uniform budget.

Uniform buffers, vertex texture fetches or direct GXM shader paths are not the default solution; they require separate validation before adoption.

### 6.4 `NORMPACKED3`

Xbox `D3DVSDT_NORMPACKED3` relies on packed integer bits. The desktop GLSL path uses integer attributes/bit shifts, which are not the safe GLSL 1.20 Vita route.

Preferred fallback: unpack on CPU during stream upload into a temporary float attribute stream.

Requirements:

- preserve Xbox signed 11/11/10 normalization exactly;
- keep original Xbox declaration/register numbering;
- update the Vita vertex declaration metadata so the shader sees normal `vec4`/`vec3` data;
- cache/reuse converted static ranges later if profiling justifies it;
- correctness first: streaming conversion is acceptable for the first Halo draw/menu.

### 6.5 `D3DCOLOR` attributes

Desktop can use the special BGRA vertex attribute form. vitaGL's regular attribute path supports normalized unsigned bytes but should not be assumed to support desktop's `size = GL_BGRA` attribute convention.

Use the already-proven Android-style CPU B/R byte swap for `D3DVSDT_D3DCOLOR` on the Vita stream path unless a dedicated hardware test proves the Vita attribute path preserves Xbox color order directly.

### 6.6 Other vertex formats

`FLOAT*`, `SHORT*`, normalized shorts and packed bytes have direct or near-direct GXM/vitaGL attribute representations. Validate every type with a tiny contract draw before marking it supported. Do not silently reinterpret an unsupported type as float4.

## 7. Varying/output budget

Current Vita vertex GLSL declares:

- `xD0`, `xD1`;
- `xB0`, `xB1`;
- `xT0..xT3`;
- `xFog`.

Current `nv2a_psh.c` consumes diffuse values, texture coordinates and fog; the B outputs are not part of the current fragment translator's register inputs. Keeping every possible output declared is undesirable with an 8-vector varying limit.

Preferred solution:

- generate only varyings actually consumed by the paired fragment shader;
- at minimum, remove unused B varyings from the Vita program pair if the current translator never consumes them;
- emit only active texture-coordinate varyings when practical;
- keep a per-program varying mask in shader metadata;
- if a later path genuinely requires more data, pack compatible scalar values before changing architecture.

The vertex and fragment generator must agree on the exact varying interface for each linked program.

## 8. Pixel shader plan

### 8.1 Preserve NV2A texture stages and register combiners

`nv2a_psh.c` already translates:

- four Xbox texture-shader stages;
- projective 2D/3D coordinates;
- cube maps;
- passthrough and clip-plane modes;
- bump environment mapping and luminance;
- dot-product/reflection/dependent texture modes;
- up to eight general register-combiner stages;
- final combiner;
- signed texture channels;
- alpha kill/test;
- fog.

Keep this logic. Vita changes should affect syntax/resource representation, not the NV2A equations.

### 8.2 Fragment uniform budget is a first-class constraint

vitaGL reports 16 fragment uniform vectors. The generic translator currently declares broad arrays and feature uniforms even when a particular shader uses only a subset.

Implement **per-pixel-shader uniform specialization**:

- only declare `ps_c0`/`ps_c1` constants for active combiner stages;
- prefer separately named constants (`ps_c0_3`) over a fixed eight-element array when static stage indexing permits it;
- declare final-combiner constants only when the final combiner references them;
- declare fog uniforms only when fog is enabled;
- declare alpha reference only when alpha test is enabled;
- declare bump matrices/luminance only for stages using bump modes;
- declare texture scale only for active sampled stages;
- declare LOD-bias data only if the selected Vita sampling implementation requires it.

Return generated-source metadata together with the GLSL source so `program_get()` knows exactly which uniform names/locations exist.

Recommended metadata concept:

```text
vertex constants mapping
vertex input mask
varying mask
active sampler mask/type
active ps-c0/c1 stage masks
uses final c0/c1
uses fog
uses alpha reference
uses bump matrix/luminance
uses texture scale / LOD bias
uses volume fallback
```

This simultaneously fixes resource limits, reduces shader compile work and reduces per-draw uniform uploads.

### 8.3 Uniform array locations

The existing backend has an optimization that assumes array element locations may be consecutive. vitaGL explicitly offers `SAFE_UNIFORMS=1` to make basic-type array location indexing more compliant.

Bring-up recommendation:

- use a vitaGL build with `SAFE_UNIFORMS=1`, or
- remove the consecutive-location assumption on Vita and cache each required uniform location explicitly.

The second approach is safest if uniform specialization eliminates most large arrays.

## 9. Texture pipeline

### 9.1 Keep Xbox decoding logic

`xbox_textures.c` already handles Xbox texture headers, Morton swizzling, linear pitch, mip layout, palettes and many formats. Preserve it.

Supported source categories include:

- DXT1/DXT3/DXT5;
- common 32/16-bit RGB(A) formats;
- L8/AL8/A8/A8L8/L16;
- P8 palettes;
- V16U16 and other signed/packed data formats;
- YUY2/UYVY;
- cube maps;
- depth formats;
- volume layouts.

### 9.2 2D DXT

Current vitaGL explicitly maps DXT1/3/5 to native SceGxm compressed formats. Therefore the preferred path is direct `glCompressedTexImage2D` for Xbox DXT blocks.

Still add a hardware contract test for:

- DXT1 punch-through alpha;
- DXT3 alpha;
- DXT5 interpolated alpha;
- mip chains;
- NPOT edge blocks if Halo supplies them.

Fallback: reuse the existing software DXT decoder path and upload RGBA/BGRA if a specific texture/layout fails.

### 9.3 BGRA and uncompressed formats

vitaGL supports BGRA texture upload/read paths. Preserve the CPU conversion from Xbox formats to the known 32-bit layout, but validate channel order with a four-color test texture before trusting retail assets.

### 9.4 Palettized and YUV

P8 palette expansion and YUV conversion can remain CPU-side. These are safer than inventing Vita-specific indexed/YUV GPU formats during bring-up.

### 9.5 Cube maps

Use ordinary cube textures through vitaGL and retain the existing six-face/mipmap Xbox layout. Validate face order/orientation with a diagnostic cube before Campaign reflection debugging.

### 9.6 Volume / 3D textures

Do not silently replace `sampler3D` with `sampler2D` or ignore depth.

The practical Vita fallback is a **2D slice atlas** because the known retail volume bitmaps are small.

Proposed implementation:

1. Decode each volume mip level into individual Z slices using the existing Xbox swizzle/format decoder.
2. Pack the slices into a 2D atlas.
3. Store atlas geometry in the texture cache entry: slice width/height, depth, atlas grid, mip information.
4. For a pixel shader stage whose sampler type is 3D, generate a Vita-only `sample_volume_2d()` helper.
5. Convert `(x,y,z)` to the two adjacent slice UV rectangles and linearly blend them for trilinear-in-Z behavior.
6. Respect clamp/wrap semantics explicitly.
7. For compressed DXT volume data, decode to RGBA before atlas upload.

Initially this can be level-0 only if instrumentation proves the affected real shader does not require volume mip selection. Any fidelity reduction must be logged and documented, not hidden.

Do not use `UNPURE_TEXFORMATS=1` as proof of native `sampler3D` support; vitaGL documents that non-2D texture storage support still expects `tex2D` shader code.

## 10. Sampler state

Halo uses address modes, min/mag/mip filtering, maximum mip level, LOD bias, anisotropy and border color.

### Direct/likely direct

- repeat;
- mirrored repeat;
- clamp-to-edge;
- point/linear filtering;
- mip filtering;
- anisotropy (vitaGL exposes it).

### LOD bias

Validate fractional LOD bias carefully. The current vitaGL sampler float entry routes through integer sampler handling, so do not assume desktop-equivalent fractional behavior.

Preferred fallback if needed: use the fragment-shader texture lookup bias overload for the specific active stages, after a Vita shader-compiler contract test.

### Border addressing/color

Current vitaGL does not expose the desktop `GL_CLAMP_TO_BORDER` + vector border-color contract used by the generic backend.

Bring-up fallback: map border addressing to clamp-to-edge only if instrumentation proves the current UI shader does not sample outside the edge or border color is irrelevant.

Faithful fallback when required: specialize the generated fragment shader for that stage, detect coordinates outside the valid range and substitute the Xbox border color. Add the border color uniform only for a shader that actually needs it because fragment uniform space is limited.

## 11. Render targets and FBOs

The Halo backend creates cached GL textures for Xbox render surfaces and FBOs for color/depth combinations.

### 11.1 Single color target

Halo's current backend uses one color attachment. vitaGL supports `GL_COLOR_ATTACHMENT0`; therefore the absence of `glDrawBuffers` is narrower than it first appears.

Vita path:

- do not call unavailable `glDrawBuffers`;
- attach color target 0 when present;
- attach depth/stencil as needed;
- verify framebuffer completeness on hardware.

### 11.2 Depth-only target

Test depth-only FBO behavior explicitly. If vitaGL requires a color target for a path Halo uses, attach a small/dummy compatible color target and disable color writes rather than returning fake success.

### 11.3 Depth/stencil persistence

vitaGL has a `STORE_DEPTH_STENCIL=1` compatibility build option that makes FBO depth/stencil surfaces load/store in memory at a performance cost.

Start correctness testing with this enabled if Halo switches render targets while expecting previous depth/stencil contents to survive. Once behavior is confirmed, benchmark whether it can be disabled for the common path.

### 11.4 Render-target mip composition

The desktop path can use `glCopyImageSubData`; the Android backend already contains an FBO + `glBlitFramebuffer` fallback. Use the same architectural fallback for Vita.

A Vita diagnostic framebuffer/blit/readback probe should pass before the first Halo render-to-texture dependency is trusted.

## 12. Raster state

Preserve state application immediately before each draw. Do not eagerly translate only state setters, because Halo's XDK inline setters and deferred state table are part of the original contract.

### Depth/stencil

vitaGL exposes the needed basic depth and stencil operations. Verify:

- Z enable/write/function;
- D16 versus D24S8 behavior;
- stencil function/reference/masks;
- stencil fail/zfail/pass operations;
- target switches.

### Blend

Normal source/destination alpha/color factors and ADD/SUBTRACT/REVERSE/MIN/MAX are represented by vitaGL/SceGxm.

`glBlendColor`/constant blend factors are not part of the direct path. Instrument actual `D3DRS_SRCBLEND`/`DESTBLEND` values from retail draws before building a complex emulation. If constant color/alpha is encountered, implement a targeted shader/state fallback and record the affected draw/shader.

### Culling/front face

Keep the existing D3D-to-GL winding logic plus Vita clip-space Y flip. Validate with real geometry; a wrong double flip can make all front faces disappear.

### Z bias / polygon offset

Retain the upstream slope + constant polygon offset translation. It matters for decals and coplanar surfaces.

### Wireframe/point fill

These are lower priority unless observed in gameplay or required by a real UI/debug path. Do not let debug-only polygon modes block Main Menu.

## 13. Geometry, buffers and draw calls

### 13.1 First bring-up should favor streaming correctness

The desktop renderer contains a sophisticated mirrored guest-memory path tied to page protection and `memory_watch.c`. Vita memory/page semantics are different and this optimization should not block the first real draw.

Recommended first real-draw path:

- stream the vertex range needed by each draw into a bounded Vita VBO;
- stream/rebase indices as required;
- orphan/rotate buffers on overflow;
- CPU-convert D3DCOLOR and NORMPACKED3 when necessary;
- log overflows and bytes streamed.

Only after stable rendering should static geometry mirroring/page-watch optimization be introduced.

### 13.2 Index/base-vertex path

If `glDrawElementsBaseVertex` behaves correctly on the installed vitaGL build, retain it. Otherwise reuse the existing GLES-style fallback: CPU-rebase the copied 16-bit indices and call ordinary `glDrawElements`.

### 13.3 Primitive conversion

Keep upstream conversion rules:

- quad lists -> CPU-generated triangle indices;
- quad strip -> triangle strip;
- polygon -> triangle fan;
- ordinary triangles/strips/fans/lines/points map directly where supported.

### 13.4 Buffer sizes

Current public Vita defaults are diagnostic (`2 MiB` vertex stream, `256 KiB` index stream, ring 1). Do not increase them blindly.

Implement overflow/orphan/rotation behavior first, then measure high-water marks from real UI and `a10` draws. Memory budget must include shader/compiler allocations, textures, render targets, game arena and audio.

## 14. Visibility queries / lens flares

Current vitaGL implements `GL_SAMPLES_PASSED` using GXM visibility tests and exposes query result retrieval. This is better than a permanent boolean fake.

Vita plan:

1. use ordinary query objects;
2. begin with `GL_SAMPLES_PASSED`;
3. end query;
4. use `GL_QUERY_RESULT_AVAILABLE` where useful;
5. obtain the count through `glGetQueryObjectuiv`;
6. preserve the upstream scaling from rendered pixels back to Halo game pixels.

Do not use the desktop persistent query-buffer path on Vita. Do not use a fragment atomic-counter shader unless a real vitaGL limitation forces it.

If query latency causes stalls, optimize later with delayed slot reads rather than changing game-visible semantics first.

## 15. Runtime shader compiler, diagnostics and caching

### Bring-up vitaGL configuration

Prefer a correctness/debug build while integrating the renderer:

- `HAVE_SHARK_LOG=1`;
- `LOG_ERRORS=1`;
- `DEBUG_GLSL_TRANSLATOR=1` when diagnosing compiler failures;
- `DEBUG_GLSL_PREPROCESSOR=1` when needed;
- `SAFE_UNIFORMS=1` unless the Vita code explicitly avoids the array-location assumption;
- `SAFE_DRAW=1` only while investigating draw glitches if necessary;
- consider `STORE_DEPTH_STENCIL=1` during FBO correctness validation.

Avoid speedhack flags during bring-up (`DRAW_SPEEDHACK`, buffer/index speedhacks, texture-upload speedhacks, sampler speedhack, depth/stencil hack, etc.). They can create crashes/glitches that look like Halo translation bugs.

### Shader failure logging

For every failed real Halo shader, save/log:

```text
stage: VS or PS
Halo/NV2A shader ID/hash
full generated GLSL
vitaGL/VitaShaRK compile log
program link log
vertex declaration
active D3D render states relevant to the pixel key
active texture modes/sampler types
uniform/varying counts after specialization
```

Never substitute a magenta/fake shader silently and mark the draw working. A debug fallback may be used to localize geometry, but status must remain blocked until the real translation works.

### Shader cache

vitaGL supports automatic shader caching and program binary APIs. Do not enable persistent caching as the first solution to compile hitches.

Order:

1. make generated shaders deterministic and correct;
2. cache shader/program objects in memory by existing NV2A keys;
3. measure compilation stalls;
4. then enable or implement disk/program-binary caching keyed by generated-source hash + renderer/vitaGL version.

Never load a stale binary across an incompatible generator/vitaGL change without versioning the cache key.

## 16. Presentation and vertical blank

The desktop backend emulates vertical blank with a host thread. Vita has real display/vblank primitives and vitaGL buffer swap.

The Vita implementation should:

- use vitaGL's real buffer swap/present path;
- maintain Halo's flip counter semantics;
- implement `BlockUntilVerticalBlank`/vertical-blank callbacks using Vita timing/display APIs, not a 60 Hz host sleep thread;
- avoid an unconditional extra wait if `vglSwapBuffers` already blocks in the configured mode;
- measure frame pacing on hardware.

Target stable 30 FPS first. Do not tie game simulation to an accidental 60 Hz presentation assumption.

## 17. Recommended implementation order

### Phase R0 — Preserve known-good diagnostic baseline

Do not regress:

- vitaGL initialization;
- runtime shader compiler;
- visible diagnostic triangle;
- current map/tag reading.

### Phase R1 — Connect the real backend without advanced optimizations

- link the real `d3d8_gl`/resource/texture path;
- add Vita wrappers only where required;
- explicit `glBindAttribLocation` before program link;
- stream vertices/indices instead of depending on desktop memory mirror;
- CPU D3DCOLOR conversion;
- CPU NORMPACKED3 unpack;
- no fake shader/state success.

Goal: `HALO DRAW REACHED`, with complete logs even if shader/program compilation fails.

### Phase R2 — Make the first UI draw render

- specialize VS constants to fit Vita;
- specialize PS uniforms;
- compact varyings;
- verify clip-space/winding/scissor;
- bind 2D/cube textures correctly;
- handle the exact blend/depth/stencil state used by the first UI draw.

Goal: first `HALO DRAW RENDERS`.

### Phase R3 — Complete Main Menu rendering

- all UI shader variants observed;
- font/bitmap formats;
- alpha test/kill;
- render-target paths used by UI;
- shader cache correctness;
- controller-driven menu.

Goal: stable, controllable real Main Menu.

### Phase R4 — Campaign renderer coverage

After entering `a10`, add only blockers actually observed:

- environment/model shader variants;
- cubemaps/reflections;
- decals and Z bias;
- particles/points;
- visibility queries/lens flares;
- render-to-texture/mip composition;
- volume-texture atlas fallback if sampled;
- additional formats/states.

### Phase R5 — Performance

Only after correctness:

- static geometry mirror/cache;
- shader binary/disk cache;
- VitaGL vertex-layout cache;
- stream ring sizing;
- texture eviction/memory budget tuning;
- remove compatibility flags one at a time with before/after hardware evidence.

## 18. Instrumentation required before broad fallback work

The first real Halo draws should record a compact trace containing:

```text
frame/draw index
primitive type and vertex/index counts
VS ID + instruction count
vertex declaration elements/formats/streams
used VS constant count and relative-address flag
pixel shader key/hash
combiner count
active texture modes and sampler types
texture format/dimensions/mips/cube/volume
render/depth target IDs and dimensions
blend/depth/stencil/cull states
viewport/scissor
shader compile/link outcome
GL error after state and draw
```

This trace decides which fallbacks are actually required for `ui.map` instead of implementing the entire theoretical Xbox API before the first menu frame.

## 19. Fallback hierarchy

When a desktop GL feature is absent, use this order:

1. **Direct vitaGL equivalent** with validated behavior.
2. **Existing upstream GLES/Android fallback** if its semantics match Vita.
3. **Small HALO_VITA wrapper** preserving the D3D contract.
4. **CPU conversion/emulation** for data layout features such as packed vertices or compressed/volume textures.
5. **Generated-shader emulation** for sampling/state features that cannot be expressed by vitaGL state.
6. **Direct SceGxm specialization** only if the above are proven insufficient and the architectural cost is justified.
7. A visible diagnostic substitute may be used only for debugging, never as the final Halo renderer path.

## 20. Things that must not be done

- Do not replace NV2A shaders with generic textured/unlit shaders and call the renderer working.
- Do not return fake-success shader handles.
- Do not ignore vertex shader constants, pixel shader programs, render targets or texture state.
- Do not silently skip unsupported texture stages.
- Do not silently flatten a 3D texture to one slice.
- Do not increase Vita memory pools until overflow/high-water evidence shows the need.
- Do not enable vitaGL speedhacks while diagnosing correctness.
- Do not assume a GL symbol's presence means desktop-equivalent semantics.
- Do not optimize static memory mirroring before a streamed real Halo draw works.

## 21. Immediate Codex checklist for renderer work

Before modifying renderer code, Codex should:

1. inspect the current local tree and the latest hardware logs;
2. read this file and `docs/GRAPHICS_COMPATIBILITY.md`;
3. identify the exact first real Halo draw/state path being reached;
4. inventory the actual shader/declaration/texture/state requirements of that draw;
5. verify the installed vitaGL API/behavior rather than guessing;
6. implement the narrowest semantically correct Vita adaptation;
7. add logging/contract tests for every newly trusted GPU behavior;
8. update `ATTEMPTS.md`, `STATUS.md`, `KNOWN_ISSUES.md` and `DECISIONS.md` as appropriate;
9. preserve the generated GLSL and compiler/link logs for failures;
10. distinguish `DIAGNOSTIC RENDERS`, `HALO DRAW REACHED` and `HALO DRAW RENDERS` in every result report.

## 22. Highest-priority technical risks

Current priority order:

1. explicit Vita attribute binding before program link;
2. vertex constant compaction (192 Xbox registers vs Vita uniform budget);
3. fragment uniform specialization;
4. varying compaction/interface matching;
5. NORMPACKED3 CPU conversion;
6. correct D3DCOLOR byte order;
7. streamed vertex/index path and overflow handling;
8. FBO/depth/stencil behavior and `glDrawBuffers` removal;
9. texture format/channel/mipmap validation;
10. exact sampler LOD/border behavior;
11. volume-texture atlas when actual runtime use is observed;
12. visibility-query integration;
13. presentation/vblank/frame pacing;
14. performance caches and memory optimization.

The Main Menu should drive the order inside this list: if tracing proves a later item is needed by the very first UI draw, promote it based on evidence.
