#include "vita_runtime.h"
#include "halo_vita_graphics.h"
#include <vitaGL.h>
#include <vitashark.h>
#include <psp2/io/stat.h>
#include <stdio.h>
#include <stdlib.h>

static GLuint probe_program, probe_vbo;
static const char *compiler_stage = "vitaGL";
static int compiler_available;
static int graphics_init_attempted, graphics_ready;
static int cache_checkpoint = -2;

int vita_graphics_initialize(void)
{
	SceIoStat info = {0};
	GLboolean resolution_fallback;
	GLint viewport[4] = {0};
	const GLubyte *version;
	uint32_t user, cdram, phycont;
	if (graphics_init_attempted) return graphics_ready;
	graphics_init_attempted = 1;
	compiler_available = sceIoGetstat("ur0:data/libshacccg.suprx", &info) >= 0 ||
		sceIoGetstat("ur0:data/external/libshacccg.suprx", &info) >= 0;
	vita_log("runtime compiler libshacccg %s (never packaged)", compiler_available ? "found" : "not found; shader probe will be skipped");
	/* Compile vertex then fragment as a pair, with real status before attach.
	 * POSTPONED reports success before compilation; on compiler failure its
	 * link path can query GXM parameters through a NULL program (A014). */
	vglSetSemanticBindingMode(VGL_MODE_SHADER_PAIR);
	/* Explicit initial budget: 16MiB RAM, 24MiB CDRAM, no phycont pool.
	 * No upstream Android stream/index rings are allocated by this probe. */
	/* vitaGL 6e7fe40 returns res_fallback, NOT success: GL_FALSE is the
	 * normal return at 960x544. Treating it as failure left its splash on
	 * screen while our input loop continued without submitting any frames.
	 * Source: vitaGL/source/vgl.c sets vgl_inited then returns res_fallback. */
	resolution_fallback = vglInitWithCustomSizes(HALO_VITA_LEGACY_SIZE, 960, 544,
		HALO_VITA_GL_RAM_SIZE, HALO_VITA_GL_CDRAM_SIZE, 0, 0, SCE_GXM_MULTISAMPLE_NONE);
	glGetIntegerv(GL_VIEWPORT, viewport);
	version = glGetString(GL_VERSION);
	vita_log("vglInitWithCustomSizes returned=%u (resolution fallback flag, not success); viewport=%d,%d,%d,%d",
		(unsigned)resolution_fallback, viewport[0], viewport[1], viewport[2], viewport[3]);
	if (!version || viewport[2] <= 0 || viewport[3] <= 0) {
		vita_log("vitaGL context probe failed: version=%s viewport=%dx%d",
			version ? (const char *)version : "NULL", viewport[2], viewport[3]);
		return 0;
	}
	vita_log("GL vendor=%s renderer=%s version=%s GLSL=%s", glGetString(GL_VENDOR),
		glGetString(GL_RENDERER), version, glGetString(GL_SHADING_LANGUAGE_VERSION));
	glViewport(0, 0, viewport[2], viewport[3]);
	vita_free_memory(&user, &cdram, &phycont);
	vita_log("free memory after vitaGL user=%u cdram=%u phycont=%u", user, cdram, phycont);
	graphics_ready = 1;
	return graphics_ready;
}
static void compiler_log(const char *message, shark_log_level level, int line)
{
	/* Capture diagnostics even when the installed vitaGL has HAVE_SHARK_LOG
	 * disabled. The runtime compiler was initialized by vitaGL; do not init
	 * or terminate a second instance or replace its allocators. */
	vita_log("NV2A compiler stage=%s level=%d line=%d: %s", compiler_stage,
		(int)level, line, message ? message : "(no message)");
}
static GLint shader_log(GLuint shader, const char *name)
{
	char log[4096]; GLsizei length = 0; GLint status = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	glGetShaderInfoLog(shader, sizeof(log), &length, log);
	log[length > 0 && length < (GLsizei)sizeof(log) ? length : 0] = 0;
	vita_log("NV2A %s compile status=%d GL error=0x%x log=%s", name, status, glGetError(), log);
	return status;
}
static void failed_shader_source(GLuint shader, const char *name)
{
	GLint size = 0; GLsizei length = 0; char *source; FILE *file;
	glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &size);
	if (size <= 1 || size > 256 * 1024) {
		vita_log("NV2A failed source %s unavailable/over budget: %d bytes", name, size);
		return;
	}
	source = malloc((size_t)size);
	if (!source) { vita_log("NV2A failed source %s allocation failed", name); return; }
	glGetShaderSource(shader, size, &length, source);
	/* vitaGL retains the translated Cg source when compilation fails. Its
	 * glGetShaderSource does not guarantee a trailing NUL; write by length. */
	file = fopen(name, "wb");
	if (file) {
		size_t written = length > 0 && length < size ? fwrite(source, 1, (size_t)length, file) : 0;
		fclose(file);
		vita_log("NV2A failed source %s saved=%u bytes", name, (unsigned)written);
	} else vita_log("NV2A failed source %s could not open", name);
	free(source);
}
int vita_graphics_shader_probe(const char *vertex, const char *fragment)
{
	GLuint vs, fs; GLint vertex_ok, fragment_ok, linked = 0; FILE *file; char log[4096]; GLsizei length = 0;
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
	if (!vs || !fs) {
		vita_log("NV2A shader allocation failed vertex=%u fragment=%u GL error=0x%x", vs, fs, glGetError());
		if (vs) glDeleteShader(vs);
		if (fs) glDeleteShader(fs);
		return 0;
	}
	glShaderSource(vs, 1, &vertex, NULL); glShaderSource(fs, 1, &fragment, NULL);
	shark_install_log_cb(compiler_log);
	compiler_stage = "vertex";
	vita_log("NV2A vertex compile begin (VGL_MODE_SHADER_PAIR)");
	glCompileShader(vs);
	vertex_ok = shader_log(vs, "vertex");
	if (!vertex_ok) failed_shader_source(vs, HALO_VITA_DATA_ROOT "vertex_probe.cg");
	/* Always finish the pair to reset vitaGL's varying-semantic pool, even
	 * if the vertex compiler rejected its source. Never attach failed code. */
	compiler_stage = "fragment";
	vita_log("NV2A fragment compile begin (VGL_MODE_SHADER_PAIR)");
	glCompileShader(fs);
	fragment_ok = shader_log(fs, "pixel");
	if (!fragment_ok) failed_shader_source(fs, HALO_VITA_DATA_ROOT "fragment_probe.cg");
	compiler_stage = "vitaGL";
	if (!vertex_ok || !fragment_ok) {
		vita_log("NV2A GLSL probe BLOCKED: compile vertex=%d fragment=%d; attach/link skipped; diagnostic continues",
			vertex_ok, fragment_ok);
		glDeleteShader(vs); glDeleteShader(fs);
		return 0;
	}
	probe_program = glCreateProgram();
	if (!probe_program) {
		vita_log("NV2A program allocation failed; diagnostic continues");
		glDeleteShader(vs); glDeleteShader(fs); return 0;
	}
	vita_log("NV2A attach compiled pair begin");
	glAttachShader(probe_program, vs); glAttachShader(probe_program, fs);
	glBindAttribLocation(probe_program, 0, "v0_in");
	glBindAttribLocation(probe_program, 3, "v3_in");
	vita_log("NV2A program link begin (both compile statuses passed)");
	glLinkProgram(probe_program);
	glGetProgramiv(probe_program, GL_LINK_STATUS, &linked);
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
	glColor4f(shader_valid > 0 ? 0.2f : 1, shader_valid > 0 ? 1 : 0.65f, 0.4f, 1);
	text(40, 424, shader_valid < 0 ? "GLSL TEST PENDING" : shader_valid ? "GLSL PASS" : "GLSL BLOCKED SEE LOG", 3);
	glColor4f(cache_checkpoint > 0 ? 0.2f : 1, cache_checkpoint > 0 ? 1 : 0.65f, 0.4f, 1);
	text(40, 466, cache_checkpoint == -2 ? "ENGINE MEMORY AND TAG TEST PENDING" :
		cache_checkpoint < 0 ? "READING REAL MAP TAGS" : cache_checkpoint ? "MENU TAGS PASS - ENGINE PENDING" : "TAG INDEX BLOCKED SEE LOG", 2);
	glColor4f(0.7f, 0.75f, 0.8f, 1);
	text(40, 496, "START EXIT   CROSS RECHECK MAPS", 2);
	vglSwapBuffers(GL_FALSE);
	if (!presented) { vita_log("[VITA 012] diagnostic frame submitted; GL error=0x%x (visibility requires console confirmation)", glGetError()); presented = 1; }
}
void vita_graphics_cache_status(int status) { cache_checkpoint = status; }
void vita_graphics_shutdown(void)
{
	if (probe_vbo) glDeleteBuffers(1, &probe_vbo);
	if (probe_program) glDeleteProgram(probe_program);
	/* Installed vitaGL exposes no global shutdown API. Process teardown
	 * releases GXM/pools after our own shader/buffer objects are deleted. */
}
