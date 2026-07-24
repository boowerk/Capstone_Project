---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-24T07:15:39
updated: 2026-07-24T07:15:39
status: active
tags:
  - memoc
  - memoc/worklog
---
# Add final deployment script

actor: douyun0623
actor_source: git config user.name
branch: main
status: done
created: 2026-07-24T07:15:39

## Summary

- Added a one-shot client/server build, cook, package, and verification batch.
- Final outputs are versioned under `Saved/FinalDeploy`, with dirty-tree protection and separate Client/Server folders.
- The current engine defaults to supported Development deployment; unsupported Shipping is rejected before compilation.

## Changed Files

- `.memoc/02-current-project-state.md`
- `.memoc/03-decisions.md`
- `.memoc/session-summary.md`
- `Project_Eden/Scripts/DedicatedServer/BuildCookDeployFinal.bat`
- `Project_Eden/Scripts/DedicatedServer/PreparePCGExClientBuild.bat`
- `Project_Eden/Source/Project_EdenServer.Target.cs`

## Verification

- Development preflight passed; explicit Shipping produced the expected unsupported-engine exit.
- Help/argument parsing and nested PCGEx `--no-pause` failure behavior passed.
- Full Development build and cook were not run.

## Follow-up

_None._

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/douyun0623.md)
