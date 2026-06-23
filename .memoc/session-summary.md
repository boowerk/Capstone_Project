---
memoc: true
type: state
scope: project-memory
updated: 2026-06-23T17:59:34+09:00
status: active
created: 2026-06-22T17:59:08
tags:
  - memoc
  - memoc/state
---
# Session Summary

## Status
- Investigated client-only stutter/camera shake when connected to a running server.
- Root cause was directional `MaxWalkSpeed` selection diverging: server preferred acceleration/controller-rotation reinterpretation while client used raw move input.
- `AGP_PlayerController::ResolveEffectiveMoveInput()` now uses the same clamped raw input on both sides and keeps acceleration only as a first-frame server fallback.

## Verified
- `Project_EdenEditor Win64 Development` build succeeded.

## Handoff
- PIE/client-server visual check still recommended for movement correction and camera shake.
