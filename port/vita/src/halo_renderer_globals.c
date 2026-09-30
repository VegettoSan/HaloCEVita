/* Storage for original Xbox rasterizer globals owned by the executable's
 * data segment. The January source headers declare them but do not emit
 * definitions. Their values are populated by the original rasterizer code;
 * this file supplies storage only, in the isolated render closure probe. */
#include <xtl.h>
#include "rasterizer/xbox/rasterizer_xbox_pixel_shader.h"
#include "rasterizer/rasterizer_frame_statistics.h"
#include "rasterizer/rasterizer.h"

struct pixel_shader_definition pixel_shader;
struct rasterizer_frame_statistics_globals rasterizer_frame_statistics;
struct rasterizer_window_begin_parameters global_window_parameters;
real_argb_color ui_plasma_effect_color;
short local_player_index_for_draw_string_and_hack_in_icons;

/* In the retail Xbox executable these live in pooled COMMON storage rather
 * than in reconstructed source units. Keep their original declared types so
 * the decompiled rasterizer owns all values and state transitions. */
D3DCAPS8 global_d3d_caps;
unsigned long renderstate_table[D3DRS_MAX];
unsigned long texturestagestate_table[D3DTSS_MAXSTAGES][D3DTSS_MAX];
D3DBaseTexture *texture_table[D3DTSS_MAXSTAGES];
