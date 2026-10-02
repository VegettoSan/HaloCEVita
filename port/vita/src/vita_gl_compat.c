/* GPU copy fallback derived from upstream Android copy_level_by_blit.
 * This narrow path is executable in the diagnostic VPK; full renderer
 * framebuffer cache integration remains a separate milestone. */
#include "vita_runtime.h"
#include <vitaGL.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

static uint32_t vita_dxt1_rgb565(uint16_t value)
{
	uint32_t r5 = (value >> 11) & 31;
	uint32_t g6 = (value >> 5) & 63;
	uint32_t b5 = value & 31;
	uint32_t r = (r5 << 3) | (r5 >> 2);
	uint32_t g = (g6 << 2) | (g6 >> 4);
	uint32_t b = (b5 << 3) | (b5 >> 2);
	return 0xFF000000u | (r << 16) | (g << 8) | b;
}

static uint32_t vita_dxt1_mix(uint32_t a, uint32_t b,
	unsigned wa, unsigned wb, unsigned divisor)
{
	uint32_t ar = (a >> 16) & 255, ag = (a >> 8) & 255, ab = a & 255;
	uint32_t br = (b >> 16) & 255, bg = (b >> 8) & 255, bb = b & 255;
	uint32_t r = (ar * wa + br * wb) / divisor;
	uint32_t g = (ag * wa + bg * wb) / divisor;
	uint32_t bl = (ab * wa + bb * wb) / divisor;
	return 0xFF000000u | (r << 16) | (g << 8) | bl;
}

/* The shared Xbox texture path already CPU-decodes DXT3/DXT5 on Vita because
 * their authored alpha is UI-critical. DXT1 used to be the last compressed
 * format passed directly through vitaGL. Keep the original DXT1 semantics,
 * including the c0<=c1 punch-through-alpha entry, but hand vitaGL ordinary
 * BGRA rows instead of assuming the Xbox block layout can be sampled as a
 * native Vita compressed texture. */
void halo_vita_glCompressedTexImage2D(GLenum target, GLint level,
	GLenum internal_format, GLsizei width, GLsizei height, GLint border,
	GLsizei image_size, const GLvoid *data)
{
	uint32_t *pixels;
	unsigned blocks_x, blocks_y, by, bx;
	size_t pixel_count, expected;
	static int first_dxt1_log;

	if (internal_format != GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) {
		/* DXT3/DXT5 should not reach this boundary: xbox_textures.c decodes
		 * them first. Preserve a checked escape hatch rather than silently
		 * interpreting another format as DXT1. */
		vita_log("[VITA DXT] unexpected compressed upload format=0x%x level=%d %dx%d bytes=%d",
			internal_format, level, width, height, image_size);
		vita_fatal("Unexpected compressed texture reached Vita upload boundary");
	}
	if (width <= 0 || height <= 0 || !data)
		vita_fatal("Invalid DXT1 texture upload on Vita");
	blocks_x = ((unsigned)width + 3u) / 4u;
	blocks_y = ((unsigned)height + 3u) / 4u;
	expected = (size_t)blocks_x * (size_t)blocks_y * 8u;
	if ((size_t)image_size < expected)
		vita_fatal("Truncated DXT1 texture upload on Vita");
	pixel_count = (size_t)width * (size_t)height;
	if (pixel_count > (16u * 1024u * 1024u) / sizeof(*pixels))
		vita_fatal("DXT1 decode exceeds Vita temporary texture bound");
	pixels = (uint32_t *)malloc(pixel_count * sizeof(*pixels));
	if (!pixels)
		vita_fatal("DXT1 decode allocation failed on Vita");

	for (by = 0; by < blocks_y; ++by) {
		for (bx = 0; bx < blocks_x; ++bx) {
			const uint8_t *block = (const uint8_t *)data +
				((size_t)by * blocks_x + bx) * 8u;
			uint16_t c0 = (uint16_t)(block[0] | ((uint16_t)block[1] << 8));
			uint16_t c1 = (uint16_t)(block[2] | ((uint16_t)block[3] << 8));
			uint32_t palette[4];
			uint32_t bits = (uint32_t)block[4] |
				((uint32_t)block[5] << 8) |
				((uint32_t)block[6] << 16) |
				((uint32_t)block[7] << 24);
			unsigned y, x;

			palette[0] = vita_dxt1_rgb565(c0);
			palette[1] = vita_dxt1_rgb565(c1);
			if (c0 > c1) {
				palette[2] = vita_dxt1_mix(palette[0], palette[1], 2, 1, 3);
				palette[3] = vita_dxt1_mix(palette[0], palette[1], 1, 2, 3);
			} else {
				palette[2] = vita_dxt1_mix(palette[0], palette[1], 1, 1, 2);
				palette[3] = 0;
			}
			for (y = 0; y < 4; ++y) {
				for (x = 0; x < 4; ++x) {
					unsigned px = bx * 4u + x, py = by * 4u + y;
					unsigned index = y * 4u + x;
					if (px < (unsigned)width && py < (unsigned)height)
						pixels[(size_t)py * (unsigned)width + px] =
							palette[(bits >> (index * 2u)) & 3u];
				}
			}
		}
	}

	if (!first_dxt1_log) {
		first_dxt1_log = 1;
		vita_log("[VITA DXT] DXT1 CPU decode -> BGRA enabled; compressed Xbox block layout no longer reaches vitaGL");
	}
	glTexImage2D(target, level, GL_RGBA8, width, height, border,
		GL_BGRA, GL_UNSIGNED_BYTE, pixels);
	free(pixels);
}

