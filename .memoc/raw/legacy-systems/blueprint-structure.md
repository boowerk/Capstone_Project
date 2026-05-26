---
memoc: true
type: raw
scope: project-memory
created: 2026-05-21T06:58:20
updated: 2026-05-21T06:58:20
status: active
tags:
  - memoc
  - memoc/system
  - memoc/raw
---
# Blueprint Structure

## Purpose

Blueprint structure verified through the running Unreal Editor MCP Automation Bridge Native MCP endpoint at `http://127.0.0.1:3000/mcp`.

## MCP Verification

- Unreal Editor process was running.
- Native MCP port `3000` was reachable.
- WebSocket bridge port `8091` was reachable.
- MCP server initialized as `unreal-mcp` version `0.6.0`.
- Used MCP tools:
  - `manage_blueprint` with `get_graph_details`
  - `manage_blueprint` with `get_scs`
  - `inspect` with `inspect_cdo`

## Region Blueprints

### `/Game/RegionSystem/Blueprints/BP_RegionSeed`

- Parent class: `Actor`
- Variables:
  - `SeedIndex:int`
  - `BaseType:ERegionBaseType`
  - `State:int`
  - `bLockRegionSettings:bool`
- Defaults:
  - `SeedIndex=0`
  - `BaseType=NewEnumerator0`
  - `State=0`
  - `bLockRegionSettings=False`
- Functions:
  - `UserConstructionScript`
  - `UpdateDebugText`
- Events:
  - `ReceiveBeginPlay`
  - `ReceiveActorBeginOverlap`
  - `ReceiveTick`
- SCS components:
  - `DefaultSceneRoot:SceneComponent`
  - `Billboard:BillboardComponent`
  - `SeedIndexText:TextRenderComponent`
  - `SeedBaseTypeText:TextRenderComponent`
  - `StateText:TextRenderComponent`

### `/Game/RegionSystem/Blueprints/BP_RegionStateManager`

- Parent class: `Actor`
- Variables:
  - `LandscapeRef:Landscape`
  - `RegionCount:int`
  - `RegionStates:Array<int>`
  - `StateRT:TextureRenderTarget2D`
  - `RegionStateCount:int`
  - `SeedWorldPositions:Array<Vector2D>`
  - `ExportSeedText:string`
  - `RegionIdTexture:Texture2D`
  - `BaseTypeRandomSeed:int`
  - `RandomBaseTypePool:Array<ERegionBaseType>`
- Defaults:
  - `RegionCount=16`
  - `StateRT=/Game/RegionSystem/RenderTargets/RT_RegionState_20x1`
  - `RegionStateCount=4`
  - `RegionIdTexture=/Game/RegionSystem/Textures/RegionID/T_RegionMapLT_RegionID`
  - `RandomBaseTypePool=(NewEnumerator0, NewEnumerator0, NewEnumerator0, NewEnumerator1, NewEnumerator2, NewEnumerator3)`
- Functions:
  - `InitRegionStates`
  - `SetRegionState`
  - `CreateStateRT`
  - `RebuildStateTexture`
  - `ApplyLandscapeMaterial`
  - `CollectSeedWorldPositions`
  - `BuildSeedExportText`
  - `RefreshRegionTexture`
  - `ExportSeedsForPython`
  - `DeubgRandomRegionStates`
  - `GetNearestSeedIndex`
  - `SyncRegionSeedStates`
  - `DebugPrintNearestSeedState`
  - `SyncPCG`
  - `RandomizeSeedBaseTypes`
  - `SetRegionSeedState`
- Events:
  - `ReceiveBeginPlay`
  - `ReceiveActorBeginOverlap`
  - `ReceiveTick`
- SCS components:
  - `DefaultSceneRoot:SceneComponent`
  - `Billboard:BillboardComponent`

### `/Game/RegionSystem/Blueprints/BP_VegetationSpawner`

- Parent class: `Actor`
- Variables:
  - `AliveVegetationDA:Object`
  - `DeadVegetationDA:Object`
  - `CorruptedVegetationDA:Object`
- Defaults:
  - `AliveVegetationDA=/Game/RegionSystem/Data/Vegetation/DA_NatureAliveVegetation`
  - `DeadVegetationDA=/Game/RegionSystem/Data/Vegetation/DA_NatureDeadVegetation`
  - `CorruptedVegetationDA=/Game/RegionSystem/Data/Vegetation/DA_NatureCorruptedVegetation`
