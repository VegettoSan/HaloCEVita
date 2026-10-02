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
/* The translated pixel path never consumes NV2A back-color outputs oB0/oB1.
 * Strip only those exact generated varyings before ShaccCg sees the source,
 * preserving the original register computation while keeping the linked Vita
 * interface inside its tight varying budget. */
void halo_vita_glShaderSource(GLuint shader, GLsizei count,
	const GLchar *const *strings, const GLint *lengths);
#define glShaderSource halo_vita_glShaderSource

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