/* xbox_textures.c converts the Xbox formats (including the Vita-only DXT3/5
 * CPU fallback) to 32-bit BGRA.  vitaGL has a native BGRA internal format
 * backed by SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ARGB and can fast-store those
 * exact bytes.  Route only that already-converted upload here: this avoids a
 * second BGRA->RGBA CPU conversion inside vitaGL while preserving the same
 * logical RGBA channels, especially the authored byte-3 alpha used by Halo's
 * menu masks.  No alpha is synthesized, forced or premultiplied. */
void halo_vita_glTexImage2D(GLenum target, GLint level, GLint internal_format,
	GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type,
	const GLvoid *pixels)
{
	static int first_bgra_log;

	if (internal_format == GL_RGBA8 && format == GL_BGRA &&
		type == GL_UNSIGNED_BYTE)
	{
		internal_format = GL_BGRA;
		if (!first_bgra_log)
		{
			first_bgra_log = 1;
			vita_log("[VITA TEXTURE] decoded BGRA uses vitaGL direct ARGB storage; authored alpha byte preserved");
		}
	}
	glTexImage2D(target, level, internal_format, width, height, border,
		format, type, pixels);
}

/* The Vita NV2A vertex path emulates desktop GL_UPPER_LEFT by negating
 * gl_Position.y in the generated shader. Unlike glClipControl, that explicit
 * Y flip reverses triangle winding. Android already compensates for the same
 * shader-side flip inside d3d8_gl.c; Vita routes the backend's glFrontFace
 * calls here so the original Xbox/D3D render-state values stay untouched. */
void halo_vita_glFrontFace(GLenum mode)
{
	GLenum corrected = mode;
	static int first_log;
	if (mode == GL_CW) corrected = GL_CCW;
	else if (mode == GL_CCW) corrected = GL_CW;
	if (!first_log) {
		first_log = 1;
		vita_log("[VITA CULL] shader Y-flip winding compensation front=%x -> %x", mode, corrected);
	}
	glFrontFace(corrected);
}

static int copy_texture_level(GLuint source, GLuint destination, GLint level, GLsizei width, GLsizei height)
{
	GLuint framebuffers[2]; GLint previous_read, previous_draw;
	GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST); int complete;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
	glGenFramebuffers(2, framebuffers);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffers[0]);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, source, 0);
	complete = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffers[1]);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, destination, level);
	complete &= glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	if (complete) {
		glDisable(GL_SCISSOR_TEST);
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	} else vita_log("FBO copy BLOCKED: incomplete source/destination");
	glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
	if (scissor) glEnable(GL_SCISSOR_TEST);
	glDeleteFramebuffers(2, framebuffers);
	return complete;
}
int vita_graphics_copy_probe(void)
{
	static const unsigned char source[16] = {
		51,102,153,255, 51,102,153,255, 51,102,153,255, 51,102,153,255
	};
	GLuint textures[2], framebuffer; unsigned char result[4] = {0};
	int copied, correct; GLenum error;
	glGenTextures(2, textures);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, source);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	copied = copy_texture_level(textures[0], textures[1], 0, 2, 2);
	glGenFramebuffers(1, &framebuffer); glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[1], 0);
	if (copied && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
		glFinish(); glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, result);
	}
	error = glGetError();
	correct = copied && error == GL_NO_ERROR && result[0] == 51 && result[1] == 102 && result[2] == 153 && result[3] == 255;
	vita_log("GPU framebuffer/blit copy probe: %s RGBA=%u,%u,%u,%u error=0x%x",
		correct ? "PASS" : "FAIL", result[0], result[1], result[2], result[3], error);
	glBindFramebuffer(GL_FRAMEBUFFER, 0); glDeleteFramebuffers(1, &framebuffer);
	glBindTexture(GL_TEXTURE_2D, 0); glDeleteTextures(2, textures);
	return correct;
}
