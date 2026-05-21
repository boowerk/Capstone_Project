---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-05-21T07:03:24
status: active
tags:
  - memoc
  - memoc/state
---
# Decisions

Durable project decisions live here. Keep entries short, dated, and useful to future agents.

## Decision Log

### 2026-05-20
- Use `UEFNSourceMesh` as the runtime animation-driving source and let `CharacterMesh0` (`MaskMan`) receive pose via `Retarget Pose From Mesh`, instead of driving MaskMan first.
- Reuse existing UEFN mannequin pose-search assets first; start with a minimal source AnimBP and a single existing PSD before building a fuller chooser/database stack.
- Prefer explicit runtime DB selection in `GP_CharacterAnimInstance` over the stock UEFN chooser for now, because chooser context wiring in this project/tooling path did not switch databases reliably in PIE.
- Prefer separate fixed `Motion Matching` nodes per locomotion state over hot-swapping the `Database` pin on a single node, because runtime DB replacement did not produce visible pose changes in PIE.
- Keep `CurrentMotionMatchState` enum as locomotion branch source of truth; remove temporary `bUse*MotionMatch` helper flags once enum-driven blending is wired.
- Prefer the Blueprint-added `CharacterTrajectoryComponent` on `BP_GP_PlayerCharacter` as trajectory source for `ABP_UEFNSource_Player` over the temporary C++ `GeneratedTrajectory` path when validating/debugging motion matching.
- To avoid fragile Blueprint nested-property wiring, bridge the Blueprint `CharacterTrajectoryComponent` into `GeneratedTrajectory` inside `UGP_CharacterAnimInstance` via reflection, then keep `GeneratedTrajectory -> Pose History` as the AnimGraph input path.
- Do not keep expanding bespoke locomotion branches as the long-term solution; chooser should remain the main database-selection authority once its expected context variables are restored.
- Restore the stock relaxed chooser by matching its input contract in `UGP_CharacterAnimInstance` (`MovementMode`, `Stance`, `MovementState`, `Gait`, direction flags, landing flags, and previous-frame state) rather than cloning chooser logic into code.
- For MaskMan locomotion, treat default movement speed (`500`) as run-family motion and sprint speed (`700`) as sprint-family motion; do not require a walk-family chooser branch in the first custom table pass.
- Build a new custom chooser rooted at `CHT_MM_MaskMan_Root` with embedded `Idle`, `Run`, `Sprint`, and `InAir` nested choosers, using the stock relaxed chooser layouts as templates instead of trying to salvage every original branch directly.
- Use a dedicated `SprintSpeedThreshold` (currently `650`) instead of reusing the broad run threshold, so chooser gait classification does not mark base speed `500` as sprint.
