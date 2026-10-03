#!/usr/bin/env python3
"""Execute the GL-boundary adapter against real NV2A translator output.

Proves fixed register identity and untouched arithmetic, resource budgets,
split/length-delimited sources, and rejection of unsafe indexing. No Vita GPU
or ShaccCg acceptance is inferred from host execution.
"""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
out = root / 'build/vita/tests/shader-uniforms'
out.mkdir(parents=True, exist_ok=True)
header = (root / 'port/vita/include/halo_vita_gl.h').read_text()
adapter = header[header.index('static inline unsigned halo_vita_shader_reference_span'):
                 header.index('#define glShaderSource')]
xgpu = (root / 'port/linux/src/xgpu.h').read_text()
types = xgpu[xgpu.index('struct xgpu_text'):xgpu.index('/* ---------- textures */')]
xdk = (root / 'port/include/xdk/xdk_pdb.h').read_text()
constants = '\n'.join(f'#define {name} {value}' for name, value in
                      re.findall(r'\b(D3D(?:RS|CMP|FOG)_[A-Z0-9_]+)\s*=\s*(0x[0-9A-Fa-f]+|[0-9]+)', xdk))

def original(name):
    return re.sub(r'^#include "(?:xgpu|port_config)\.h"\n', '',
                  (root / f'port/linux/src/{name}.c').read_text(), flags=re.M)

prefix = r'''
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef unsigned long DWORD;
typedef int BOOL, GLint, GLsizei;
typedef unsigned GLuint;
typedef char GLchar;
#define TRUE 1
#define FALSE 0
#define HALO_VITA 1
#define GL_MAX_VERTEX_UNIFORM_VECTORS 1
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS 2
static jmp_buf failure;
static int vertex_limit = 128, fragment_limit = 16;
static char *received;
static _Noreturn void vita_fatal(const char *s) { fprintf(stderr, "%s\n", s); longjmp(failure, 1); }
static void vita_log(const char *s, ...) { (void)s; }
static void platform_log(const char *s, ...) { (void)s; }
static const char *config_string(const char *s) { (void)s; return ""; }
static int config_boolean(const char *s) { (void)s; return 0; }
static void glGetIntegerv(unsigned key, GLint *value) {
    assert(key == 1 || key == 2); *value = key == 1 ? vertex_limit : fragment_limit;
}
static void halo_vita_glShaderSource(GLuint shader, GLsizei count,
    const GLchar *const *strings, const GLint *lengths) {
    assert(shader == 7 && count == 1 && !lengths);
    free(received); received = malloc(strlen(strings[0]) + 1); strcpy(received, strings[0]);
}
'''
suffix = r'''
static void submit(const char *source) { halo_vita_glShaderSourceBounded(7, 1, &source, NULL); }
static void reject(const char *source) {
    if (!setjmp(failure)) { submit(source); assert(!"unsafe shader accepted"); }
}
int main(void) {
    /* Actual translator source, including all original c[-38/-37] position
     * math and original register-combiner stages. Adapt only declarations. */
    const DWORD code[] = { 0, (1UL << 21) | 0x1B, 2UL << 26, (15UL << 12) | (1UL << 11),
                          0, (1UL << 21) | (3UL << 9) | 0x1B, 2UL << 26,
                          (15UL << 12) | (1UL << 11) | (3UL << 3) | 1 };
    char *vs = nv2a_vertex_shader_to_glsl(code, 2, 0);
    struct nv2a_pixel_shader_key key = {0};
    submit(vs);
    assert(strstr(received, "uniform vec4 c[ 60];"));
    assert(!strcmp(strstr(vs, "void main()"), strstr(received, "void main()")));
    /* Exact original indices survive (no compact-slot renumbering). */
    assert(strstr(received, "c[58]") && strstr(received, "c[59]"));
    free(vs);
    for (unsigned count = 0; count <= 8; count++) {
        key.combiner_state[D3DRS_PSCOMBINERCOUNT] = count;
        key.texture_modes = 0x421;
        for (unsigned i = 0; i < 4; i++) key.sampler_type[i] = _xgpu_sampler_2d;
        for (unsigned i = 0; i < count; i++) {
            key.combiner_state[D3DRS_PSRGBINPUTS0 + i] = 0x01020102;
            key.combiner_state[D3DRS_PSALPHAINPUTS0 + i] = 0x01020102;
        }
        char *ps = nv2a_pixel_shader_to_glsl(&key);
        submit(ps);
        assert(!strcmp(strstr(ps, "void main()"), strstr(received, "void main()")));
        assert(!strstr(received, "uniform vec4 bump_matrix"));
        assert(!strstr(received, "uniform vec4 bump_luminance"));
        assert(strstr(received, "uniform vec4 texture_scale[3];"));
        free(ps);
    }
    const char *split[] = { "uniform vec4 c[", "192];\nvoid main(){ vec4 v=c[59]; }TRAIL" };
    GLint lengths[] = { -1, (GLint)strlen(split[1]) - 5 };
    halo_vita_glShaderSourceBounded(7, 2, split, lengths);
    assert(strstr(received, "c[ 60]") && !strstr(received, "TRAIL"));
    submit("uniform vec4 c[192];\nvoid main(){ vec4 v=c[ 59 ]; }");
    assert(strstr(received, "c[ 60]"));
    reject("uniform vec4 c[192];\nvoid main(){ vec4 v=c[a0]; }");
    reject("uniform vec4 c[192];\nvoid main(){ vec4 v=c[clamp(a0,0,191)]; }");
    reject("uniform vec4 c[192];\nvoid main(){ vec4 v=c[192]; }");
    reject("uniform vec4 c[192];\nvoid main(){ vec4 v=c[99999999999999]; }");
    reject("uniform vec4 c[192]; uniform vec4 viewport_scale; uniform vec4 viewport_offset;\n"
           "void main(){ vec4 v=c[127] + viewport_scale + viewport_offset; }");
    vertex_limit = 0;
    reject("uniform vec4 c[192];\nvoid main(){ vec4 v=c[0]; }");
    vertex_limit = 128;
    const char *pixel = XGPU_PIXEL_UNIFORMS "void main(){ vec4 v=ps_c0[6]+ps_c1[0]+texture_scale[2]; }";
    submit(pixel);
    assert(strstr(received, "ps_c0[7]") && strstr(received, "ps_c1[1]"));
    fragment_limit = 10; reject(pixel); fragment_limit = 16;
    reject(XGPU_PIXEL_UNIFORMS "void main(){ vec4 v=ps_c0[8]; }");
    reject(XGPU_PIXEL_UNIFORMS "void main(){ vec4 v=texture_scale[stage]; }");
    submit("void main(){ gl_Position=vec4(0.0); }");
    assert(!strcmp(received, "void main(){ gl_Position=vec4(0.0); }"));
    free(received);
    puts("PASS actual NV2A -> Vita uniform adapter: unchanged arithmetic/indices, 9 combiner variants, split sources, bounds/budgets and dynamic-index rejection");
}
'''
source = out / 'contract.c'
source.write_text(prefix + constants + '\n' + types + '\n' +
                  original('xgpu_text') + original('nv2a_vsh') + original('nv2a_psh') +
                  adapter + suffix)
subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', str(source), '-o', str(out / 'contract')], check=True)
subprocess.run([str(out / 'contract')], check=True)
