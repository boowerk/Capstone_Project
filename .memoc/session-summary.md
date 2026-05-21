# Session Summary
Last: 2026-05-20T18:25:00
Keep each section ≤ 3 bullets. Agent-owned — updated by you, not by `memoc update`.

## Status
- Runtime retarget path still uses `UEFNSourceMesh` as animation source and `CharacterMesh0` (`MaskMan`) as retarget target.
- `BP_GP_PlayerCharacter` has a Blueprint `CharacterTrajectoryComponent`, and `UGP_CharacterAnimInstance` now bridges that into `GeneratedTrajectory`.
- New custom chooser authoring started under `ChooserTables/CHT_MM_MaskMan_Root`, with `Idle / Run / Sprint / InAir` embedded nested choosers tuned for MaskMan's `500 -> Run`, `700 -> Sprint` locomotion.

## Changed
- Exported `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases_Relaxed` and identified the root and nested chooser inputs.
- Added chooser-facing enums and variables to `UGP_CharacterAnimInstance`: `MovementMode`, `Stance`, `MovementState`, `Gait`, `MovementDirection`, `MovementDirection_Recent`, `Speed2D`, `MovementMode_LastFrame`, `Gait_LastFrame`, `IsStarting`, `IsPivoting`, `ShouldSpinTransition`, `JustTraversed`, `JustLanded_Light`, `JustLanded_Heavy`, `ShouldTurnInPlace`.
- Kept the current enum-branch AnimGraph alive as a temporary fallback while the chooser path is being restored safely, and fixed the user-found one-slot blend offset in `ABP_UEFNSource_Player`.
- Seeded `CHT_MM_MaskMan_Root` embedded chooser rows for `Idle` (`Idles`, `TurnInPlace`, `Run_Stops`, `Sprint_Stops`, `Idle_Lands_*`), `Run`, `Sprint`, and `InAir`.
- Switched the default runtime chooser load path to `CHT_MM_MaskMan_Root` and split sprint detection onto `SprintSpeedThreshold = 650` so base speed `500` remains run-family.

## Open Tasks
- Point runtime chooser evaluation at `CHT_MM_MaskMan_Root`.
- Verify in PIE that `Idle / TurnInPlace / Run / Sprint / InAir` resolve to the intended PSDs and that the half-step start issue improves.
- Decide whether `MovementDirection_Recent` needs separate exposure before expanding the new `Run` chooser beyond the initial forward-focused rows.

## Resume
- Start with `GP_CharacterAnimInstance.h/.cpp`; chooser input contract and runtime evaluation live there.
- Then finish wiring runtime to `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/ChooserTables/CHT_MM_MaskMan_Root` and validate the new embedded nested choosers.
