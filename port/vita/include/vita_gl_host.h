/* Native SDK ABI boundary for the new vitaGL host; no SDK enums cross it. */
#ifndef HALO_VITA_GL_HOST_H
#define HALO_VITA_GL_HOST_H

/* 1 on context creation, 0 on failure. Call on the render thread. */
int vita_gl_initialize(unsigned long legacy_pool_size, unsigned long ram_threshold);
/* NULL means unsupported; callers must fail or implement the actual operation. */
void *vita_gl_get_proc(const char *name);
void vita_gl_present(void);

#endif
