---
memoc: true
type: worklog
scope: project-memory
created: 2026-06-01T05:21:03
updated: 2026-06-01T05:21:03
status: active
tags:
  - memoc
  - memoc/worklog
---
# fix memoc wrapper timeout

actor: lim
actor_source: git config user.name
branch: main
status: done
created: 2026-06-01T05:21:03

## Summary

- Reproduced `memoc.cmd summary/search/doctor` timing out in the Codex sandbox while direct approved `node .../cli.js` execution was fast.
- Added project-local `.memoc/runtime` and updated wrappers to prefer it before global AppData/home runtimes.
- Verified `summary`, `search`, and `doctor` now complete in under 1 second without escalation.

## Changed Files

- `.memoc/bin/memoc`
- `.memoc/bin/memoc.cmd`
- `.memoc/bin/memoc.ps1`
- `.memoc/runtime/`
- `.memoc/02-current-project-state.md`
- `.memoc/session-summary.md`
- `.memoc/worklog/lim/2026-06/20260601T0521-fix-memoc-wrapper-timeout.md`

## Verification

- `.\.memoc\bin\memoc.cmd --version`
- `.\.memoc\bin\memoc.cmd summary`
- `.\.memoc\bin\memoc.cmd search timeout --limit 5`
- `.\.memoc\bin\memoc.cmd doctor`

## Follow-up

_None._

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/lim.md)
