#!/usr/bin/env python3
"""Inventory upstream GL calls against the installed header AND archive."""
import hashlib
import os
from pathlib import Path
import re
import subprocess

sdk = Path(os.environ['VITASDK'])
header = sdk / 'arm-vita-eabi/include/vitaGL.h'
archive = sdk / 'arm-vita-eabi/lib/libvitaGL.a'
text = header.read_text()
symbols = subprocess.run([str(sdk / 'bin/arm-vita-eabi-nm'), '-g', '--defined-only', str(archive)],
                         capture_output=True, text=True, check=True).stdout
exports = set(re.findall(r'\b[TW]\s+(gl\w+)\b', symbols))
inputs = [Path('port/linux/src') / f for f in ('gl.h', 'd3d8_gl.c', 'nv2a_vsh.c', 'nv2a_psh.c', 'xbox_textures.c')]
required = set()
for path in inputs:
    source = path.read_text()
    required.update(re.findall(r'\bX\((gl[A-Z]\w+)\)', source))
    required.update(re.findall(r'\b(gl[A-Z]\w+)\s*\(', source))
special = {
 'glCopyImageSubData': ('FALLBACK', 'Framebuffer/blit level-copy probe in port/vita; full d3d8_gl integration pending.'),
 'glTexImage3D': ('BLOCKED', 'No installed 3D texture API. Campaign c10/c20 metadata contains volume bitmaps (A011); runtime use untested.'),
 'glCompressedTexImage3D': ('BLOCKED', 'No installed 3D upload API; no CPU replacement invented.'),
 'glClipControl': ('WRAPPER', 'HALO_VITA vertex generation flips Y/remaps Z; renderer call wrapper pending.'),
 'glBindFragDataLocation': ('WRAPPER', 'HALO_VITA pixel generation writes gl_FragColor; renderer binding wrapper pending.'),
 'glBufferStorage': ('FALLBACK', 'Use mutable BufferData/SubData streaming; not integrated in full renderer yet.'),
 'glVertexAttribIPointer': ('EMULATE', 'CPU NORMPACKED3 unpack required; translator logs/rejects packed input until implemented.'),
 'glVertexAttribI4ui': ('EMULATE', 'Integer attributes require conversion; no silent stub.'),
 'glDebugMessageCallback': ('NOT NEEDED', 'Use Vita logging and glGetError instead of desktop debug callback.'),
 'glGenSamplers': ('EMULATE', 'Separate sampler objects absent; apply cached parameters to textures (pending).'),
 'glBindSampler': ('EMULATE', 'Texture-state wrapper pending.'),
 'glSamplerParameteri': ('EMULATE', 'Texture-state wrapper pending.'),
 'glSamplerParameterf': ('EMULATE', 'Texture-state wrapper pending.'),
 'glSamplerParameterfv': ('EMULATE', 'Texture-state wrapper pending.'),
}
notes = {
 'glBindAttribLocation': 'A019 original d3d8_gl Vita binds16 NV2A inputs before link; COMPILES/CPU contract PASS, hardware pending.',
 'glGetUniformLocation': 'A019 Vita backend queries each c[i]; sparse/nonconsecutive CPU contract PASS. No192-vector/retail shader proof.',
 'glUniform4fv': 'A019 Vita vertex constants upload by explicit location/count1; COMPILES, full backend hardware pending.',
 'glMapBufferRange': 'Export exists; desktop persistent/coherent flags are not established. Requires Vita streaming path.',
 'glGenQueries': 'Export exists; supported target/count semantics need hardware testing. No Android atomics copied.',
 'glBeginQuery': 'Export exists; actual lens-flare sample-count fidelity remains unverified.',
 'glGetQueryObjectuiv': 'Export exists; no runtime evidence yet.',
 'glCompressedTexImage2D': 'Export exists; S3TC formats/decoding still need texture tests.',
 'glBlitFramebuffer': 'A014 real Vita: bounded known-color FBO/blit/readback PASS; full renderer copies/formats unverified.',
}
lines = ['# Graphics compatibility matrix', '',
 'Generated from actual installed artifacts by `VITASDK=/usr/local/vitasdk-hardfp python3 tools/vita_gl_audit.py`.', '',
 f'- Header: `{header}`, SHA-256 `{hashlib.sha256(header.read_bytes()).hexdigest()}`.',
 f'- Static library: `{archive}`, SHA-256 `{hashlib.sha256(archive.read_bytes()).hexdigest()}`.',
 '- Inputs: `gl.h`, `d3d8_gl.c`, `nv2a_vsh.c`, `nv2a_psh.c`, `xbox_textures.c` in `port/linux/src/`.',
 '- DIRECT means declaration and exported implementation exist, **not** runtime fidelity/performance proof. Rows cover the union of desktop and Android requirements.',
 '- A016: current 00.05 RENDERS diagnostic + synthetic NV2A triangle on real Vita; vertex1/pixel1/link1, visible draw, responsive input and Start exit. Full Halo renderer and repeated stability unverified.', '',
 '| API | Status | Header / exported | Evidence / next work |',
 '|---|---|---|---|']
