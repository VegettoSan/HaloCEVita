# Native full-UI comparison build

The `Vita Full UI Reference` workflow builds the publicly released
[`BirchWoodGod/halo-ce-vita` v1.0](https://github.com/BirchWoodGod/halo-ce-vita/tree/v1.0)
at commit `b9409394c0816af72cec477cc7d3a4a7a855d06a`. Its source and
GPL-3.0 license remain in that repository; the CI artifact includes the
license and exact source commit. No retail maps, Xbox executable, keys, or
assets are committed or placed in the VPK.

This is an **independent native GXM build**, not a build of our vitaGL code.
Its complete game main loop, memory layout, shader translation and GPU device
are different. It provides a usable comparison while we complete the original
UI/update and later scenario closure in this repository. Do not count a
successful reference build or a donor screenshot as HaloCEVita's own UI or
world-rendering acceptance.

| Package | Source | Data path | Title ID |
| --- | --- | --- | --- |
| `HaloCE.vpk` | This repository, vitaGL staged shell | `ux0:data/HaloCE/maps/` | `HCEV00001` |
| `halo.vpk` from the reference workflow | BirchWoodGod v1.0, native GXM | `ux0:data/haloce-vita/maps/` | `HCEV00001` |

Both packages use the same Vita title ID. Installing either replaces the
other bubble. The reference expects user-supplied Xbox maps in its own data
directory; `ui.map` starts the menu, while selecting a campaign/multiplayer
level requires its corresponding map. Its separate `default.xbe` supplies the
loading picture. See the [donor installation guide](https://github.com/BirchWoodGod/halo-ce-vita/blob/v1.0/port/vita/README.md).

## What can transfer to our renderer

- Keep the original 640x480 widget coordinates separate from the final Vita
  display resolution. Our rasterizer contract already protects this.
- Preserve authored texture alpha, blend factors, topology and final raster
  state at each original draw. Our vitaGL state/upload ordering and current
  alpha probe should be checked on hardware before another change.
- Reuse original Halo UI handlers, keyboard and full main-loop ownership as
  their dependencies are linked. The present staged shell cannot be labeled
  complete while world/network handlers remain guarded.

GXM command buffers, Cg programs, texture heaps and host APIs cannot be
copied into vitaGL as drop-in replacements. Any future adoption needs an
explicit backend choice and measured Vita behavior. See
`VITA_RENDERER_BEHAVIOR_CONTRACTS.md` for our current renderer contract.
