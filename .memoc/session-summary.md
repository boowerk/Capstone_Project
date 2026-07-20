---
memoc: true
type: state
scope: project-memory
updated: 2026-07-21T06:47:00+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-18T07:01:20
---
# Session Summary

- `main` merges local work through `b1baeb60` with origin through `8c9cd99b`.
- Dark Knight uses Basic 350/Heavy 420/Dark Wave 520cm; Charge/GroundCrack stay ranged.
- FurnaceWalker gains root-motion turns and an attack step. Committed actions and cancel-without-fallback-hit remain preserved.
- Foley/Niagara additions remain; origin's broken legacy EarlyTransition trio is excluded.
- Build and local tests pass: AI 22/22, Dark Knight 6/6, Network 2/2, Lobby 2/2. No live server run.
- `TestMap`, `DA_RegionEventData`, and `L_MainMap` are clean and unchanged.
