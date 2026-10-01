/* CPU equivalent of upstream nv2a_vsh.c unpack_normpacked3. */
#ifndef HALO_VITA_VERTEX_H
#define HALO_VITA_VERTEX_H
#include <stdint.h>

static inline void halo_vita_unpack_normpacked3(uint32_t packed, float *normal)
{
    /* Unsigned extraction avoids implementation-defined signed shifts. Keep
     * the asymmetric minimum, exactly as the original shader does; no clamp. */
    int x = (int)(packed & 2047u);
    int y = (int)((packed >> 11) & 2047u);
    int z = (int)(packed >> 22);
    if (x & 1024) x -= 2048;
    if (y & 1024) y -= 2048;
    if (z & 512) z -= 1024;
    normal[0] = (float)x / 1023.0f;
    normal[1] = (float)y / 1023.0f;
    normal[2] = (float)z / 511.0f;
}
#endif
