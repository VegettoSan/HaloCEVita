#ifndef HALO_VITA_GL_H
#define HALO_VITA_GL_H

/* The Linux renderer normally routes GL calls through SDL-resolved function
 * pointers in port/linux/src/gl.h. Vita owns a vitaGL context directly, so
 * renderer units compiled for HALO_VITA must call the exported vitaGL entry
 * points instead of SDL_GL_GetProcAddress indirection. Keep this boundary
 * small; unsupported desktop GL operations are adapted at their call sites. */
#include <vitaGL.h>

/* Used as a target discriminator so the original renderer can reject an
 * unsupported volume bind before it reaches vitaGL. This does not add 3D
 * texture storage or sampler3D support. */
#ifndef GL_TEXTURE_3D
#define GL_TEXTURE_3D 0x806F
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

#ifdef HALO_VITA
#include "vita_runtime.h"
#include <stdlib.h>
#include <string.h>

/* The translated pixel path never consumes NV2A back-color outputs oB0/oB1.
 * Strip only those exact generated varyings before ShaccCg sees the source,
 * preserving the original register computation while keeping the linked Vita
 * interface inside its tight varying budget. */
void halo_vita_glShaderSource(GLuint shader, GLsizei count,
	const GLchar *const *strings, const GLint *lengths);

/* Keep the decomp's NV2A translators as the semantic owner.  Their generic GL
 * source deliberately declares the Xbox's complete register files (c[192],
 * ps_c0/ps_c1[8], texture/bump arrays[4]).  vitaGL advertises only 128 vertex
 * and 16 fragment uniform vectors, so carrying those generic declarations to
 * ShaccCg is outside the backend contract even when a particular Halo shader
 * references only a small prefix.
 *
 * This boundary adapter does not renumber a register or change an equation.
 * It only shortens each generated array to the highest numeric element that
 * the already-generated upstream source references.  Thus c[59] is still
 * c[59], ps_c0[6] is still ps_c0[6], and d3d8_gl.c uploads the same original
 * register values through the reflected shorter span. */
static inline unsigned halo_vita_shader_reference_span(const char *source,
	const char *declaration, const char *token)
{
	const char *decl = strstr(source, declaration);
	const char *scan;
	unsigned span = 0;

	if (!decl)
		return 0;
	scan = decl + strlen(declaration);
	while (*scan) {
		/* The upstream vertex epilogue documents c[-38]/c[-37] in a
		 * comment. Comments are not register reads or dynamic addressing. */
		if (scan[0] == '/' && scan[1] == '*') {
			const char *end = strstr(scan + 2, "*/");
			if (!end) vita_fatal("Unterminated generated GLSL comment");
			scan = end + 2;
			continue;
		}
		if (scan[0] == '/' && scan[1] == '/') {
			while (*scan && *scan != '\n') scan++;
			continue;
		}
		if (strncmp(scan, token, strlen(token))) {
			scan++;
			continue;
		}
		const char *p = scan + strlen(token);
		unsigned value = 0;
		int digits = 0;
		while (*p == ' ' || *p == '\t') p++;
		while (*p >= '0' && *p <= '9') {
			if (value > 192u)
				vita_fatal("Vita GLSL uniform register index exceeds Xbox capacity");
			value = value * 10u + (unsigned)(*p - '0');
			p++;
			digits = 1;
		}
		while (*p == ' ' || *p == '\t') p++;
		if (!digits || *p != ']')
			vita_fatal("Vita GLSL uniform adapter requires fixed original register indices");
		if (value + 1u > span)
			span = value + 1u;
		scan = p;
	}
	return span;
}

static inline int halo_vita_shader_reference_after(const char *source,
	const char *declaration, const char *token)
{
	const char *decl = strstr(source, declaration);
	return decl && strstr(decl + strlen(declaration), token) != NULL;
}

static inline void halo_vita_shader_patch_three_digit_span(char *source,
	const char *declaration, unsigned span)
{
	char *decl = strstr(source, declaration);
	char *digits;

	if (!decl || !span || span > 999u)
		vita_fatal("Vita vertex uniform span adapter contract failed");
	digits = strchr(decl, '[');
	if (!digits)
		vita_fatal("Vita vertex uniform declaration malformed");
	digits++;
	digits[0] = span >= 100u ? (char)('0' + (span / 100u) % 10u) : ' ';
	digits[1] = span >= 10u ? (char)('0' + (span / 10u) % 10u) : ' ';
	digits[2] = (char)('0' + span % 10u);
}

