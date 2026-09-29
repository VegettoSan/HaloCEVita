#include "vita_runtime.h"
#include "halo_vita_graphics.h"
#include <vitaGL.h>
#include <psp2/io/stat.h>
#include <stdio.h>
#include <stdlib.h>

static GLuint probe_program, probe_vbo;
static int compiler_available;

int vita_graphics_initialize(void)
{
	SceIoStat info = {0};
	compiler_available = sceIoGetstat("ur0:data/libshacccg.suprx", &info) >= 0 ||
		sceIoGetstat("ur0:data/external/libshacccg.suprx", &info) >= 0;
	vita_log("runtime compiler libshacccg %s (never packaged)", compiler_available ? "found" : "not found; shader probe will be skipped");
	vglSetSemanticBindingMode(VGL_MODE_POSTPONED);
	/* Explicit initial budget: 16MiB RAM, 24MiB CDRAM, no phycont pool.
	 * No upstream Android stream/index rings are allocated by this probe. */
	if (!vglInitWithCustomSizes(HALO_VITA_LEGACY_SIZE, 960, 544,
		HALO_VITA_GL_RAM_SIZE, HALO_VITA_GL_CDRAM_SIZE, 0, 0, SCE_GXM_MULTISAMPLE_NONE)) {
		vita_log("vglInitWithCustomSizes failed"); return 0;
	}
	vita_log("GL vendor=%s renderer=%s version=%s GLSL=%s", glGetString(GL_VENDOR),
		glGetString(GL_RENDERER), glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
	glViewport(0, 0, 960, 544);
	return 1;
}
static void shader_log(GLuint shader, const char *name)
{
	char log[4096]; GLsizei length = 0; GLint status = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	glGetShaderInfoLog(shader, sizeof(log), &length, log);
	log[length > 0 && length < (GLsizei)sizeof(log) ? length : 0] = 0;
	vita_log("NV2A %s compile status=%d log=%s", name, status, log);
}
int vita_graphics_shader_probe(const char *vertex, const char *fragment)
{
	GLuint vs, fs; GLint linked = 0; FILE *file; char log[4096]; GLsizei length = 0;
	static const float vertices[] = {
		480, 100, 0.5f, 1,  0.2f, 0.9f, 0.8f, 1,
		320, 300, 0.5f, 1,  0.2f, 0.5f, 1.0f, 1,
		640, 300, 0.5f, 1,  0.8f, 0.3f, 1.0f, 1
	};
	file = fopen(HALO_VITA_DATA_ROOT "vertex_probe.glsl", "wb");
	if (file) { fputs(vertex, file); fclose(file); }
	file = fopen(HALO_VITA_DATA_ROOT "fragment_probe.glsl", "wb");
	if (file) { fputs(fragment, file); fclose(file); }
	if (!compiler_available) { vita_log("NV2A GLSL probe BLOCKED: install runtime compiler"); return 0; }
	vs = glCreateShader(GL_VERTEX_SHADER); fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vs, 1, &vertex, NULL); glShaderSource(fs, 1, &fragment, NULL);
	glCompileShader(vs); glCompileShader(fs);
	probe_program = glCreateProgram();
	glAttachShader(probe_program, vs); glAttachShader(probe_program, fs);
	glBindAttribLocation(probe_program, 0, "v0_in");
	glBindAttribLocation(probe_program, 3, "v3_in");
	glLinkProgram(probe_program);
	glGetProgramiv(probe_program, GL_LINK_STATUS, &linked);
	shader_log(vs, "vertex"); shader_log(fs, "pixel");
	glGetProgramInfoLog(probe_program, sizeof(log), &length, log);
	log[length > 0 && length < (GLsizei)sizeof(log) ? length : 0] = 0;
	vita_log("NV2A program link=%d log=%s", linked, log);
	glDeleteShader(vs); glDeleteShader(fs);
	if (!linked) { glDeleteProgram(probe_program); probe_program = 0; return 0; }
	glGenBuffers(1, &probe_vbo); glBindBuffer(GL_ARRAY_BUFFER, probe_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return 1;
}
/* Original diagnostic glyphs; no retail UI, fonts or artwork in the VPK. */
static const unsigned char glyphs[26][7] = {
	{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
	{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
	{14,17,16,23,17,17,14},{17,17,17,31,17,17,17},{31,4,4,4,4,4,31},
	{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
	{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
	{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
	{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
	{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
	{17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
};
static void text(float x, float y, const char *string, float scale)
{
	int row, column;
	glBegin(GL_QUADS);
	while (*string) {
		if (*string >= 'A' && *string <= 'Z') {
			const unsigned char *glyph = glyphs[*string - 'A'];
			for (row = 0; row < 7; ++row) for (column = 0; column < 5; ++column) {
				if (glyph[row] & (1 << (4 - column))) {
					float px = x + column * scale, py = y + row * scale;
					glVertex2f(px, py); glVertex2f(px + scale, py);
					glVertex2f(px + scale, py + scale); glVertex2f(px, py + scale);
				}
			}
		}
		x += 6 * scale; ++string;
	}
	glEnd();
}
void vita_graphics_frame(int maps_valid, int core_valid, int shader_valid)
{
	static int presented;
	glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
	glClearColor(0.025f, 0.045f, 0.075f, 1); glClear(GL_COLOR_BUFFER_BIT);
	if (probe_program && shader_valid) {
		static const GLfloat scale[] = {480, 272, 1, 1};
		static const GLfloat offset[] = {480, 272, 0, 0};
		glUseProgram(probe_program);
		glUniform4fv(glGetUniformLocation(probe_program, "viewport_scale"), 1, scale);
		glUniform4fv(glGetUniformLocation(probe_program, "viewport_offset"), 1, offset);
		glBindBuffer(GL_ARRAY_BUFFER, probe_vbo);
		glEnableVertexAttribArray(0); glEnableVertexAttribArray(3);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(4 * sizeof(float)));
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(0); glDisableVertexAttribArray(3);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	glUseProgram(0); glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, 960, 544, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glColor4f(0.75f, 0.85f, 1, 1);
	text(40, 30, "HALO CE VITA", 4);
	text(40, 75, "NATIVE HALO CORE PROBE", 2);
	glColor4f(core_valid ? 0.2f : 1, core_valid ? 1 : 0.3f, 0.4f, 1);
	text(40, 340, core_valid ? "CORE PASS" : "CORE FAILED", 3);
	glColor4f(maps_valid ? 0.2f : 1, maps_valid ? 1 : 0.65f, 0.4f, 1);
	text(40, 382, maps_valid ? "MAPS VALID" : "MAPS MISSING OR INVALID", 3);
	glColor4f(shader_valid ? 0.2f : 1, shader_valid ? 1 : 0.65f, 0.4f, 1);
	text(40, 424, shader_valid ? "GLSL PASS" : "GLSL BLOCKED SEE LOG", 3);
	glColor4f(0.7f, 0.75f, 0.8f, 1);
	text(40, 496, "START EXIT   CROSS RECHECK MAPS", 2);
	vglSwapBuffers(GL_FALSE);
	if (!presented) { vita_log("[VITA 012] diagnostic frame submitted; GL error=0x%x (visibility requires console confirmation)", glGetError()); presented = 1; }
}
void vita_graphics_shutdown(void)
{
	if (probe_vbo) glDeleteBuffers(1, &probe_vbo);
	if (probe_program) glDeleteProgram(probe_program);
	/* Installed vitaGL exposes no global shutdown API. Process teardown
	 * releases GXM/pools after our own shader/buffer objects are deleted. */
}
