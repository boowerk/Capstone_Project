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
# Distance Field Dissolve 에셋 준비 결과

## 전체 상태

- 상태: PASS
- Unreal 에셋 수정 범위: `/Game/Niagara/Dissolve_SK/DistanceField` 폴더 생성, `NS_DistanceField_Dissolve` 및 `M_UEFN_Mannequin_DistanceField` 복제·저장만 수행.
- 원본 보호 여부: PASS — 원본을 수정·이동·이름 변경·저장하지 않았다. 재조회에서 모든 보호 원본이 기존 경로/클래스로 존재하고, 현재 Dirty Content Package 목록은 `[]`다.
- 수동 작업 필요 여부: 예. 다음 단계의 Material Graph, MI 파라미터/할당, Niagara Scratch Pad 및 DF Alpha 연결은 이번 범위에서 수행하지 않았다.

## 기준 Skeletal Mesh

- 경로: `/Game/Niagara/Dissolve_SK/SKM_Manny_Simple_DissolveSample.SKM_Manny_Simple_DissolveSample`
- Material Slot 개수: `1`

| Slot Index | Slot Name | 할당 에셋 | 클래스 | Parent |
|---|---|---|---|---|
| 0 | `lambert1` | `/Game/Characters/UEFN_Mannequin/Materials/M_UEFN_Mannequin.M_UEFN_Mannequin` | `Material` | 해당 없음 — Material 직접 할당 |

Slot이 Material Instance가 아니라 Material을 직접 참조하므로, 이번 준비 단계에서 복제할 MI와 Parent 재지정 작업은 발생하지 않았다.

## Niagara 복제

| Source | Destination | 존재 | 클래스 | 상태 |
|---|---|---|---|---|
| `/Game/Niagara/Dissolve_SK/NS_Simple_SK.NS_Simple_SK` | `/Game/Niagara/Dissolve_SK/DistanceField/NS_DistanceField_Dissolve.NS_DistanceField_Dissolve` | PASS — 재로드 확인 | `NiagaraSystem` | PASS |

복제본의 Niagara Stack, 모듈, 입력값은 이번 작업에서 조회·변경하지 않았다.

## Surface Material 복제

| 역할 | Source | Destination | 클래스 | 상태 |
|---|---|---|---|---|
| Slot 0 Surface Material 직접 복제 | `/Game/Characters/UEFN_Mannequin/Materials/M_UEFN_Mannequin.M_UEFN_Mannequin` | `/Game/Niagara/Dissolve_SK/DistanceField/M_UEFN_Mannequin_DistanceField.M_UEFN_Mannequin_DistanceField` | `Material` | PASS — 재로드 및 저장 확인 |

같은 Source Material을 참조하는 추가 Slot은 없으므로 복제본은 하나만 만들었다.

## Material Instance Parent

| 복제 MI | 목표 Parent | 실제 Parent | 상태 |
|---|---|---|---|
| 해당 없음 | 해당 없음 — 기준 Skeletal Mesh Slot이 Material을 직접 사용 | 해당 없음 | PASS — MI Parent 변경 불필요 |

## 보호 대상 확인

| 원본 에셋 | 현재 경로 | 존재 | 변경 여부 확인 | 상태 |
|---|---|---|---|---|
| `NS_Simple_SK` | `/Game/Niagara/Dissolve_SK/NS_Simple_SK.NS_Simple_SK` | PASS, `NiagaraSystem` | 본 작업에서 원본 write 없음; 재로드/Dirty 목록 `[]` | PASS |
| `M_Mannequin_Simple_SK` | `/Game/Niagara/Dissolve_SK/M_Mannequin_Simple_SK.M_Mannequin_Simple_SK` | PASS, `Material` | 본 작업에서 원본 write 없음; 재로드/Dirty 목록 `[]` | PASS |
| `SKM_Manny_Simple_DissolveSample` | `/Game/Niagara/Dissolve_SK/SKM_Manny_Simple_DissolveSample.SKM_Manny_Simple_DissolveSample` | PASS, `SkeletalMesh` | 본 작업에서 원본 write 없음; 재로드/Dirty 목록 `[]` | PASS |
| `M_DistField` | `/Game/Niagara/Dissolve/M_DistField.M_DistField` | PASS, `Material` | 본 작업에서 원본 write 없음; 재로드/Dirty 목록 `[]` | PASS |
| `MI_DistField` | `/Game/Niagara/Dissolve/MI_DistField.MI_DistField` | PASS, `MaterialInstanceConstant` | 본 작업에서 원본 write 없음; 재로드/Dirty 목록 `[]` | PASS |
| `M_DistanceFieldExample` | `/Game/Niagara/Dissolve/M_DistanceFieldExample.M_DistanceFieldExample` | PASS, `Material` | 본 작업에서 원본 write 없음; 재로드/Dirty 목록 `[]` | PASS |
| `MI_DistanceFieldExample` | `/Game/Niagara/Dissolve/MI_DistanceFieldExample.MI_DistanceFieldExample` | PASS, `MaterialInstanceConstant` | 본 작업에서 원본 write 없음; 재로드/Dirty 목록 `[]` | PASS |
| `M_UEFN_Mannequin` | `/Game/Characters/UEFN_Mannequin/Materials/M_UEFN_Mannequin.M_UEFN_Mannequin` | PASS, `Material` | 복제 Source로만 읽음; 재로드/Dirty 목록 `[]` | PASS |

## 사용자가 다음에 수동으로 할 작업

- Distance Field Material Graph 구성
- Material Instance Parameter 설정
- Skeletal Mesh 또는 Actor의 Material Slot 할당
- Niagara Scratch Pad 구성
- Distance Field Alpha 연결

## 문제 또는 제한 사항

- `manage_asset.duplicate_asset`를 사용한 Niagara 복제 요청은 응답 대기 제한(25초)에 도달했고, 재조회 결과 목적지 에셋이 존재하지 않았다. 이 호출은 성공으로 처리하지 않았다.
- 동일 작업을 Unreal Editor Python의 `EditorAssetLibrary.duplicate_asset`으로 안전하게 재시도했으며, 목적지 `NiagaraSystem` 재로드와 저장을 확인했다.
- 기준 Skeletal Mesh에 Material Instance Slot이 없어, 복제 MI Parent를 `_DistanceField` Material로 재지정하는 작업은 필요하지 않았다.

보고서 실제 저장 경로: `D:\Unreal Projects\Capstone_Project\.memoc\distance_field_dissolve_asset_preparation.md`