counts = {}
for name in sorted(required):
    declared = bool(re.search(r'\b' + name + r'\s*\(', text))
    exported = name in exports
    if declared and exported:
        status, note = 'DIRECT', notes.get(name, 'Declaration and archive export verified; runtime semantics pending.')
    else:
        status, note = special.get(name, ('BLOCKED', 'Missing declaration or implementation; investigate exact upstream use.'))
    counts[status] = counts.get(status, 0) + 1
    lines.append(f'| `{name}` | {status} | {"yes" if declared else "no"} / {"yes" if exported else "no"} | {note} |')
lines += ['', '## Shader and memory contracts', '',
 'A013/A014: vitaGL revision 6e7fe40 vglInitWithCustomSizes returns a resolution-fallback flag (GL_FALSE at native resolution), not success. 00.03 confirms the corrected branch on hardware: viewport960x544, diagnostic frame visible, GL error0, known-color GPU-copy PASS. Full Halo graphics remain unintegrated.', '',
 'A014/A015: the 00.03 dump identifies a GXM data abort during deferred link. POSTPONED can query NULL programs after rejection. Since 00.04, VGL_MODE_SHADER_PAIR compiles vertex then fragment, validates statuses before attach/link, captures VitaShaRK diagnostics and saves failed Cg. The real 00.04 run proves vertex0/pixel1, recovery to011, responsive diagnostic/input and Start exit. This is not repeated stability or NV2A rendering evidence (KI-014/D008).', '',
 'A015/A016: the 00.04 vertex compiler rejects invariant gl_Position; at Cg line177. Current 00.05 omits that declaration only under HALO_VITA and logs the unverified multipass guarantee. Actual hardware now accepts both shaders and link, and the user sees the synthetic triangle. NV2A operations/arithmetic/varyings/pixel shader preserved; hardware source comparison confirms the one-line omission. Host114-case comparisons per backend preserve desktop/Android output. Broader retail shader coverage and multipass position agreement remain untested (KI-015/D009).', '',
 'HALO_VITA retains upstream NV2A instruction/combiner translation, changes only the GLSL dialect to 1.20 attributes/varyings, typed texture lookups and gl_FragColor, and removes the unused uint bit-shift helper. Integer-packed attributes and sampler3D log explicit blockers rather than generating unsupported shaders. Runtime dumps are vertex_probe.glsl and fragment_probe.glsl.', '',
 'The diagnostic VPK links the installed runtime GLSL translator and VitaShaRK (confirmed archive imports shark_compile_shader_extended and sceShaccCg functions). Console libshacccg is required for its generated shader probe; it is not packaged. Legacy diagnostic text/clear is attempted even without the compiler.', '',
 'Current probe: 64 MiB cap for newlib heap, vitaGL 16 MiB RAM + 24 MiB CDRAM and 2 MiB legacy pool; no phycont pool; one 96-byte static vertex buffer only after successful link. Failed source capture has a 256KiB temporary-buffer cap. A014 observed post-init free memory user165675008/CDRAM75497472/phycont27262976 bytes; these are one snapshot, not peak measurements or a full-game budget. No upstream stream/index rings are allocated. Android defaults total54MiB (3 x [16+2]MiB); desktop40MiB plus persistent mapping. Full Vita streaming sizes/flushes must be measured when integrating the renderer.', '',
 '3D texture modes project3d/dot_str_3d are recognized by the existing pixel translator and volume uploads occur in xbox_textures.c. A011 reads existing user-owned decompressed Xbox v5 caches in place: ui contains 5 volume bitmaps; campaign c10/c20 each contain 6 (distance attenuation, four default-3d bitmaps and Elite plasma-shield noise). Dimensions are 32x32x32 or 4x4x4. This confirms campaign content contains volume resources, not that each resource has been observed sampled at runtime. Compressed a10 was skipped; its usage is still unknown. Metadata-only commands/results are in ATTEMPTS.md; no map/pixel data is copied into the repository.', '']
Path('docs/GRAPHICS_COMPATIBILITY.md').write_text('\n'.join(lines))
print(f'{len(required)} APIs audited: {counts}')