static inline void halo_vita_shader_patch_one_digit_span(char *source,
	const char *declaration, unsigned span)
{
	char *decl = strstr(source, declaration);
	char *digit;

	if (!decl || !span || span > 9u)
		vita_fatal("Vita fragment uniform span adapter contract failed");
	digit = strchr(decl, '[');
	if (!digit)
		vita_fatal("Vita fragment uniform declaration malformed");
	digit[1] = (char)('0' + span);
}

/* Unreferenced declarations carry no Halo value. Remove only the exact
 * generated declaration so dead arrays need no backend register allocation. */
static inline void halo_vita_shader_patch_array(char *source,
	const char *declaration, unsigned span, unsigned capacity)
{
	char *decl = strstr(source, declaration);
	if (!decl || span > capacity)
		vita_fatal("Vita GLSL array exceeds original register capacity");
	if (!span) {
		memset(decl, ' ', strlen(declaration));
		return;
	}
	if (capacity > 9u)
		halo_vita_shader_patch_three_digit_span(source, declaration, span);
	else
		halo_vita_shader_patch_one_digit_span(source, declaration, span);
}

static inline char *halo_vita_shader_join_source(GLsizei count,
	const GLchar *const *strings, const GLint *lengths)
{
	const size_t source_limit = 512u * 1024u;
	char *source;
	size_t total = 0, offset = 0;
	GLsizei index;

	if (count <= 0 || !strings)
		vita_fatal("Invalid Vita GLSL source array");
	for (index = 0; index < count; ++index) {
		size_t part;
		if (!strings[index])
			vita_fatal("NULL Vita GLSL source string");
		part = lengths && lengths[index] >= 0 ?
			(size_t)lengths[index] : strlen(strings[index]);
		if (part > source_limit || total > source_limit - part)
			vita_fatal("Vita GLSL source exceeds bounded uniform adapter");
		total += part;
	}
	source = (char *)malloc(total + 1u);
	if (!source)
		vita_fatal("Vita GLSL uniform adapter allocation failed");
	for (index = 0; index < count; ++index) {
		size_t part = lengths && lengths[index] >= 0 ?
			(size_t)lengths[index] : strlen(strings[index]);
		memcpy(source + offset, strings[index], part);
		offset += part;
	}
	source[total] = 0;
	return source;
}

