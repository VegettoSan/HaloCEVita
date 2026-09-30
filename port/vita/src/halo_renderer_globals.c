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

extern D3DDevice *global_d3d_device;

/* January primes renderstate_table from the real device in SetupSmartStates(),
 * then routes selected state transitions through SetRenderStateSmart(). The
 * owning object is absent from the reconstructed source set, so preserve that
 * original shadow contract here: suppress redundant D3D writes and update the
 * shadow exactly when a real write is issued. This remains a D3D8 state-cache
 * helper; the actual translation is still D3DDevice_SetRenderState -> vitaGL. */
HRESULT SetRenderStateSmart(D3DRENDERSTATETYPE state, DWORD value)
{
	unsigned long index = (unsigned long)state;
	if (index >= D3DRS_MAX)
		return E_INVALIDARG;
	if (renderstate_table[index] != value) {
		HRESULT result = IDirect3DDevice8_SetRenderState(global_d3d_device, state, value);
		if (SUCCEEDED(result))
			renderstate_table[index] = value;
		return result;
	}
	return S_OK;
}
