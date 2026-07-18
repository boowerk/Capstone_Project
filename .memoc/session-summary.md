---
memoc: true
type: state
scope: project-memory
updated: 2026-07-18T16:53:56+09:00
status: active
tags: [memoc, memoc/state]
---
# Session Summary

## Status
- `L_GameMap1` and `L_LandscapeMap` runtime vegetation work. Shared graph: default 128m, current grass 32m, cleanup 1.5, bounded samplers.

## Verified
- `L_LandscapeMap` Box `32cm` -> `64000,64000,8000`; PIE grass 1,911 at start, 1,803 after 400m move, 50 cells pooled. No cache/no-surface error.

## Handoff
- Other maps need per-instance runtime/partition/bounds checks. Existing non-grass Bounds Modifier warnings remain.
