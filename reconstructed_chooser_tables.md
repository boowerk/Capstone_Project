# Chooser Table Multi-Table Parse Report
- **Asset Path:** `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases_ExtremeSparse.uasset`
- **File Size:** 31,643 bytes
- **Context Source ABP:** `SandboxCharacter_CMC_ABP_C`

## Overview
This asset is structured as a **Multi-Table Chooser Asset**, containing **7 sub-tables** inside a single `.uasset` file.

### [Sub-Table #1] Base Selection Table (Grounded / Base Branch)
- Row Count: 7

| Row | MovementMode | MovementState (Sub) | MovementState | Gait | Result PSD |
| --- | --- | --- | --- | --- | --- |
| 0 | Grounded | val(0) | Idle | Idle | `PSD_Dense_Jumps_Far` |
| 1 | Grounded | val(0) | Move | Idle | `PSD_Dense_Stand_Idles` |
| 2 | Grounded | val(0) | Move | Walk | `PSD_Extreme_Sparse_Crouch_Idles` |
| 3 | Grounded | val(0) | Move | Run | `PSD_Extreme_Sparse_Crouch_TurnInPlace` |
| 4 | val(1) | val(0) | Idle | Idle | `PSD_Extreme_Sparse_Crouch_Walk_Loops` |
| 5 | Grounded | val(1) | Idle | Idle | `PSD_Extreme_Sparse_Crouch_Walk_Pivots` |
| 6 | Grounded | val(1) | Move | Idle | `PSD_Extreme_Sparse_Crouch_Walk_Starts` |

### [Sub-Table #2] Crouch Idles / TurnInPlace Table
- Row Count: 2

| Row | Speed2D | ShouldTurnInPlace | Result PSD |
| --- | --- | --- | --- |
| 0 | 0 ~ 20 | Any | `PSD_Extreme_Sparse_Crouch_Idles` |
| 1 | Any | True | `PSD_Extreme_Sparse_Crouch_TurnInPlace` |

### [Sub-Table #3] Crouch Walk (Type A - Basic Speed Check)
- Row Count: 2

| Row | Speed2D | Result PSD |
| --- | --- | --- |
| 0 | 0 ~ 999 | `PSD_Extreme_Sparse_Crouch_Walk_Loops` |
| 1 | 1 ~ 999 | `PSD_Extreme_Sparse_Crouch_Walk_Starts` |

### [Sub-Table #4] Crouch Walk (Type B - Main Crouch Walk Evaluation)
- Row Count: 4

| Row | Gait | Speed2D | JustLanded_Light | ShouldTurnInPlace | Result PSD |
| --- | --- | --- | --- | --- | --- |
| 0 | Idle | 0 ~ 20 | False | False | `PSD_Extreme_Sparse_Crouch_Walk_Starts` |
| 1 | Idle | 0 ~ 0 | False | True | `PSD_Extreme_Sparse_Crouch_Walk_Pivots` |
| 2 | Walk | 20 ~ 999 | False | False | `PSD_Extreme_Sparse_Crouch_Walk_Loops` |
| 3 | Idle | 0 ~ 999 | True | False | `PSD_Extreme_Sparse_Crouch_Walk_Stops` |

### [Sub-Table #5] Stand Run Evaluation Table
- Row Count: 2

| Row | IsStarting | JustLanded_Light | JustLanded_Heavy | Result PSD |
| --- | --- | --- | --- | --- |
| 0 | True | False | False | `PSD_Extreme_Sparse_Stand_Run_Starts` |
| 1 | Any | False | False | `PSD_Extreme_Sparse_Stand_Run_Loops` |

### [Sub-Table #6] Stand Sprint Starts/Loops Table
- Row Count: 2

| Row | IsStarting | Result PSD |
| --- | --- | --- |
| 0 | True | `PSD_Extreme_Sparse_Stand_Sprint_Starts` |
| 1 | Any | `PSD_Extreme_Sparse_Stand_Sprint_Loops` |

### [Sub-Table #7] Stand Walk Table
- Row Count: 2

| Row | IsStarting | JustTraversed | JustLanded_Light | JustLanded_Heavy | IsPivoting | Result PSD |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | True | Any | False | False | Any | `PSD_Extreme_Sparse_Stand_Walk_Starts` |
| 1 | Any | Any | False | False | Any | `PSD_Extreme_Sparse_Stand_Walk_Loops` |
