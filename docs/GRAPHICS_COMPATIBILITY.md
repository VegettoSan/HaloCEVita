# Graphics compatibility matrix

Generated from actual installed artifacts by `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_gl_audit.py`.

- Header: `/usr/local/vitasdk-hardfp/arm-vita-eabi/include/vitaGL.h`, SHA-256 `ba68004be9faf49fa4b0526ecbcdf26c3b4b8d001f85e119ce35db26a3f70e3a`.
- Static library: `/usr/local/vitasdk-hardfp/arm-vita-eabi/lib/libvitaGL.a`, SHA-256 `735f9be7dd02bbf36a75b0a5ad7a13bc51af82d3ac75434d6c375abe1c8be38e`.
- Inputs: `gl.h`, `d3d8_gl.c`, `nv2a_vsh.c`, `nv2a_psh.c`, `xbox_textures.c` in `port/linux/src/`.
- DIRECT means declaration and exported implementation exist, **not** runtime fidelity/performance proof. Rows cover the union of desktop and Android requirements.
- A016: current 00.05 RENDERS diagnostic + synthetic NV2A triangle on real Vita; vertex1/pixel1/link1, visible draw, responsive input and Start exit. Full Halo renderer and repeated stability unverified.

| API | Status | Header / exported | Evidence / next work |
|---|---|---|---|
| `glActiveTexture` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glAttachShader` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glBeginQuery` | DIRECT | yes / yes | Export exists; actual lens-flare sample-count fidelity remains unverified. |
| `glBindAttribLocation` | DIRECT | yes / yes | A019 original d3d8_gl Vita binds16 NV2A inputs before link; COMPILES/CPU contract PASS, hardware pending. |
| `glBindBuffer` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glBindBufferBase` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glBindBufferRange` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glBindFragDataLocation` | WRAPPER | no / no | HALO_VITA pixel generation writes gl_FragColor; renderer binding wrapper pending. |
| `glBindFramebuffer` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glBindSampler` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glBindTexture` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glBindVertexArray` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glBlendColor` | BLOCKED | no / no | Missing declaration or implementation; investigate exact upstream use. |
| `glBlendEquation` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glBlendFunc` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glBlitFramebuffer` | DIRECT | yes / yes | A014 real Vita: bounded known-color FBO/blit/readback PASS; full renderer copies/formats unverified. |
| `glBufferData` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glBufferStorage` | FALLBACK | no / no | Use mutable BufferData/SubData streaming; not integrated in full renderer yet. |
| `glBufferSubData` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glCheckFramebufferStatus` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glClear` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glClearColor` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glClearDepth` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glClearDepthf` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glClearStencil` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glClipControl` | WRAPPER | no / no | HALO_VITA vertex generation flips Y/remaps Z; renderer call wrapper pending. |
| `glColorMask` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glCompileShader` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glCompressedTexImage2D` | DIRECT | yes / yes | Export exists; S3TC formats/decoding still need texture tests. |
| `glCompressedTexImage3D` | BLOCKED | no / no | No installed 3D upload API; no CPU replacement invented. |
| `glCopyImageSubData` | FALLBACK | no / no | Framebuffer/blit level-copy probe in port/vita; full d3d8_gl integration pending. |
| `glCreateProgram` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glCreateShader` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glCullFace` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glDebugMessageCallback` | NOT NEEDED | no / no | Use Vita logging and glGetError instead of desktop debug callback. |
| `glDeleteBuffers` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glDeleteFramebuffers` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glDeleteShader` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glDeleteTextures` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glDepthFunc` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glDepthMask` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glDepthRange` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glDepthRangef` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glDisable` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glDisableVertexAttribArray` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glDrawArrays` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glDrawBuffers` | BLOCKED | no / no | Missing declaration or implementation; investigate exact upstream use. |
| `glDrawElements` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glDrawElementsBaseVertex` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glEnable` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glEnableVertexAttribArray` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glEndQuery` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glFinish` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glFlush` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glFramebufferTexture2D` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glFrontFace` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glGenBuffers` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glGenFramebuffers` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glGenQueries` | DIRECT | yes / yes | Export exists; supported target/count semantics need hardware testing. No Android atomics copied. |
| `glGenSamplers` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glGenTextures` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glGenVertexArrays` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glGenerateMipmap` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glGetError` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glGetIntegerv` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glGetProgramInfoLog` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glGetProgramiv` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glGetQueryObjectuiv` | DIRECT | yes / yes | Export exists; no runtime evidence yet. |
| `glGetShaderInfoLog` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glGetShaderiv` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glGetString` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glGetTexImage` | BLOCKED | no / no | Missing declaration or implementation; investigate exact upstream use. |
| `glGetUniformLocation` | DIRECT | yes / yes | A019 Vita backend queries each c[i]; sparse/nonconsecutive CPU contract PASS. No192-vector/retail shader proof. |
| `glInvalidateFramebuffer` | BLOCKED | no / no | Missing declaration or implementation; investigate exact upstream use. |
| `glLineWidth` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glLinkProgram` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glMapBufferRange` | DIRECT | yes / yes | Export exists; desktop persistent/coherent flags are not established. Requires Vita streaming path. |
| `glPixelStorei` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glPolygonMode` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glPolygonOffset` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glReadBuffer` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glReadPixels` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glSamplerParameterf` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glSamplerParameterfv` | EMULATE | no / no | Texture-state wrapper pending. |
| `glSamplerParameteri` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glScissor` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glShaderSource` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glStencilFunc` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glStencilMask` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glStencilOp` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glTexImage2D` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glTexImage3D` | BLOCKED | no / no | No installed 3D texture API. Campaign c10/c20 metadata contains volume bitmaps (A011); runtime use untested. |
| `glTexParameterf` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glTexParameterfv` | BLOCKED | no / no | Missing declaration or implementation; investigate exact upstream use. |
| `glTexParameteri` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glTexParameteriv` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glTexSubImage2D` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glUniform1f` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glUniform1i` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glUniform1iv` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glUniform2f` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glUniform4fv` | DIRECT | yes / yes | A019 Vita vertex constants upload by explicit location/count1; COMPILES, full backend hardware pending. |
| `glUseProgram` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glVertexAttrib4fv` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glVertexAttribI4ui` | EMULATE | no / no | Integer attributes require conversion; no silent stub. |
| `glVertexAttribIPointer` | EMULATE | no / no | CPU NORMPACKED3 unpack required; translator logs/rejects packed input until implemented. |
| `glVertexAttribPointer` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |
| `glViewport` | DIRECT | yes / yes | Declaration and archive export verified; runtime semantics pending. |

