---
memoc: true
type: state
scope: project-memory
updated: 2026-07-19T22:20:00+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-18T13:21:02
---
# Session Summary

## Status
- V2.2 uses per-boundary PCHIP wobble (2.5m coarse/0.65m detail, 22-58m, fixed junctions). Weight is non-VT/Bilinear/NoMip. A stale failed shader caused the gray default material; toggling V2.2 off/on forced 13/13 recompile and restored the map. MI saved true; map file untouched.

## Verified
- Generator 42/42; material 13/13, no new sampler error. Weight `E851B002...679B73`; MI `9673F631...F049B`.

## Resume
- The 1.08m Region 1-13 edge stays fixed. Other material/projection issues are separate; source mirrors match.
