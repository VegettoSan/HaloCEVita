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

/* Standard format/type selectors for the depth-stencil allocation probe.
 * Their availability as enum values is not proof vitaGL accepts the pair;
 * d3d8_gl.c checks the actual allocation error before using the target. */
#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL 0x84F9
#endif
#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8 0x84FA
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

#endif
