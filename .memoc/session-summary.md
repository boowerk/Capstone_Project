---
memoc: true
type: state
scope: project-memory
updated: 2026-07-23T13:39:22+09:00
status: active
tags: [memoc, memoc/state]
created: 2026-07-18T13:21:02
---
# Session Summary

## Status
- Mixed 00/01 presets are committed as `8f794c86`.
- Current: flat footprints and transient `Rebuild/Clear Preview`. PIE defaults to `PreviewSeed`; packaged runtime uses GameState RunSeed and On-Demand PCG.
- Saved map slots are now uniquely identified as `Village_A..E`.

## Verified
- Full Editor build and `ProjectEden.Game.WorldLayout.VillageSelection` pass.

## Next
- PreviewSeed 186 selects A/01 + E/00. Continue authoring slots/presets.