static inline void halo_vita_glShaderSourceBounded(GLuint shader, GLsizei count,
	const GLchar *const *strings, const GLint *lengths)
{
	char *source = halo_vita_shader_join_source(count, strings, lengths);
	GLint vertex_limit = 0, fragment_limit = 0;
	static unsigned logged_vertex, logged_fragment;

	if (strstr(source, "uniform vec4 c[192];")) {
		unsigned span, active_vectors;
		/* Relative addressing can select any Xbox c[] entry and therefore
		 * cannot be prefix-compacted without additional shader metadata. Fail
		 * loudly rather than silently changing NV2A register semantics. */
		if (strstr(strstr(source, "uniform vec4 c[192];") + 23, "c[clamp(")) {
			free(source);
			vita_fatal("Vita NV2A vertex shader uses relative c[] addressing beyond the compact-register adapter");
		}
		span = halo_vita_shader_reference_span(source,
			"uniform vec4 c[192];", "c[");
		active_vectors = span;
		active_vectors += halo_vita_shader_reference_after(source,
			"uniform vec4 viewport_scale;", "viewport_scale") ? 1u : 0u;
		active_vectors += halo_vita_shader_reference_after(source,
			"uniform vec4 viewport_offset;", "viewport_offset") ? 1u : 0u;
		active_vectors += halo_vita_shader_reference_after(source,
			"uniform float point_size;", "point_size") ? 1u : 0u;
		active_vectors += halo_vita_shader_reference_after(source,
			"uniform float screen_offset;", "screen_offset") ? 1u : 0u;
		glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &vertex_limit);
		if (vertex_limit <= 0 || active_vectors > (unsigned)vertex_limit) {
			free(source);
			vita_fatal("Vita NV2A vertex constants exceed vitaGL hardware uniform budget");
		}
		halo_vita_shader_patch_array(source, "uniform vec4 c[192];", span, 192);
		if (logged_vertex < 8u) {
			logged_vertex++;
			vita_log("[VITA SHADER ABI] upstream vertex c[] prefix=%u/192 active_vec4<=%u vitaGL_limit=%d; register indices unchanged",
				span, active_vectors, vertex_limit);
		}
	}

	if (strstr(source, "uniform vec4 ps_c0[8];")) {
		unsigned c0 = halo_vita_shader_reference_span(source,
			"uniform vec4 ps_c0[8];", "ps_c0[");
		unsigned c1 = halo_vita_shader_reference_span(source,
			"uniform vec4 ps_c1[8];", "ps_c1[");
		unsigned bump = halo_vita_shader_reference_span(source,
			"uniform vec4 bump_matrix[4];", "bump_matrix[");
		unsigned luminance = halo_vita_shader_reference_span(source,
			"uniform vec4 bump_luminance[4];", "bump_luminance[");
		unsigned texture = halo_vita_shader_reference_span(source,
			"uniform vec4 texture_scale[4];", "texture_scale[");
		unsigned active_vectors = c0 + c1 + bump + luminance + texture;

		/* Count the remaining vec4/scalar uniforms conservatively as one
		 * vector each when referenced after their declaration. */
		active_vectors += halo_vita_shader_reference_after(source,
			"uniform vec4 ps_final_c0;", "ps_final_c0") ? 1u : 0u;
		active_vectors += halo_vita_shader_reference_after(source,
			"uniform vec4 ps_final_c1;", "ps_final_c1") ? 1u : 0u;
		active_vectors += halo_vita_shader_reference_after(source,
			"uniform vec4 fog_color;", "fog_color") ? 1u : 0u;
		active_vectors += halo_vita_shader_reference_after(source,
			"uniform vec4 fog_parameters;", "fog_parameters") ? 1u : 0u;
		active_vectors += halo_vita_shader_reference_after(source,
			"uniform float alpha_reference;", "alpha_reference") ? 1u : 0u;

		glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &fragment_limit);
		if (fragment_limit <= 0 || active_vectors > (unsigned)fragment_limit) {
			free(source);
			vita_fatal("Vita NV2A pixel shader exceeds vitaGL hardware uniform budget after exact prefix compaction");
		}

		halo_vita_shader_patch_array(source, "uniform vec4 ps_c0[8];", c0, 8);
		halo_vita_shader_patch_array(source, "uniform vec4 ps_c1[8];", c1, 8);
		halo_vita_shader_patch_array(source, "uniform vec4 bump_matrix[4];", bump, 4);
		halo_vita_shader_patch_array(source, "uniform vec4 bump_luminance[4];", luminance, 4);
		halo_vita_shader_patch_array(source, "uniform vec4 texture_scale[4];", texture, 4);

		if (logged_fragment < 8u) {
			logged_fragment++;
			vita_log("[VITA SHADER ABI] upstream pixel prefixes c0=%u c1=%u bump=%u lum=%u tex=%u active_vec4<=%u vitaGL_limit=%d",
				c0, c1, bump, luminance, texture, active_vectors, fragment_limit);
		}
	}

	{
		const GLchar *single = (const GLchar *)source;
		halo_vita_glShaderSource(shader, 1, &single, NULL);
	}
	free(source);
}
#define glShaderSource halo_vita_glShaderSourceBounded

/* Xbox cache DXT blocks and the Vita GPU's compressed texture layout do not
 * share a portable storage contract. DXT3/DXT5 are already decoded by the
 * shared texture path; route the remaining compressed call (DXT1) through a
 * checked CPU decode so vitaGL always receives ordinary BGRA texels. */
void halo_vita_glCompressedTexImage2D(GLenum target, GLint level,
	GLenum internal_format, GLsizei width, GLsizei height, GLint border,
	GLsizei image_size, const GLvoid *data);
#define glCompressedTexImage2D halo_vita_glCompressedTexImage2D
#endif

#endif
