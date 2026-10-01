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
| `glFramebufferTexture2D` | DIRECT | yes / yes | vitaGL accepts color attachment only (A049); depth uses real renderbuffer requests in A050. Retail FBO GPU execution pending. |
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
| `glTexImage2D` | DIRECT | yes / yes | Packed GL_DEPTH_STENCIL input rejected0x501 on00.18 hardware (A049); use renderbuffer depth. Retail color formats still require actual upload/render. |
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

## A037 native D3D8 renderer probe (2026-09-30)

The separate `HALO_VITA_MENU_RENDER_PROBE` target now compiles the original Linux D3D8 adapter and Xbox texture decoder with installed vitaGL headers. The ordinary VPK remains on the root checkpoint and does not call these units. Vita uses the existing NV2A vertex clip conversion, direct visibility-query reads, transient vertex uploads (no guest page-write mirror), and checked FBO blits for mip copies. GL 3D textures, nonzero LOD bias/min level, constant blend factors, integer vertex attributes, depth-only FBO draw-buffer selection, and multi-level render-target mip composition currently fail explicitly if reached. Xbox texture uploads preserve original texture formats and decode path, validate temporary conversion bounds and log the first GL object/upload/error. Texture-cache GPU copies refresh from guest memory per use until Vita has explicit dirty tracking; performance is unmeasured. These are compile/link-probe facts, not runtime evidence of a visible retail bitmap. See A037 in ATTEMPTS.md.


## A038–A041 hardware/source reconciliation

00.16 now links and initializes the original rasterizer/device on Vita, including texture-cache map open; it fails on duplicate compiled-bitmap postprocess before any draw (A038). 00.17 keeps compiled bitmap offsets/flags intact and validates cold cache state/ranges, corrects the standalone pregame window index, and preserves vitaGL eager pair semantics across shader-cache hits/misses. Per-element pixel uniform locations remove the remaining assumed contiguous array upload in the Vita path. Real generated GLSL and rejected Cg are saved under ux0:data/HaloCE/halo_{vertex,pixel}.{glsl,cg}. A041 host contracts verify ordering/cache/immutability, not driver acceptance. Retail uniform budgets, unsupported volume/LOD/border states, FBO behavior and visible menu remain hardware gates.


A045/A046:00.17 hardware stops at039 before resource read/upload/shader/draw. Cached bitmap base/hardware words may be serialized Xbox values even with LRU index NONE.00.18 validates the cold cached contract without consuming those words; original loader owns replacement and registration. Actual loader/query host fixture passes; native/hardware texture upload remains unverified. No GPU capability change is inferred from this controlled exit.

## A049/A050 packed depth vs renderbuffer on Vita

00.18 hardware rejects packed glTexImage2D(GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8) with0x501 before first-window completion. Do not infer support from numeric tokens. Primary vitaGL6e7fe40 source/textures.c rejects this input; shared.h _glFramebufferTexture2D only accepts color attachment. Exposed source/vitaGL.h/framebuffers.c provide glGen/Bind/DeleteRenderbuffers, glRenderbufferStorage(GL_DEPTH24_STENCIL8) and glFramebufferRenderbuffer.00.19 uses those requests plus checked FBO color/depth attachments. Native archive resolution must be verified by CI; GPU acceptance remains pending.

vitaGL gxm.c lazily allocates the physical depth/stencil surface per FBO. A050 keeps one FBO per depth object across equal-size color switches and does not reattach depth on hits. Original Z/stencil masks/clears/tests are retained. glCheckFramebufferStatus chiefly checks color presence and cannot prove later GXM allocation. STORE_DEPTH_STENCIL load/store behavior, depth-only, mismatched-size pairs and depth sampling remain unverified/blocked. Actual-function host mocks pass namespace, scaling, cache identity and GL/error rejection; no retail draw/render claim. References: https://github.com/Rinnegatamante/vitaGL/blob/6e7fe40/source/framebuffers.c and source/{shared.h,textures.c,gxm.c,vitaGL.h} at the same revision.

A05100.19: glVertexAttribPointer in primary vitaGL6e7fe40/source/custom_shaders.c requires size1..4. Vita now shares Android's original CPU B/R byte swap and4 normalized UBYTE representation for D3DCOLOR, as prescribed by renderer plan6.5. Source bytes/declaration registers and non-color fields remain intact; actual-function Vita/Android/desktop host contracts PASS. This is a source correction, not a hardware-observed vertex failure. Original float/short representations stay intact. New Vita GL checks localize program/state/stream/draw/clear rejection; real shader/GPU acceptance remains a console gate.

