# Vita renderer behavior contracts

This note records renderer behavior that HaloCEVita must preserve while the
original Halo D3D8/NV2A path is adapted to vitaGL.

It is intentionally written as a set of **project contracts**, not as a second
renderer design.  The conclusions below were cross-checked against independent
hardware-valid behavior and then checked against HaloCEVita's own source,
runtime evidence, or original D3D8/NV2A semantics.  No foreign renderer source
code is imported by this note.

## Classification

- **FACT**: already present in HaloCEVita source or observed in our Vita logs /
  screenshots.
- **CORROBORATED CONTRACT**: behavior independently cross-checked and consistent
  with HaloCEVita/upstream semantics; safe to protect with regression tests.
- **HYPOTHESIS**: plausible explanation that still needs a Vita test.
- **PROPOSAL**: future change; do not mark working until measured on hardware.

## Immediate UI renderer contracts

### 1. Keep Halo UI author space independent from Vita output resolution

**CORROBORATED CONTRACT**

Halo's menu/widget geometry is authored in the original logical screen space.
The Vita output target may be lower resolution for performance, but changing
render resolution must not rewrite widget coordinates or tag data.

The shared GL backend already performs the D3D-to-GL viewport conversion with:

- positive X half-width scale;
- negative Y half-height scale;
- X/Y offsets at the viewport center.

Therefore a misplaced widget is not, by itself, evidence that the original
widget bounds should be scaled, flipped, or patched in `ui.map`.

**Rule:** keep author geometry untouched.  Fix viewport/scissor/target scaling
at the renderer boundary.

### 2. `D3DPT_TRIANGLEFAN` is a real UI primitive

**FACT + CORROBORATED CONTRACT**

Retail UI draws can arrive as four-vertex triangle fans.  Mapping them to a
different topology produces geometrically valid-looking but incorrect quads and
can masquerade as a texture/alpha bug.

HaloCEVita now maps `D3DPT_TRIANGLEFAN` to `GL_TRIANGLE_FAN`.

**Rule:** do not collapse fan draws to generic triangles.  The regression gate
`tools/vita_ui_render_contract_regression.py` protects this mapping.

### 3. Resource preparation happens before the final raster-state application

**FACT + CORROBORATED CONTRACT**

The Vita texture/resource bridge can bind or upload GL resources underneath the
D3D8 device state cache.  If the D3D8 raster/blend state is applied first and a
resource operation mutates GL state afterwards, the cached shadow can claim the
desired state is active while the real GPU state differs.

This explains a class of defects where:

- cold and warm draws differ;
- translucent UI becomes white/opaque;
- navigating to another widget appears to "repair" the frame;
- the same draw data produces different output depending on whether a texture
  had to be uploaded.

Current HaloCEVita resolves/binds textures first and re-applies the complete
raster contract afterwards on Vita.

**Rule:** the final sequence for a Vita draw must preserve:

`program/state derivation -> resource bind/upload -> full raster/blend restore -> draw`

Any lower layer that changes GL state outside `d3d8_gl.c` must invalidate the
shared GL-state shadow through the existing invalidation boundary.

### 4. Preserve original alpha semantics

**CORROBORATED CONTRACT**

DXT3 and DXT5 carry real alpha information.  UI correctness depends on the
original blend factors, texture alpha, vertex color/alpha and alpha-kill state
arriving together at the draw.

**Do not use as a permanent fix:**

- force decoded UI pixels to opaque;
- globally disable blending;
- hardcode one blend mode for all widgets;
- replace translucent backgrounds with custom Vita rectangles.

Those can make one screenshot look cleaner while breaking highlights, text,
gradients or later HUD/world effects.

### 5. Render scale and UI coordinate space are separate decisions

**CORROBORATED CONTRACT**

A reduced 3D/render target can be useful on Vita without redefining Halo's
logical UI coordinates.  Performance scaling belongs at the render-target /
presentation boundary.

**Rule:** a future 75%, 50% or other render-scale option must preserve:

- original viewport semantics;
- original UI/widget bounds;
- scissor rectangles transformed once;
- texture sampling/alpha unchanged.

This also means the current low-resolution bring-up target is not evidence that
tag-space UI coordinates should be rescaled.

## Texture contracts

### Compressed formats

**CORROBORATED CONTRACT**

