# Known issues / open risks

Use stable IDs so attempts and commits can reference the same problem.

## KI-001 — Vita source not imported yet

Severity: blocking initial compilation.

The repository scaffold exists, but the upstream source must be imported on the development PC. Record the imported SHA in `docs/UPSTREAM.md`.

## KI-002 — Compiler/ABI differences from x86/MSVC assumptions

Severity: high.

The upstream Linux build uses compiler flags and generated compatibility headers to reproduce important MSVC/Xbox semantics. ARM Vita needs an equivalent audit for struct layout, calling conventions, wchar width, FP behavior and undefined-behavior assumptions.

Do not solve this by globally disabling warnings/errors without understanding the affected ABI.

## KI-003 — Dynamic GLSL subset on vitaGL

Severity: high.

Halo Universal generates vertex and pixel shaders dynamically from Xbox NV2A state. vitaGL supports runtime shader compilation, but the exact generated GLSL syntax/features must be validated.

## KI-004 — 3D textures

Severity: medium/high until usage is known.

Upstream supports `GL_TEXTURE_3D`/`sampler3D`. Determine which Halo content actually uses it and whether vitaGL can support or emulate the required path.

## KI-005 — Memory pressure

Severity: high later in rendering.

Do not copy Android's large stream-buffer ring sizes. Establish Vita-specific budgets once rendering starts.

## KI-006 — Vita3K is not authoritative for all homebrew behavior

Severity: medium.

Plugin/runtime shader compiler/data-path behavior may differ from real hardware. Track test platform explicitly.
