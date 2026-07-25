---
memoc: true
type: state
scope: project-memory
updated: 2026-07-25T21:53:00+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-24T01:26:27
---
# Session Summary

- `8dbbca6e` commits GameMode policy/state and responsibility splits.
- Uncommitted: extracted `GPVillagePresetPolicy`; split Village editor-preview implementation while retaining public compatibility wrappers.
- No reflected API, selection algorithm, or gameplay behavior changed.
- Editor/Server builds and Village selection automation pass.
- Next: create an Editor-only test module, then continue Village streaming/PCG separation.
