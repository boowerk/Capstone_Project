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
