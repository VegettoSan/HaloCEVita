/* Storage for original Xbox rasterizer globals owned by the executable's
 * data segment. The January source headers declare them but do not emit
 * definitions. Their values are populated by the original rasterizer code;
 * this file supplies storage only, in the isolated render closure probe. */
#include "rasterizer/xbox/rasterizer_xbox_pixel_shader.h"
#include "rasterizer/rasterizer_frame_statistics.h"

struct pixel_shader_definition pixel_shader;
struct rasterizer_frame_statistics_globals rasterizer_frame_statistics;