The original cache uses DXT1, DXT3 and DXT5 paths that must retain their
format-specific alpha behavior.  Native compressed upload is optional; a
decoded BGRA fallback is acceptable when vitaGL/GXM sampling or mip behavior is
unreliable, provided the decoded result is semantically equivalent.

For UI bring-up, correctness is more important than avoiding decode cost.

### Mip chains

**HYPOTHESIS**

Some future world-rendering defects may come from compressed mip-chain handling
rather than base-level texture data.  This is not currently the best
explanation for the Main Menu white-overlay defect, because the UI already
shows correct bitmap content in several draws and the state-order defect was
directly identified.

Do not disable all mipmaps globally as a menu fix.

## Depth / render-target persistence

**CORROBORATED CONTRACT, NOT CURRENT MENU BLOCKER**

Halo assumes depth/stencil contents survive the render-target transitions where
D3D8 semantics require them.  This will matter for BSP/world rendering and
effects even if the Main Menu does not expose it strongly.

Treat depth persistence as a renderer contract, not an optimization.

## Shader strategy

**DECISION REINFORCED**

Continue the existing architecture:

`Halo D3D8/NV2A -> d3d8_gl + nv2a translators -> Vita-compatible GL -> vitaGL -> GXM`

Do not replace it with a parallel hand-authored menu renderer and do not switch
the project to a second graphics backend merely because another backend can
demonstrate equivalent behavior.

Independent renderer behavior is useful as an oracle for semantics; the
implementation in HaloCEVita remains our vitaGL path.

## Map/resource cache observation

**PROPOSAL — AFTER MENU CORRECTNESS**

Persisting a verified decompressed map cache can greatly reduce repeated startup
cost.  HaloCEVita currently favors safe regeneration because stale cache
identity is dangerous.

A future persistent-cache implementation must validate at least:

- source map identity, not filename alone;
- compressed/source size;
- expected logical/decompressed length;
- cache/header version or magic;
- bounds before exposing resource ranges.

Do not trade startup time for silent use of stale tag/resource data.

## Performance observations for later gameplay

**PROPOSAL — DEFER**

Once Campaign/gameplay is stable, Vita-specific quality controls may be useful:

- render resolution;
- model LOD distance;
- tiny distant-object rejection;
- less frequent static-scenery updates;
- less frequent object-lighting recomputation.

None of these belongs in the current menu-correctness patch.  First preserve the
original visual contract, then profile and reduce work with measurable toggles.

## Current diagnosis of the Main Menu visual defect

### Facts

- The original Main Menu root exists and real retail widgets are being drawn.
- Real bitmap resources are reaching the GPU path.
- Audio/menu state is alive.
- Navigation can change visible composition.
- The white/translucent corruption has been sensitive to draw/resource timing.
- A real ordering defect was found: resource preparation could invalidate GL
  state after the raster state had been considered applied.
- Current `main` restores the full raster state after resource preparation.
- Current `main` also processes the virtual-keyboard/menu input timing once per
  frame rather than repeatedly in one frame.

### Still unverified on hardware after the latest commits

- whether the white overlay is fully eliminated;
- whether every text layer keeps the correct painter order;
- whether all widgets remain anchored after repeated D-pad navigation;
- whether keyboard entry remains stable after many key transitions.

### Next Vita acceptance test

Use the current `main` build.  Verify the root menu first, then enter the
name/virtual-keyboard flow and navigate repeatedly.

Capture:

1. a photo of the first stable Main Menu frame;
2. a photo after several D-pad transitions;
3. a photo of the keyboard before and after repeated navigation;
4. `ux0:data/HaloCE/debug.txt`;
5. `gamestate.txt` and generated shader dumps if the build emits them.

Acceptance criteria:

- no full-screen or widget-sized white fallback rectangle;
- logo/buttons remain in stable positions;
- text does not disappear or switch to unrelated strings;
- one physical input transition produces one intended UI transition;
- no crash while the same widgets are revisited;
- the log reaches the original UI draw/present path without new GL errors.

## Regression protection

Run:

```sh
python3 tools/vita_ui_render_contract_regression.py
```

The test intentionally protects only stable, high-value contracts:

- triangle-fan topology;
- D3D viewport Y transform;
- resource-before-final-raster ordering on Vita;
- explicit GL state-shadow invalidation boundary.

It does **not** claim that the menu is visually fixed.  Only a Vita hardware
test can promote that state to `RENDERS` / `STABLE`.