## Shader and memory contracts

A013/A014: vitaGL revision 6e7fe40 vglInitWithCustomSizes returns a resolution-fallback flag (GL_FALSE at native resolution), not success. 00.03 confirms the corrected branch on hardware: viewport960x544, diagnostic frame visible, GL error0, known-color GPU-copy PASS. Full Halo graphics remain unintegrated.

A014/A015: the 00.03 dump identifies a GXM data abort during deferred link. POSTPONED can query NULL programs after rejection. Since 00.04, VGL_MODE_SHADER_PAIR compiles vertex then fragment, validates statuses before attach/link, captures VitaShaRK diagnostics and saves failed Cg. The real 00.04 run proves vertex0/pixel1, recovery to011, responsive diagnostic/input and Start exit. This is not repeated stability or NV2A rendering evidence (KI-014/D008).

A015/A016: the 00.04 vertex compiler rejects invariant gl_Position; at Cg line177. Current 00.05 omits that declaration only under HALO_VITA and logs the unverified multipass guarantee. Actual hardware now accepts both shaders and link, and the user sees the synthetic triangle. NV2A operations/arithmetic/varyings/pixel shader preserved; hardware source comparison confirms the one-line omission. Host114-case comparisons per backend preserve desktop/Android output. Broader retail shader coverage and multipass position agreement remain untested (KI-015/D009).

HALO_VITA retains upstream NV2A instruction/combiner translation, changes only the GLSL dialect to 1.20 attributes/varyings, typed texture lookups and gl_FragColor, and removes the unused uint bit-shift helper. Integer-packed attributes and sampler3D log explicit blockers rather than generating unsupported shaders. Runtime dumps are vertex_probe.glsl and fragment_probe.glsl.

The diagnostic VPK links the installed runtime GLSL translator and VitaShaRK (confirmed archive imports shark_compile_shader_extended and sceShaccCg functions). Console libshacccg is required for its generated shader probe; it is not packaged. Legacy diagnostic text/clear is attempted even without the compiler.

Current probe: 64 MiB cap for newlib heap, vitaGL 16 MiB RAM + 24 MiB CDRAM and 2 MiB legacy pool; no phycont pool; one 96-byte static vertex buffer only after successful link. Failed source capture has a 256KiB temporary-buffer cap. A014 observed post-init free memory user165675008/CDRAM75497472/phycont27262976 bytes; these are one snapshot, not peak measurements or a full-game budget. No upstream stream/index rings are allocated. Android defaults total54MiB (3 x [16+2]MiB); desktop40MiB plus persistent mapping. Full Vita streaming sizes/flushes must be measured when integrating the renderer.

3D texture modes project3d/dot_str_3d are recognized by the existing pixel translator and volume uploads occur in xbox_textures.c. A011 reads existing user-owned decompressed Xbox v5 caches in place: ui contains 5 volume bitmaps; campaign c10/c20 each contain 6 (distance attenuation, four default-3d bitmaps and Elite plasma-shield noise). Dimensions are 32x32x32 or 4x4x4. This confirms campaign content contains volume resources, not that each resource has been observed sampled at runtime. Compressed a10 was skipped; its usage is still unknown. Metadata-only commands/results are in ATTEMPTS.md; no map/pixel data is copied into the repository.