- SCS components:
  - `DefaultSceneRoot:SceneComponent`
  - `PCG_Vegetation:PCGComponent`
  - `Box:BoxComponent`

## City Blueprints

### `/Game/RegionSystem/Blueprints/City/BP_CityAnchor`

- Parent class: `Actor`
- Variables:
  - `CityId:int`
  - `TargetRegionIndex:int`
  - `CityData:Object`
- Defaults:
  - `CityId=0`
  - `TargetRegionIndex=-1`
  - `CityData=None`
- SCS components:
  - `DefaultSceneRoot:SceneComponent`
  - `PCG_City:PCGComponent`
  - `Box:BoxComponent`

### `/Game/RegionSystem/Blueprints/City/BP_CityRoadSpline`

- Parent class: `Actor`
- Variables:
  - `CityId:int`
  - `RoadWidth:float`
- Defaults:
  - `CityId=0`
  - `RoadWidth=800`
- SCS components:
  - `DefaultSceneRoot:SceneComponent`
  - `Spline:SplineComponent`

### `/Game/RegionSystem/Blueprints/City/BP_CityDistrictSpline`

- Parent class: `Actor`
- Variables:
  - `CityId:int`
  - `DistrictType:name`
  - `Priority:int`
- Defaults:
  - `CityId=0`
  - `DistrictType=Residential`
  - `Priority=0`
- SCS components:
  - `DefaultSceneRoot:SceneComponent`
  - `Spline:SplineComponent`

## Gameplay Blueprints

### `/Game/Characters/PlayerCharacter/BP_GP_PlayerCharacter`

- Parent class: `GP_PlayerCharacter`
- Variable:
  - `DebugWidget:WBP_DebugAttributeWidget_C`
- Functions:
  - `LockOnGetTarget`
  - `LockOnRotateAction`
  - `LockOnSet`
  - `ActiveAbilityByTagPureNode`
- Events:
  - `ReceiveBeginPlay`
  - `ReceiveActorBeginOverlap`
  - `ReceiveTick`
- SCS/native component view:
  - `CollisionCylinder:CapsuleComponent`
  - `Arrow:ArrowComponent`
  - `CharMoveComp:CharacterMovementComponent`
  - `CharacterMesh0:SkeletalMeshComponent`
  - `CameraBoom:SpringArmComponent`
  - `FollowCamera:CameraComponent`
  - `AIPerceptionStimuliSource:AIPerceptionStimuliSourceComponent`

### `/Game/Characters/EnemyCharacter/BP_Enemy_Base`

- Parent class: `GP_EnemyCharacter`
- Functions:
  - `UserConstructionScript`
- Events:
  - `ReceiveBeginPlay`
  - `ReceiveActorBeginOverlap`
  - `ReceiveTick`
- SCS/native component view:
  - `CollisionCylinder:CapsuleComponent`
  - `Arrow:ArrowComponent`
  - `CharMoveComp:CharacterMovementComponent`
  - `CharacterMesh0:SkeletalMeshComponent`
  - `AbilitySystemComponent:GP_AbilitySystemComponent`
  - `AIRangeVisualizer:EnemyAIRangeVisualizationComponent`
  - `BP_GP_WidgetComponent:BP_GP_WidgetComponent_C`

### `/Game/GAS_Pattern/Game/BP_ProjectEden_Gamemode`

- Parent class: `GameModeBase`
- Functions:
  - `UserConstructionScript`
- Events:
  - `ReceiveBeginPlay`
  - `ReceiveTick`
- SCS components:
  - `DefaultSceneRoot:SceneComponent`

## UI Blueprint

### `/Game/UI/HUD/WBP_PlayerHUDWidget`

- Variable:
  - `ASC:AbilitySystemComponent`
- Events:
  - `PreConstruct`
  - `Construct`
  - `Tick`
- SCS components:
  - None reported by `get_scs`; this is expected for Widget Blueprints.

## Remaining Limits

- MCP confirmed variables, defaults, events, functions, parent classes, and SCS/component structure.
- Full visual Blueprint node graphs were not exhaustively exported.
- PCG graph node internals and Material graph internals still need dedicated MCP queries or direct Unreal Editor inspection.
