---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-23T16:08:07+09:00
updated: 2026-07-23T16:08:07+09:00
status: complete
tags:
  - memoc
  - worklog
  - world-layout
  - village
  - memoc/worklog
---
# Center Village_01 on its level origin

- Shifted the five authored Village_01 layout actors together by local `Y +2000cm`; kept `PCGWorldActor0` at origin.
- Centered Village_01 Footprint XY in native defaults and the placed `L_LandscapeMap` Director override.
- Added regression checks for centered Village_00 and Village_01 Footprints.
- Rebuild Preview seed 186 visually aligned the compact village and 130m Footprint.
- Full `Project_EdenEditor` build and `ProjectEden.Game.WorldLayout.VillageSelection` passed.
