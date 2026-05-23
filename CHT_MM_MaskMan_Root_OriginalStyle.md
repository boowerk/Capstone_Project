# Chooser Table Multi-Table Parse Report
- **Asset Path:** `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_MM_MaskMan_Root_OriginalStyle.uasset`
- **File Size:** 30,223 bytes
- **Context Source ABP:** `ABP_UEFNSource_Player_C`

## Overview
This asset is structured as a **Multi-Table Chooser Asset**, containing **7 sub-tables** inside a single `.uasset` file.

### [Sub-Table #1] Base Selection Table (Grounded / Base Branch)
- Row Count: 5

| Row | MovementMode | MovementState (Sub) | MovementState | Gait | Result PSD |
| --- | --- | --- | --- | --- | --- |
| 0 | Grounded | val(0) | Idle | Idle | `Sub-Table #3 (Stand Idle / Turn / Stops)` |
| 1 | Grounded | val(0) | Move | Idle | `Sub-Table #6 (Stand Walk Evaluation Table)` |
| 2 | Grounded | val(0) | Move | Walk | `Sub-Table #4 (Stand Run Evaluation Table)` |
| 3 | Grounded | val(0) | Move | Run | `Sub-Table #5 (Stand Sprint Starts/Loops/Pivots Table)` |
| 4 | InAir | val(0) | Idle | Idle | `Sub-Table #2 (In-Air (Jumps / Falls) Evaluation Table)` |

### [Sub-Table #2] In-Air (Jumps / Falls) Evaluation Table
- Row Count: 2

| Row | Speed2D | JustTraversed | Result PSD |
| --- | --- | --- | --- |
| 0 | >= 1 | Any | `PSD_Dense_Jumps_Far` |
| 1 | >= 0 | Any | `PSD_Extreme_Sparse_Jumps` |

### [Sub-Table #3] Stand Idle / TurnInPlace / Stops Table
- Row Count: 6

| Row | Speed2D | JustLanded_Light | JustLanded_Heavy | ShouldTurnInPlace | Result PSD |
| --- | --- | --- | --- | --- | --- |
| 0 | 0 ~ 20 | False | False | False | `PSD_Dense_Stand_Idles` |
| 1 | 20 ~ 310 | False | False | Any | `PSD_Sparse_Stand_Walk_Stops` |
| 2 | >= 310 | False | False | Any | `PSD_Dense_Stand_Run_Stops` |
| 3 | Any | True | False | Any | `PSD_Dense_Stand_Idle_Lands_Light` |
| 4 | Any | False | True | Any | `PSD_Dense_Stand_Idle_Lands_Heavy` |
| 5 | Any | False | False | True | `PSD_Dense_Stand_TurnInPlace` |

### [Sub-Table #4] Stand Run Evaluation Table
- Row Count: 4

| Row | IsStarting | IsPivoting | JustTraversed | JustLanded_Light | JustLanded_Heavy | IsStopping | Result PSD |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | True | Any | Any | False | False | Any | `PSD_Extreme_Sparse_Stand_Run_Starts` |
| 1 | Any | Any | Any | False | False | Any | `PSD_Dense_Stand_Run_Loops` |
| 2 | Any | True | Any | False | False | Any | `PSD_Sparse_Stand_Run_Pivots` |
| 3 | Any | False | Any | Any | Any | True | `PSD_Dense_Stand_Run_SpinTransition` |

### [Sub-Table #5] Stand Sprint Starts/Loops/Pivots Table
- Row Count: 3

| Row | IsStarting | IsPivoting | JustLanded_Light | JustLanded_Heavy | Result PSD |
| --- | --- | --- | --- | --- | --- |
| 0 | True | False | Any | Any | `PSD_Dense_Stand_Sprint_Starts` |
| 1 | Any | False | Any | Any | `PSD_Dense_Stand_Sprint_Loops` |
| 2 | Any | True | Any | Any | `PSD_Sparse_Stand_Sprint_Pivots` |

### [Sub-Table #6] Stand Walk Evaluation Table
- Row Count: 4

| Row | IsStarting | IsPivoting | JustTraversed | JustLanded_Light | JustLanded_Heavy | IsStopping | Result PSD |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | True | Any | Any | False | False | Any | `PSD_Extreme_Sparse_Stand_Walk_Starts` |
| 1 | Any | Any | Any | False | False | Any | `PSD_Extreme_Sparse_Stand_Walk_Loops` |
| 2 | Any | True | Any | False | False | Any | `PSD_Sparse_Stand_Walk_Pivots` |
| 3 | Any | Any | Any | Any | Any | True | `PSD_Dense_Stand_Walk_SpinTransition` |
