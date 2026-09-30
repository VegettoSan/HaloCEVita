#ifndef HALO_VITA_GL_H
#define HALO_VITA_GL_H

/* The Linux renderer normally routes GL calls through SDL-resolved function
 * pointers in port/linux/src/gl.h. Vita owns a vitaGL context directly, so
 * renderer units compiled for HALO_VITA must call the exported vitaGL entry
 * points instead of SDL_GL_GetProcAddress indirection. Keep this boundary
 * small; unsupported desktop GL operations are adapted at their call sites. */
#include <vitaGL.h>

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

#endif
