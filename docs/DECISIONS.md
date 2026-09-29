# Architectural decisions

This is a lightweight ADR log. Append; do not silently reverse decisions.

## D001 — Independent repository, not a GitHub fork

Date: 2026-09-28

Decision: maintain HaloCEVita as a separate repository with its own history.

Reason: Vita work is expected to develop a distinct build/platform layer and should not be constrained by fork presentation/history. Upstream remains a remote/reference source.

## D002 — Native 32-bit Vita port first

Date: 2026-09-28

Decision: target native `arm-vita-eabi` code rather than Android's AArch64 ILP32 guest/host architecture.

Reason: Vita is already a 32-bit ARM platform, while the Android guest/host architecture primarily solves the conflict between Halo's 32-bit layout and modern 64-bit-only Android.

Revisit only if a concrete memory/layout/toolchain blocker proves the direct approach unworkable.

## D003 — Reuse upstream NV2A/D3D8 translation

Date: 2026-09-28

Decision: adapt the existing GL renderer/shader translation to vitaGL rather than recreating Halo materials/rendering from scratch.

Reason: upstream already implements the hard semantic translation from Xbox D3D8/NV2A state to shaders and GL state.

## D004 — 30 FPS before 60 FPS

Date: 2026-09-28

Decision: initial performance target is stable 30 FPS.

Reason: matches the original console design target and reduces premature optimization during bring-up.
