# Graphics compatibility matrix

This is a living audit between Halo Universal's renderer requirements and vitaGL/Vita GPU capabilities.

Status values: `UNKNOWN`, `DIRECT`, `WRAPPER`, `FALLBACK`, `EMULATE`, `BLOCKED`, `NOT NEEDED`.

| Feature/API | Initial status | Notes |
|---|---|---|
| `glCreateShader` / `glShaderSource` / `glCompileShader` | DIRECT | vitaGL exposes runtime GLSL shader APIs; generated shader syntax still requires testing. |
| Program link/use/uniform APIs | DIRECT | Verify uniform arrays and translator edge cases. |
| Vertex/index buffers | DIRECT | Audit streaming strategy and Vita memory cost. |
| Vertex arrays/attributes | DIRECT | Validate integer attributes and packed formats used by Halo. |
| `glDrawElementsBaseVertex` | DIRECT | Exposed by vitaGL; behavior/performance must be tested. |
| 2D textures | DIRECT | Format conversion/swizzle/compression still needs audit. |
| Cubemap textures | UNKNOWN | Test actual generated shaders and upload formats. |
| 3D textures / `sampler3D` | BLOCKED | Known risk; determine whether campaign content actually requires them and whether vitaGL path is viable. |
| Framebuffers | DIRECT | vitaGL exposes framebuffer APIs. |
| `glBlitFramebuffer` | DIRECT | Useful for upstream Android-style copy fallback. |
| `glCopyImageSubData` | FALLBACK | Prefer framebuffer/blit fallback similar to upstream Android when unavailable. |
| Sampler objects | DIRECT | API exposed; validate exact parameter support. |
| Occlusion/visibility queries | UNKNOWN | Test query targets/results and Halo lens-flare expectations. |
| Atomic-counter visibility path | NOT NEEDED | Do not copy Android ES 3.1+ path automatically; choose a Vita-appropriate fallback. |
| Texture compression DXT/S3TC | UNKNOWN | Determine direct VitaGL format support vs CPU decode/upload. |
| Render-to-texture | UNKNOWN | Critical for effects; verify orientation, depth/stencil and target reuse. |
| GLSL precision qualifiers | UNKNOWN | Normalize generated shader source for vitaGL translator as required. |

## Procedure for each API

When resolving a row:

1. identify every upstream call site;
2. identify whether VitaGL implements it and with what restrictions;
3. make a minimal standalone test if behavior is uncertain;
4. record result in `docs/ATTEMPTS.md`;
5. update this table with evidence;
6. only then integrate into Halo.
