/* New host boundary built against official vitaGL, not discarded port glue.
 * This component is compiled in isolation until the original renderer is
 * adapted. The engine owns draw order and presentation; no UI is drawn here.
 */
#include <limits.h>
#include <stddef.h>
#include <vitaGL.h>

#include "vita_gl_host.h"

static int initialized;

int vita_gl_initialize(unsigned long legacy_pool_size, unsigned long ram_threshold)
{
    if (initialized)
        return 1;
    if (legacy_pool_size > INT_MAX || ram_threshold > INT_MAX)
        return 0;
    initialized = vglInitExtended((int)legacy_pool_size, 960, 544,
                                 (int)ram_threshold, SCE_GXM_MULTISAMPLE_NONE) != GL_FALSE;
    return initialized;
}

void *vita_gl_get_proc(const char *name)
{
    if (!initialized || !name || !*name)
        return NULL;
    return vglGetProcAddress(name);
}

void vita_gl_present(void)
{
    if (initialized)
        vglSwapBuffers(GL_FALSE);
}
