#!/usr/bin/env python3
"""Execute the actual prepare_draw resource/raster prefix against FBO mutations.

This models the native copy_level_by_blit contract: resource preparation leaves
framebuffer zero and invalidates the GL shadow. A Halo draw must still use its
requested color/depth target. It does not emulate Vita GPU output.
"""
from pathlib import Path
import resource
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/vita/tests/draw-target'
OUT.mkdir(parents=True, exist_ok=True)
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
renderer = (ROOT / 'port/linux/src/d3d8_gl.c').read_text()
start = renderer.index('static struct program_entry *prepare_draw(BOOL immediate)')
end = renderer.index('\tfor (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)', start)
body = renderer[start:end] + '''
    (void)entry; (void)stage; (void)immediate;
    return (struct program_entry *)program;
}
'''
prefix = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int BOOL;
typedef unsigned DWORD;
#define TRUE 1
#define FALSE 0
#define D3DTSS_MAXSTAGES 4
enum { D3DRS_PSCONSTANT0_0, D3DRS_PSFINALCOMBINERCONSTANT0 = 32,
    D3DRS_PSFINALCOMBINERCONSTANT1, D3DRS_PSTEXTUREMODES };
static DWORD D3D__RenderState[64];
struct vertex_shader_object { unsigned long id; void *instructions; };
struct program_entry { int unused; };
struct nv2a_pixel_shader_key { DWORD combiner_state[64]; DWORD texture_modes; };
struct draw_uniforms { float texture_scale[4][4]; };
static struct vertex_shader_object vertex = {4, &vertex};
static struct { int gl_ready; struct vertex_shader_object *vertex_shader; } device;
static struct { unsigned skipped_no_program, skipped_no_target; } stats;
static struct { const char *skip_vertex_shaders; } debug_settings;
static unsigned actual_fbo, shadow_fbo, requested_fbo, binds, raster_calls;
static int resource_copy, target_missing, missing_after_resources;
static struct vertex_shader_object *current_program(void) { return &vertex; }
static void vita_log(const char *format, ...) { (void)format; }
static BOOL bind_targets(BOOL *has_depth) {
    if (target_missing) return FALSE;
    if (shadow_fbo != requested_fbo) {
        actual_fbo = shadow_fbo = requested_fbo;
        ++binds;
    }
    *has_depth = requested_fbo == 7;
    return TRUE;
}
static void bind_textures(struct nv2a_pixel_shader_key *key, float scale[4][4]) {
    (void)key; (void)scale;
    if (resource_copy) {
        /* Actual copy_level_by_blit unbinds and invalidates all GL shadow. */
        actual_fbo = 0; shadow_fbo = ~0u;
    }
    if (missing_after_resources) target_missing = 1;
}
static void apply_raster_state(BOOL has_depth) {
    /* A viewport/blend correction cannot repair the wrong destination. */
    assert(actual_fbo == requested_fbo);
    assert(has_depth == (requested_fbo == 7));
    ++raster_calls;
}
'''
suffix = r'''
int main(void) {
    device.gl_ready = 1; device.vertex_shader = &vertex;
    /* Cold resource preparation on color-only and color/depth targets. */
    for (unsigned depth = 0; depth < 2; ++depth) {
        requested_fbo = depth ? 7 : 6;
        actual_fbo = 0; shadow_fbo = ~0u; binds = raster_calls = 0;
        resource_copy = 1;
        assert(prepare_draw(TRUE));
        assert(actual_fbo == requested_fbo && binds == 2 && raster_calls == 1);
        /* Warm hits keep the original destination without redundant GL binds. */
        resource_copy = 0;
        assert(prepare_draw(TRUE));
        assert(binds == 2 && raster_calls == 2);
    }
    target_missing = 1;
    assert(!prepare_draw(TRUE) && stats.skipped_no_target == 1);
    assert(raster_calls == 2);
    target_missing = 0; missing_after_resources = 1;
    assert(!prepare_draw(TRUE) && stats.skipped_no_target == 2);
    assert(raster_calls == 2);
    puts("PASS actual prepare_draw: cold FBO mutation, color/depth ownership, warm hits, missing target rejection");
}
'''

def compile_run(source):
    path = OUT / 'draw-target.c'
    executable = OUT / 'draw-target'
    path.write_text(source)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                    '-DHALO_VITA', str(path), '-o', str(executable)], check=True)
    return subprocess.run([str(executable)], capture_output=True, text=True)

result = compile_run(prefix + body + suffix)
assert result.returncode == 0, result.stderr
print(result.stdout.strip())
# Show the same stateful fixture catches the old resource-before-target defect.
restore = '''\tif (!bind_targets(&has_depth))
\t{
\t\tstats.skipped_no_target++;
\t\treturn NULL;
\t}
'''
position = body.index('copy_level_by_blit ends on framebuffer zero')
before, after = body[:position], body[position:]
assert restore in after
historical = before + after.replace(restore, '', 1)
assert compile_run(prefix + historical + suffix).returncode != 0
print('PASS historical draw into framebuffer zero reproduced')
