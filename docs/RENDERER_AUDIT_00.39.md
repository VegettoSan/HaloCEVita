# Original renderer audit — 00.39 / A118

Date: 2026-10-03. Imported reference: `21714ac0860e9b9ca08fbdc8a1d620f1b8a03797`;
current `cybersecurity/halo-ce-universal` inspected: `23b542601f2ca505c7a0143703e92fbda6075e18`;
native vitaGL dependency: `6e7fe40`. Source/host/native-build evidence is not console acceptance.

| Original owner | Current Vita boundary and audit result |
| --- | --- |
| `process_ui_widgets`, `event_handler_dispatch`, `widget_instance_process_one_event` | Complete original tables/traversal selected by `halo_ui_original.c`; recovery macro undefined; no replacement focus/menu logic |
| `widget_instance_render_recursive`, textbox/spinner rendering | Original authored coordinates, text, alpha and ordering; no layout/data corrections |
| `draw_bitmap_in_rect`, plasma/widget draw paths | Original scales, vertices, UVs, blending and bitmap selection; no concealment of white panels |
| screen-quad `Begin`/`End`, stream preparation | Original fan/vertex conversion; native API binding preserves topology/register data |
| `nv2a_vsh.c` translator | Original equations/constants and existing GLSL/native clip-space boundary; hardware-negative clip-position experiment not retried |
| `nv2a_psh.c` translator | Original stages/combiners/final equations; unsupported volume sampling explicit; no approximate shader |
| `glShaderSource` adaptation | Exact active prefix of original register arrays; unused declarations removed; hardened comment/index/capacity/native-limit checks preserve all register numbers and shader body |
| `program_get`, shader-pair cache, attribute binding, whole-array uniform updates | Original ownership with native reflected uniform handles; existing reflected-array behavior and invalid-handle rejection now covered in CI |
| `xgpu_texture_get`, `xgpu_texture_upload`, `dxt_decode_level` | Removed separate Vita DXT1 decoder; DXT1/3/5 now use existing upstream decoder and native BGRA uploader with original mip/face/alpha semantics |
| `bind_textures`, sampler setup | Stage selected before cold resource work; original raster state reapplied afterwards; existing cold/warm tests retained; capability gaps remain open |
| `bind_targets`, `apply_raster_state`, `prepare_draw` | Original color/depth target and state restored after resource work; existing framebuffer/state ordering tests retained |
| `SetIndices`, `DrawIndexedVertices` | Previously discarded `BaseVertexIndex`; now stored and applied to stream base plus minimum index exactly as current upstream; executable nonzero/large-base and empty-draw tests added |
| `main_menu_load`, original new-map owners | Shipping uses original menu/new-map initialization and native logical resource reader; corrected obsolete verifier requirements for unselected recovery owners; no replacement stubs |
| `main_pregame_render`, `render_frame_pregame`, `render_frame_present` | Original pregame frame construction/presentation retained; complete gameplay `main_loop` remains outside native harness |
| `D3DDevice_Present`, native swap | Original aspect-preserving composition and one swap; no additional blit or coordinate adjustment |

Original new-map source selection supersedes older staged-mount descriptions;
it does not establish successful scenario/BSP/world execution on console.

## Evidence

- `vita_shader_uniform_regression.py` executes real NV2A translators and the actual
  adapter: shader bodies remain byte-identical, register indices remain unchanged,
  nine combiner cases pass, invalid/dynamic/overbudget references fail explicitly.
- `vita_program_regression.py` and `vita_index_base_regression.py` execute actual
  C boundary implementations, including negative cases.
- Dumps now capture exact effective source after adaptation/unused-varying removal:
  `halo_vertex_native.glsl`, `halo_pixel_native.glsl` and first four `_native_00..03`
  files. Earlier non-native dumps capture translator input, not final compiler input.
- All26 required workflow host commands pass locally; additional shader-dump tests
  and retail-map transaction checks pass. Supplied `ui.map` remains unchanged,
  validating485 widgets/3543 pointers. Bloodgulch safely rejects Main Menu use.
- Exact CI SDK native00.39 ELF/SELF/VPK builds and passes `tools/vita_verify.py`.
  Build271/272 failed on obsolete recovery-symbol requirements. The gate now
  requires selected original map/UI/input/resource owners and excludes only
  unselected staged owners removed by section GC. Mandatory public `cache_file_read`
  disassembly still verifies a call to `vita_cache_resource_read`.
- Final CI source/run/version and VPK SHA-256 are recorded in BUILD.md after download.
  The local package is not the deliverable.

## Hardware status and remaining boundaries

No Vita is attached. White panels, text placement, transitions, FPS and stability
are unverified for00.39; none is marked fixed. Indexed-base correction is source
proven, not a proven cause of the reported UI symptoms. Volume textures,
nonzero LOD/unsupported anisotropy, border-color sampling, complete depth behavior
and gameplay-loop/world acceptance still require their own evidence.

Install exact00.39 CI VPK, retain original `ux0:data/HaloCE/maps/ui.map`, capture
cold menu and repeated D-pad entry/back in Campaign/Settings/profile windows,
including original name keyboard. Cross activates, Circle returns, Start confirms
keyboard; Select exits test app. Run at least60 seconds. Return fresh `debug.txt`,
`gamestate.txt` if created, all original/native GLSL files, cold/after-input photos
and any crash dump. Preserve matching CI ELF/map. Campaign entry tests the UI;
it does not promise playable campaign.
