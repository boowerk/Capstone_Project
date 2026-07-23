---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-23T16:39:19+09:00
updated: 2026-07-23T16:39:19+09:00
status: complete
tags:
  - memoc
  - worklog
  - world-layout
  - village
  - memoc/worklog
---
# Correct Village_01 anchor center

- Removed the accidental local `Y +2000cm` double correction from the five authored Village_01 layout actors while preserving their relative layout.
- Kept `PCGWorldActor0` and `BP_CityAnchor` at level XY origin.
- Kept the centered 130m Footprint configuration unchanged.
- Rebuild Preview seed 186 in Top Orthographic confirmed exact XY center alignment.
