---
memoc: true
type: wiki
scope: project-memory
created: 2026-07-23T07:18:32
updated: 2026-07-23T17:36:04+09:00
status: active
confidence: high
tags:
  - memoc
  - memoc/wiki
  - memoc/knowledge-wiki
  - memoc/topic
---
# Village slot capacity and preset footprint

## Summary

Keep a slot's available capacity separate from a village preset's actual
footprint.

- Add a per-slot `SlotSizeClass`: Small, Medium, and Large.
- Current target capacities are Small = 130x130m and Medium = 230x230m;
  Large remains configurable for future presets.
- Keep `FootprintExtent` and `FootprintOffset` on each preset as the source of
  truth for its actual occupied area.
- A preset may be assigned only when its footprint fits the slot capacity.
  Medium slots may accept Small presets; Small slots must reject Medium
  presets.
- Use the preset footprint, not the capacity box, for overlap rejection.
- Editor visualization should distinguish the outer slot-capacity box from
  the inner assigned-preset footprint box. Overlapping assigned footprints
  remain red.

This avoids treating a debug box as both placement capacity and actual village
size, and allows future presets to use different footprints safely.

## Evidence

- [Sources](../sources.md)

## Open Questions

- Final Large capacity dimensions.
- Whether slots also restrict presets by biome or region in addition to size.

## Implemented

- Commit `817189ac` implements Small/Medium capacity and footprint-fit
  filtering.
- `L_LandscapeMap`: slots 5 and 7 are Medium; the other five are Small.

## Related

- [Knowledge Wiki](../README.md)
- [Topics](README.md)
- [Glossary](../glossary.md)
