# Chooser Table Report - CHT_MM_MaskMan_Root_OriginalStyle

- **Asset Path:** `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/ChooserTables/CHT_MM_MaskMan_Root_OriginalStyle`
- **Context Source ABP:** `ABP_UEFNSource_Player_C`
- **Verification:** editor screenshots plus binary string scan of the saved `.uasset`
- **Note:** Direct MCP inspection was unavailable because the Unreal MCP session was expired.

## Overview

This chooser is now configured as the original-style MaskMan motion matching root with explicit `Stance` routing. Standing locomotion keeps the existing stand tables, while crouching routes to new crouch idle / walk subtables that use existing dense crouch Pose Search Databases.

The obsolete C++ compatibility variables `EMMDatabaseLOD`, `MMDatabaseLOD`, and `MMDatabaseLODEnum` are not used by this table.

## Root Table

| Result | Movement Mode | Stance | Movement State | Gait |
| --- | --- | --- | --- | --- |
| Stand Idles | Grounded | Standing | Idle | Any |
| Stand Walks | Grounded | Standing | Moving | Walk |
| Stand Runs | Grounded | Standing | Moving | Run |
| Stand Sprint | Grounded | Standing | Moving | Sprint |
| Crouch Idles | Grounded | Crouching | Idle | Any |
| Crouch Walks | Grounded | Crouching | Moving | Any |
| InAir | InAir | Any | Any | Any |

## Crouch Idles

| Result | Should Turn in Place |
| --- | --- |
| `PSD_Dense_Crouch_TurnInPlace` | True |
| `PSD_Dense_Crouch_Idles` | False |

## Crouch Walks

| Result | Is Pivoting | Is Starting | Is Stopping |
| --- | --- | --- | --- |
| `PSD_Dense_Crouch_Walk_Pivots` | True | Any | Any |
| `PSD_Dense_Crouch_Walk_Starts` | Any | True | Any |
| `PSD_Dense_Crouch_Walk_Stops` | Any | Any | True |
| `PSD_Dense_Crouch_Walk_Loops` | False | False | False |

## Existing Standing / In-Air Tables

The saved `.uasset` still contains the existing standing and in-air database references:

- `PSD_Dense_Stand_Idles`
- `PSD_Dense_Stand_TurnInPlace`
- `PSD_Dense_Stand_Walk_SpinTransition`
- `PSD_Extreme_Sparse_Stand_Walk_Starts`
- `PSD_Extreme_Sparse_Stand_Walk_Loops`
- `PSD_Sparse_Stand_Walk_Pivots`
- `PSD_Sparse_Stand_Walk_Stops`
- `PSD_Extreme_Sparse_Stand_Run_Starts`
- `PSD_Dense_Stand_Run_Loops`
- `PSD_Sparse_Stand_Run_Pivots`
- `PSD_Dense_Stand_Run_SpinTransition`
- `PSD_Dense_Stand_Sprint_Starts`
- `PSD_Dense_Stand_Sprint_Loops`
- `PSD_Sparse_Stand_Sprint_Pivots`
- `PSD_Dense_Jumps_Far`
- `PSD_Extreme_Sparse_Jumps`

## PIE Checks

- Crouch idle should select `PSD_Dense_Crouch_Idles`.
- Crouch turn-in-place should select `PSD_Dense_Crouch_TurnInPlace`.
- Crouch moving should select pivot, start, stop, or loop by `IsPivoting`, `IsStarting`, and `IsStopping`.
- If crouch stopping is missed in PIE, test moving the stop row above the start row.
