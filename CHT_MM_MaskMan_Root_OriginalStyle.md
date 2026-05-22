# Chooser Table Multi-Table Parse Report - CHT_MM_MaskMan_Root_OriginalStyle

- Asset Path: /Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/ChooserTables/CHT_MM_MaskMan_Root_OriginalStyle.uasset
- File Size: 30,337 bytes
- Context Source ABP: ABP_UEFNSource_Player_C

## Overview

This asset is structured as a Legacy UE5.x Multi-Table Chooser Asset, containing 6 sub-tables serialized sequentially within a single uasset file. The table structures and evaluation branches have been fully reconstructed by parsing the NameMap, ImportMap, and ExportMap byte layouts.

The evaluation process flows hierarchically: the main "Base Selection Table" branches into 5 specialized sub-tables (InAir, Stand Idles, Stand Runs, Stand Sprint, Stand Walks) based on locomotion state enums, which then evaluate detailed motion matching pose databases.

---

## [Sub-Table #1] Base Selection Table (Grounded / Base Branch)
- Offset Range: 9795 to 15651
- Row Count: 5

This table acts as the main router. It evaluates enums from the character anim instance to determine which specialized locomotor sub-table to delegate to.

| Row | MovementMode | Unknown (State Sub) | MovementState | Gait | Target Result (Sub-Table) |
| --- | --- | --- | --- | --- | --- |
| 0 | Grounded | val(0) | Idle | Idle | Stand Idles (Sub-Table #3) |
| 1 | Grounded | val(0) | Move | Idle | Stand Walks (Sub-Table #6) |
| 2 | Grounded | val(0) | Move | Walk | Stand Walks (Sub-Table #6) |
| 3 | Grounded | val(0) | Move | Run | Stand Runs (Sub-Table #4) |
| 4 | InAir | val(0) | Idle | Idle | InAir (Sub-Table #2) |

---

## [Sub-Table #2] InAir (Grounded to Falling / Jump)
- Offset Range: 15651 to 17347
- Row Count: 2

Evaluates when the character is in the air. 

| Row | Speed2D | JustTraversed | Result PoseSearchDatabase |
| --- | --- | --- | --- |
| 0 | >= 1 | Any | PSD_Dense_Jumps_Far |
| 1 | >= 0 | Any | PSD_Extreme_Sparse_Jumps |

---

## [Sub-Table #3] Stand Idles (Idling & Turn In Place / Stops)
- Offset Range: 17347 to 20826
- Row Count: 6

Handles standing idles, land transitions, stops from locomotion, and turn-in-place rotations.

| Row | Speed2D | JustLanded_Light | JustLanded_Heavy | ShouldTurnInPlace | Result PoseSearchDatabase |
| --- | --- | --- | --- | --- | --- |
| 0 | 0 ~ 20 | False | False | False | PSD_Dense_Stand_Idles |
| 1 | 20 ~ 200 | False | False | Any | PSD_Sparse_Stand_Walk_Stops |
| 2 | >= 200 | False | False | Any | PSD_Relaxed_Stand_Run_Stops |
| 3 | Any | True | False | Any | PSD_Dense_Stand_Idle_Lands_Light |
| 4 | Any | False | True | Any | PSD_Dense_Stand_Idle_Lands_Heavy |
| 5 | Any | False | False | True | PSD_Extreme_Sparse_Stand_TurnInPlace |

---

## [Sub-Table #4] Stand Runs (Running Locomotion)
- Offset Range: 20826 to 24362
- Row Count: 4

Evaluates running motion matching database ranges.

| Row | IsStarting | IsPivoting | JustTraversed | JustLanded_Light | JustLanded_Heavy | Unknown (ShouldSpin) | Result PoseSearchDatabase |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | True | Any | Any | False | False | Any | PSD_Extreme_Sparse_Stand_Run_Starts |
| 1 | Any | Any | Any | False | False | Any | PSD_Extreme_Sparse_Stand_Run_Loops |
| 2 | Any | True | Any | False | False | Any | PSD_Sparse_Stand_Run_Pivots |
| 3 | Any | False | Any | Any | Any | True | PSD_Dense_Stand_Run_SpinTransition |

---

## [Sub-Table #5] Stand Sprint (Sprinting Locomotion)
- Offset Range: 24362 to 26778
- Row Count: 3

Evaluates sprint acceleration and high-speed motion matching databases.

| Row | IsStarting | IsPivoting | JustLanded_Light | JustLanded_Heavy | Result PoseSearchDatabase |
| --- | --- | --- | --- | --- | --- |
| 0 | True | False | Any | Any | PSD_Extreme_Sparse_Stand_Sprint_Starts |
| 1 | Any | False | Any | Any | PSD_Extreme_Sparse_Stand_Sprint_Loops |
| 2 | Any | True | Any | Any | PSD_Sparse_Stand_Sprint_Pivots |

---

## [Sub-Table #6] Stand Walks (Walking Locomotion)
- Offset Range: 26778 to 30285
- Row Count: 4

Evaluates standard walking motion matching database ranges.

| Row | IsStarting | IsPivoting | JustTraversed | JustLanded_Light | JustLanded_Heavy | Unknown (ShouldSpin) | Result PoseSearchDatabase |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | True | Any | Any | False | False | Any | PSD_Extreme_Sparse_Stand_Walk_Starts |
| 1 | Any | Any | Any | False | False | Any | PSD_Extreme_Sparse_Stand_Walk_Loops |
| 2 | Any | True | Any | False | False | Any | PSD_Sparse_Stand_Walk_Pivots |
| 3 | Any | Any | Any | Any | Any | True | PSD_Dense_Stand_Walk_SpinTransition |
