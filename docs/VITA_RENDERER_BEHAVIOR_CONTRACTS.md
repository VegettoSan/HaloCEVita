# Vita renderer behavior contracts

This note records renderer behavior that HaloCEVita must preserve while the
original Halo D3D8/NV2A path is adapted to vitaGL.

It is intentionally written as a set of **project contracts**, not as a second
renderer design. The conclusions below were cross-checked against independent
hardware-valid behavior and then checked against HaloCEVita's own source,
runtime evidence, or original D3D8/NV2A semantics. No foreign renderer source
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

Halo's menu/widget geometry is authored in the original 640x480 screen space.
Changing output resolution must not rewrite widget coordinates or tag data.
During the current UI-correctness phase HaloCEVita also renders the game target
at 640x480 one-to-one. The final presentation to the 960x544 Vita panel remains
a separate letterboxed presentation step.

The shared GL backend already performs the D3D-to-GL viewport conversion with:

- positive X half-width scale;
- negative Y half-height scale;
- X/Y offsets at the viewport center.

Therefore a misplaced widget is not, by itself, evidence that the original
widget bounds should be scaled, flipped, or patched in `ui.map`.

**Rule:** keep author geometry untouched. Fix viewport/scissor/target scaling at
the renderer boundary. While the Main Menu is being stabilized, keep the game
render target at the original 640x480 so half-resolution scaling cannot hide a
renderer defect.

### 2. `D3DPT_TRIANGLEFAN` is a real UI primitive

**FACT + CORROBORATED CONTRACT**

Retail UI draws can arrive as four-vertex triangle fans. Mapping them to a
different topology produces geometrically valid-looking but incorrect quads and
can masquerade as a texture/alpha bug.

HaloCEVita maps `D3DPT_TRIANGLEFAN` to `GL_TRIANGLE_FAN`.

**Rule:** do not collapse fan draws to generic triangles. The regression gate
`tools/vita_ui_render_contract_regression.py` protects this mapping.

### 3. Resource preparation happens before the final raster-state application

**FACT + CORROBORATED CONTRACT**

The Vita texture/resource bridge can bind or upload GL resources underneath the
D3D8 device state cache. If the D3D8 raster/blend state is applied first and a
resource operation mutates GL state afterwards, the cached shadow can claim the
desired state is active while the real GPU state differs.

This explains a class of defects where:

- cold and warm draws differ;
- translucent UI becomes white/opaque;
- navigating to another widget appears to repair the frame;
- the same draw data produces different output depending on whether a texture
  had to be uploaded.

Current HaloCEVita resolves/binds textures first and re-applies the complete
raster contract afterwards on Vita.

**Rule:** the final sequence for a Vita draw must preserve:

`program/state derivation -> resource bind/upload -> full raster/blend restore -> draw`

Any lower layer that changes GL state outside `d3d8_gl.c` must invalidate the
shared GL-state shadow through the existing invalidation boundary.

### 4. A texture upload belongs to the texture stage that requested it

**FACT + CORROBORATED CONTRACT**

`bind_textures()` walks the four Xbox texture stages. A cold cache miss can
create/upload a GL texture while this walk is in progress, and that upload uses
the currently active GL texture unit. A cached `state_texture()` decision is
not sufficient to prove the hardware unit is currently selected.

On Vita, HaloCEVita therefore selects `GL_TEXTURE0 + stage` **before** the
resource lookup/upload or mip composition for that stage. Only then does it
restore the stage's final texture/sampler binding.

Without this ordering, a first-time upload for a later stage can overwrite an
earlier stage's binding. That produces the dangerous cold/warm pattern where a
widget is wrong initially but looks different after navigation causes resources
to become resident.

**Rule:** do not move the Vita `glActiveTexture()` selection after
`xgpu_texture_get()`/mip preparation, and do not remove it because the GL shadow
appears to contain the same stage already.

### 5. Xbox blend-factor values are already GL/NV2A-compatible

**FACT**

The clean-room XDK/PDB contract in `xdk_pdb.h` records the blend factors as the
NV2A/OpenGL enumerant values: for example source alpha is `0x302` and inverse
source alpha is `0x303`. The shared renderer therefore passes the verified
values directly to `glBlendFunc()`.

**Rule:** do not add a second D3D-to-GL blend-factor translation table unless
the underlying XDK contract itself changes. A second conversion would alter
valid alpha behavior rather than fix it.

### 6. Preserve original alpha semantics

**CORROBORATED CONTRACT**

DXT1 punch-through alpha and DXT3/DXT5 explicit/interpolated alpha are authored
information. `AL8`/AY8-style one-byte data also uses the sample as both
luminance and alpha in the existing Xbox texture conversion path. UI correctness
depends on the original blend factors, texture alpha, vertex color/alpha and
alpha-kill state arriving together at the draw.

**Do not use as a permanent fix:**

- force decoded UI pixels to opaque;
- globally disable blending;
- hardcode one blend mode for all widgets;
- replace translucent backgrounds with custom Vita rectangles.

Those can make one screenshot look cleaner while breaking highlights, text,
gradients or later HUD/world effects.

### 7. Render scale and UI coordinate space are separate decisions

**CORROBORATED CONTRACT**