## A054/A055 pixel-store upload boundary

00.19 hardware returns0x500 after the first real shield-noise bitmap read, with no draw/present. Primary vitaGL6e7fe40/source/textures.c glPixelStorei accepts only UNPACK_ROW_LENGTH; UNPACK_ALIGNMENT is declared but rejected.00.20 preserves original DXT blocks and tightly packed decoded32-bit BGRA, setting supported ROW_LENGTH0 under Vita. Desktop/Android retain alignment1. Check bind/incoming state, row setup and each mip separately; failure logs phase/level rather than attributing stale state errors to a compressed format. Actual-function host tests cover DXT1/3/5, observed DXT3 mip metadata and odd-width BGRA with mocked GPU/decode. Actual texture acceptance/sampling/render remains hardware pending.

## A056 low-resolution screen policy

Vita keeps original640x480 coordinates and scales screen targets to320x240 with the existing original backend scale mechanism; non-screen assets/targets stay intact. Native context/display remains960x544 for final original aspect-preserving upscale/letterbox.320x240 is85.3% fewer screen-target pixels vs previous960x544, not measured overall GPU cost. Actual screen/Present/FBO host bodies pass640/.5, odd-edge rounding, separate color/depth320x240 requests and centered4:3 blit; Android/desktop chooser behavior preserved. Hardware/native acceptance remains required.00.19 closure was unsupported pixel-store state, not observed GPU saturation.


## A058/A059 original draw and mapped stream contract

00.20 hardware now proves original320x240 allocation/clear, shield-noise upload, two compiled NV2A pairs and first original draw error0. No Present/visible menu. Exact workflow121 ELF/core identifies later font vertex upload in glBufferSubData copying into NULL after full-buffer clone allocation, not shader rejection/link crash.00.21 keeps2MiB vertex/256KiB index storage, finishes prior GPU reads, maps/writes/unmaps exact source bytes and wraps offsets without SubData clones/orphans. Same-draw aggregate reservations remain; alignment only advances offsets. Actual-function host lifetime/error/guard-page contracts PASS, native/hardware pending. This serializes draws and splits scenes; no FPS/memory benchmark or cross-scene depth persistence guarantee. Primary vitaGL6e7fe40/source/buffers.c and source/gxm.c define the exact lifetime/sync contract. Do not replace original fonts/shaders/menus or enable global driver speedhacks.

A060: native workflow122/sourcefb92bbf/VPK00.21 verifies real map/sync APIs and complete retained renderer/package closure; downloaded artifacts match release/verifier digests. The stream correction is LINKS, not hardware RENDERS/STABLE. Await font/UI/frame/Present/image; use the matching122 ELF/map for crash relocation.


## A061–A063 texture lifetime and installed uniform-array ABI

00.21 hardware completes original UI/font/frame/Present, still black, then later texture allocation aborts with corrupt malloc links. Original stream correction passes that full traversal. Primary vitaGL6e7fe40 asynchronous DXT transfer plus incremental mip realloc has an explicit lifetime hazard;00.22 waits native transfer queue after every original mip and before mutation, separately from finishing draw reads. Preserve every level/format/pixel and decoder staging. Dump identifies corrupted allocator, not sole writer.

Exact122 ELF glUniform4fv negates a whole-uniform handle and writes offset0 (NON-STRICT_UNIFORMS_COMPLIANCE); previous c[i]/pixel-element lookup contract was unsupported.00.22 reflects actual active vec4 arrays and uploads checked original prefixes from base locations. Supersedes A019/A041 element-handle assumptions for this archive; source math,192 CPU registers and original indices unchanged. Shader compaction/uniform budgets and visible output remain separate. Native reflection/transfer symbols and hardware compiled spans/repeated frames/image must pass before RENDERS/STABLE. Host actual helper/lifetime/bytes/failure tests PASS.


A065: Vita NORMPACKED3 now uses bounded CPU expansion into float3 streams with w1 supplied by ordinary GL attribute defaults, retaining original NV2A/register/11-11-10 semantics. Actual conversion/setup host tests and native ELF link PASS. Integer GL attribute requests remain unsupported; model/BSP activation and hardware rendering remain separate gates.
