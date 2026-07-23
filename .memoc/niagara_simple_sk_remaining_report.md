---
memoc: true
type: core
scope: project-memory
created: 2026-07-21T12:29:05
updated: 2026-07-21T12:29:05
status: active
tags:
  - memoc
  - memoc/core
---
# NS_Simple_SK 잔여 설정 결과

## 성공

- Niagara System `/Game/Niagara/Dissolve_SK/NS_Simple_SK` validation 재확인.
  - 실제 확인값: `isValid=true`, errors `[]`, warnings `[]`.
  - 기존 사용자가 추가한 Initialize/Update Mesh Reproduction Sprite는 생성·제거·수정하지 않음.
- 기존 Material `/Game/Niagara/Dissolve_SK/M_Mannequin_Simple_SK`의 Masked, Niagara Sprite Usage, SphereMask Opacity Mask는 유지.

## 실패 또는 미검증

- Lifetime, Loop Duration, Update Sprite Size, Mesh Source, Renderer Alignment/Facing/Material:
  - 원인: 현재 MCP/Python은 기존 Niagara Emitter Stack module input과 Sprite Renderer 속성의 실제 read/write API를 노출하지 않음.
  - 시도: `manage_effect` Stack action, NiagaraSystem Python reflection, Clipboard API 조사.
  - 최소 수작업: `NS_Simple_SK > SimpleSpriteBurst` Stack에서 Lifetime=5, Emitter Loop Duration=5, Update Mesh Reproduction Sprite Size=5, 두 module Mesh input=`SKM_Manny_Simple_DissolveSample`, Sprite Renderer Alignment=Custom Alignment, Facing=Custom Facing Vector, Material=`M_Mannequin_Simple_SK` 확인·설정.
- `Particles.SpriteFacing`:
  - 원인: `set_niagara_parameter` handler가 `UNKNOWN_ACTION`.
  - 최소 수작업: Mesh Reproduction module이 직접 write하지 않으면 Set Parameters에 `(1,1,1)` 설정.
- UV/Normal:
  - UV Function 경로는 확인됐으나 function output pin Python API가 노출되지 않아 Texture Sample UV 연결 실패.
  - Normal output pin도 미노출. 기존 Base Color/Normal Texture와 SphereMask는 보존.
  - 최소 수작업: `Niagara_MeshReproductionSpriteUVs` UV → Base Color/Normal Texture Sample UVs, Normal → 기존 Normal 경로 결합.
- Material Compile: recompile 호출은 이전 작업에서 성공했으나 compile error 직접 조회 API 없음. NOT_VERIFIED.

## 최종 검증

| 항목 | 실제값 | 상태 |
|---|---|---|
| Lifetime | Stack readback 없음 | NOT_VERIFIED |
| Loop Duration | Stack readback 없음 | NOT_VERIFIED |
| Mesh Source | module input readback 없음 | NOT_VERIFIED |
| Update Sprite Size | Stack readback 없음 | NOT_VERIFIED |
| Custom Alignment | Renderer readback 없음 | NOT_VERIFIED |
| Custom Facing Vector | Renderer readback 없음 | NOT_VERIFIED |
| Renderer Material | Renderer readback 없음 | NOT_VERIFIED |
| UV 연결 | 미검증 | NOT_VERIFIED |
| Normal 연결 | 미검증 | NOT_VERIFIED |
| Material Compile | error 조회 없음 | NOT_VERIFIED |
| Niagara Compile | errors/warnings [] | PASS |

보고서 실제 저장 경로: `D:\Unreal Projects\Capstone_Project\.memoc\niagara_simple_sk_remaining_report.md`
