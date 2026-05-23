# Chooser Table Multi-Table Parse Report - CHT_MM_MaskMan_Root_OriginalStyle
- **Asset Path:** `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_MM_MaskMan_Root_OriginalStyle.uasset`
- **File Size:** 30,337 bytes
- **Context Source ABP:** `ABP_UEFNSource_Player_C`

## Overview
This asset is structured as a **Multi-Table Chooser Asset**, containing **6 sub-tables** inside a single `.uasset` file.

### [Sub-Table #1] CHT_MM_MaskMan_Root_OriginalStyle
- Offset Range: 9795 ~ 15651
- Row Count: 5

| Row | MovementMode | Unknown | MovementState | Gait | Target Result |
| --- | --- | --- | --- | --- | --- |
| 0 | Grounded | val(0) | Idle | Idle | Sub-Table #2 (InAir) |
| 1 | Grounded | val(0) | Move | Idle | Sub-Table #3 (Stand Idles) |
| 2 | Grounded | val(0) | Move | Walk | Sub-Table #4 (Stand Runs) |
| 3 | Grounded | val(0) | Move | Run | Sub-Table #5 (Stand Sprint) |
| 4 | InAir | val(0) | Idle | Idle | Sub-Table #6 (Stand Walks) |

### [Sub-Table #2] InAir
- Offset Range: 15651 ~ 17347
- Row Count: 2

| Row | Speed2D | JustTraversed | Target Result |
| --- | --- | --- | --- |
| 0 | >= 1 | Any | `PSD_Dense_Jumps_Far` |
| 1 | >= 0 | Any | `PSD_Extreme_Sparse_Jumps` |

### [Sub-Table #3] Stand Idles
- Offset Range: 17347 ~ 20826
- Row Count: 6

| Row | Speed2D | JustLanded_Light | JustLanded_Heavy | ShouldTurnInPlace | Target Result |
| --- | --- | --- | --- | --- | --- |
| 0 | 0 ~ 20 | False | False | False | `PSD_Dense_Stand_Idles` |
| 1 | 20 ~ 200 | False | False | Any | `PSD_Sparse_Stand_Walk_Stops` |
| 2 | >= 200 | False | False | Any | `PSD_Relaxed_Stand_Run_Stops` |
| 3 | Any | True | False | Any | `PSD_Dense_Stand_Idle_Lands_Light` |
| 4 | Any | False | True | Any | `PSD_Dense_Stand_Idle_Lands_Heavy` |
| 5 | Any | False | False | True | `PSD_Extreme_Sparse_Stand_TurnInPlace` |

### [Sub-Table #4] Stand Runs
- Offset Range: 20826 ~ 24362
- Row Count: 4

| Row | IsStarting | IsPivoting | JustTraversed | JustLanded_Light | JustLanded_Heavy | Unknown | Target Result |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | True | Any | Any | False | False | Any | `PSD_Extreme_Sparse_Stand_Run_Starts` |
| 1 | Any | Any | Any | False | False | Any | `PSD_Extreme_Sparse_Stand_Run_Loops` |
| 2 | Any | True | Any | False | False | Any | `PSD_Sparse_Stand_Run_Pivots` |
| 3 | Any | False | Any | Any | Any | True | `PSD_Dense_Stand_Run_SpinTransition` |

### [Sub-Table #5] Stand Sprint
- Offset Range: 24362 ~ 26778
- Row Count: 3

| Row | IsStarting | IsPivoting | JustLanded_Light | JustLanded_Heavy | Target Result |
| --- | --- | --- | --- | --- | --- |
| 0 | True | False | Any | Any | `PSD_Extreme_Sparse_Stand_Sprint_Starts` |
| 1 | Any | False | Any | Any | `PSD_Extreme_Sparse_Stand_Sprint_Loops` |
| 2 | Any | True | Any | Any | `PSD_Sparse_Stand_Sprint_Pivots` |

### [Sub-Table #6] Stand Walks
- Offset Range: 26778 ~ 30285
- Row Count: 4

| Row | IsStarting | IsPivoting | JustTraversed | JustLanded_Light | JustLanded_Heavy | Unknown | Target Result |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | True | Any | Any | False | False | Any | `PSD_Extreme_Sparse_Stand_Walk_Starts` |
| 1 | Any | Any | Any | False | False | Any | `PSD_Extreme_Sparse_Stand_Walk_Loops` |
| 2 | Any | True | Any | False | False | Any | `PSD_Sparse_Stand_Walk_Pivots` |
| 3 | Any | Any | Any | Any | Any | True | `PSD_Dense_Stand_Walk_SpinTransition` |