HaloCEVita currently keeps both logical space and the game target at 640x480 to
remove render-scale ambiguity from Main Menu bring-up. A reduced future 3D
render target may still be useful on Vita, but performance scaling belongs at
the render-target/presentation boundary and must not redefine Halo's UI space.

**Rule:** a future 75%, 50% or other render-scale option must preserve:

- original viewport semantics;
- original UI/widget bounds;
- scissor rectangles transformed once;
- texture sampling/alpha unchanged.

## Texture contracts

### Compressed formats and the Vita boundary

**FACT + CORROBORATED CONTRACT**

The original cache uses DXT1, DXT3 and DXT5. Their decoded texels are portable;
the byte/block layout expected by a specific GPU compressed-texture descriptor
is not a portable contract. During UI bring-up HaloCEVita therefore avoids
letting Xbox compressed blocks cross the vitaGL/GXM boundary unchanged:

- DXT3 and DXT5 continue through the existing CPU decoder in
  `xbox_textures.c`;
- the remaining DXT1 `glCompressedTexImage2D` call is intercepted at the Vita
  GL boundary, decoded with the original DXT1 565/punch-through semantics, and
  uploaded as ordinary BGRA rows.

This is a renderer-boundary adaptation, not an asset rewrite. `ui.map` bytes,
original texture headers, mip offsets, blend state and shaders stay unchanged.

**Rule:** do not re-enable direct Xbox DXT block upload on Vita merely because
vitaGL exposes S3TC entry points. Native compressed upload is acceptable later
only after its exact block-layout/mip contract is proven equivalent on hardware.

For UI bring-up, correctness is more important than avoiding decode cost.

### Mip chains

**HYPOTHESIS**

Some future world-rendering defects may come from compressed mip-chain handling
rather than base-level texture data. This is not currently the best sole
explanation for the Main Menu corruption because the same UI can change between
cold and warm resource states.

Do not disable all mipmaps globally as a menu fix.

## Depth / render-target persistence

**CORROBORATED CONTRACT, NOT CURRENT MENU BLOCKER**

Halo assumes depth/stencil contents survive the render-target transitions where
D3D8 semantics require them. This will matter for BSP/world rendering and
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

Persisting a verified decompressed map cache can reduce repeated startup cost.
HaloCEVita currently favors safe regeneration because stale cache identity is
dangerous.

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

None of these belongs in the current menu-correctness patch. First preserve the
original visual contract, then profile and reduce work with measurable toggles.

## Current diagnosis of the Main Menu visual defect

### Facts

- The original Main Menu root exists and real retail widgets are being drawn.
- Real bitmap resources are reaching the GPU path.
- Audio/menu state is alive and authored navigation/profile windows execute.
- The supplied hardware photos show the same screens changing substantially
  after interaction: cold frames can be pale/white or contain large gradient
  rectangles, while later frames become much closer to the authored UI.
- The supplied runtime log shows a 320x240 game target in that tested package,
  correct DXT3/DXT5 CPU-alpha decode, and one remaining native compressed DXT1
  upload.
- A real ordering defect was previously found: resource preparation could
  invalidate GL state after the raster state had been considered applied.
- Current `main` restores the full raster state after resource preparation.
- Current `main` selects each Vita GL texture unit before a cold upload can
  modify that stage's binding.
- Current `main` now renders the game target at 640x480 and decodes the
  remaining DXT1 boundary to BGRA before vitaGL.

### Still unverified on hardware after the latest commits

- whether the large white/gradient rectangles are eliminated;
- whether every text layer keeps the correct painter order;
- whether all widgets remain anchored after repeated D-pad navigation;
- the performance cost of 640x480 plus the correctness-first DXT decode path.

### Next Vita acceptance test

Use the current `main` build. Verify the root menu first, then enter the
name/virtual-keyboard and profile/settings flows and navigate repeatedly.

Capture:

1. a photo of the first Main Menu frame;
2. a photo after several D-pad transitions;
3. Enter Name before and after repeated navigation;
4. profile/settings screens that previously showed the right-side gradient;
5. `ux0:data/HaloCE/debug.txt`;
6. `gamestate.txt` and generated shader dumps if emitted.

Acceptance criteria:

- log reports a 640x480 game/render target;
- log reports the Vita DXT1 CPU-decode boundary when DXT1 is first used;
- no full-screen or widget-sized white/gradient fallback rectangle;
- logo/buttons remain in stable positions;
- text does not disappear or switch to unrelated strings;
- one physical input transition produces one intended UI transition;
- no crash while the same widgets are revisited;
- no new GL upload/draw errors.

## Regression protection

Run:

```sh
python3 tools/vita_ui_render_contract_regression.py
```

The test intentionally protects stable, high-value contracts:

- 640x480 game/render target during UI bring-up;
- triangle-fan topology;
- D3D viewport Y transform;
- Xbox/NV2A-to-GL blend-factor compatibility;
- per-stage GL texture-unit selection before Vita uploads;
- DXT1-to-BGRA compressed-layout boundary;
- resource-before-final-raster ordering on Vita;
- explicit GL state-shadow invalidation boundary.

The Vita CI workflow runs this regression together with the existing texture,
menu, input, audio, cache and renderer contract gates.

It does **not** claim that the menu is visually fixed. Only a Vita hardware
test can promote that state to `RENDERS` / `STABLE`.
