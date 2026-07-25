---
memoc: true
type: state
scope: project-memory
updated: 2026-07-25T22:18:51+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-24T01:26:27
---
# Session Summary

- `8dbbca6e`: GameMode policy/state and responsibility splits.
- `91eddf95`: Village preset policy and editor-preview split.
- `241f1351`: Editor-only `Project_EdenTests` module plus four moved tests.
- Uncommitted: eight more AI policy, lobby, run-seed, and session tests moved unchanged.
- Editor/Server builds and all eight tests pass; Server excludes the test module.
- Next: migrate the remaining low-dependency tests, then separate Village streaming/PCG state.
