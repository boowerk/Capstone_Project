# Distance Field 기초 예제 감사 보고서

## 1. 결론

- 전체 판정: PARTIAL — 에셋, Graph, MI, 현재 열려 있는 테스트 Actor 및 프로젝트 Distance Field 설정은 확인됐다. 컴파일 메시지와 Viewport 결과는 확인하지 못했다.
- Verified completion: **83% (24/29)**
  - 산정: 에셋 2 + Graph 노드/핀/출력 12 + MI 4 + 레벨 구성 3 + Distance Field 설정 3 = 24개 PASS. Compile 상태 1개와 시각 결과 4개는 PASS에 포함하지 않았다.
- Possible completion: **100% (29/29)**
  - 5개 미확인 항목은 FAIL 근거가 없고, 이 항목들을 뒷받침하는 Graph/MI/Actor 설정은 존재한다. 단, 이것은 시각적 성공 판정이 아니다.
- Visual verification: `VISUALLY_NOT_VERIFIED`
- 사용자가 직접 확인할 최소 항목:
  1. `M_DistField` Material Editor에서 Compile Results의 Error/Warning 유무.
  2. EventMap Viewport에서 Cube 접근 시 Sphere 마스크가 검게 변하는지.
  3. MI Start Distance/Falloff 변경에 따른 실제 외형 반응.

이번 감사에서는 Unreal 에셋, Level, Material, Material Instance, 프로젝트 설정을 수정하거나 저장하지 않았다.

## 2. 발견된 에셋

| 타입 | 경로 | 역할 | 판정 | 근거 |
|---|---|---|---|---|
| Material | `/Game/Niagara/Dissolve/M_DistField.M_DistField` | Distance Field 흑백 마스크 Master Material | PASS | `Material`, Surface/Opaque, `DistanceToNearestSurface` 및 6개 Graph Expression 직접 조회 |
| Material Instance | `/Game/Niagara/Dissolve/MI_DistField.MI_DistField` | 위 Master의 테스트 파라미터 Instance | PASS | `MaterialInstanceConstant`, Parent 및 Override 직접 조회 |
| Material | `/Game/Niagara/Dissolve/M_DistanceFieldExample.M_DistanceFieldExample` | 방패 Static Mesh Dissolve 테스트용 별도 Master | NOT_APPLICABLE | Base Color가 Texture Sample, Scalar Parameter가 `metalic/Roughness/AO/Normal`; 기초 DF Mask 대상과 역할이 다름 |
| Material Instance | `/Game/Niagara/Dissolve/MI_DistanceFieldExample.MI_DistanceFieldExample` | 위 방패 Dissolve Master의 Instance | NOT_APPLICABLE | Parent가 `M_DistanceFieldExample`; 기초 DF Mask Parameter Override 없음 |
| Object Redirector | `/Game/Niagara/Examples/MI_DistField` | 이전 경로 Redirector | NOT_APPLICABLE | Asset Registry Class가 `ObjectRedirector`; 실제 대상은 `/Game/Niagara/Dissolve/MI_DistField` |

## 3. Material Graph 검사

| 항목 | 판정 | 확인된 값 또는 참조 | 확인 방법 | 제한사항 |
|---|---|---|---|---|
| Material Domain | PASS | `MD_SURFACE` | Material 속성 직접 조회 | 없음 |
| Blend Mode | PASS | `BLEND_OPAQUE` | Material 속성 직접 조회 | 없음 |
| Base Color 출력 | PASS | `MaterialExpressionSaturate_0`가 Base Color Source | MaterialEditingLibrary | 없음 |
| Distance To Nearest Surface | PASS | `MaterialExpressionDistanceToNearestSurface_0` | Base Color에서 upstream 역추적 | 없음 |
| Subtract | PASS | `MaterialExpressionSubtract_0` | 같은 역추적 | 없음 |
| Divide | PASS | `MaterialExpressionDivide_0` | 같은 역추적 | 없음 |
| Saturate | PASS | `MaterialExpressionSaturate_0` | Base Color Source 직접 조회 | 없음 |
| Start Distance Scalar | PASS | `DFMaskStartDistance`, 기본값 `0.0`, Group `None`, Sort Priority `32` | Expression 속성 직접 조회 | 없음 |
| Falloff Scalar | PASS | `DFMaskFalloff`, 기본값 `5.0`, Group `None`, Sort Priority `32` | Expression 속성 직접 조회 | Falloff는 0이 아님 |
| Distance → Subtract A | PASS | `DistanceToNearestSurface_0 → Subtract_0.A` | `get_inputs_for_material_expression` | 없음 |
| Start → Subtract B | PASS | `DFMaskStartDistance → Subtract_0.B` | 같은 API와 Parameter Name | 없음 |
| Subtract → Divide A | PASS | `Subtract_0 → Divide_0.A` | 같은 API | 없음 |
| Falloff → Divide B | PASS | `DFMaskFalloff → Divide_0.B` | 같은 API와 Parameter Name | 없음 |
| Divide → Saturate | PASS | `Divide_0 → Saturate_0.Input` | 같은 API | 없음 |
| Saturate → Base Color | PASS | `Saturate_0 → Base Color` | Material Property Source 직접 조회 | 없음 |
| Compile Error/Warning | NOT_VERIFIED | 현재 Compile Result 읽기 API 없음 | `validate_assets`도 MCP에서 `NOT_IMPLEMENTED` | 재컴파일/저장은 감사 범위상 수행하지 않음 |
| 저장되지 않은 변경 | PASS | 현재 Dirty Content Package 목록 `[]` | `EditorLoadingAndSavingUtils.get_dirty_content_packages()` | 감사 시점 기준 |

직접 확인된 식:

`Saturate((DistanceToNearestSurface - DFMaskStartDistance) / DFMaskFalloff)`

## 4. Material Instance 검사

| 항목 | 판정 | 확인된 값 | 근거 |
|---|---|---|---|
| Parent | PASS | `/Game/Niagara/Dissolve/M_DistField.M_DistField` | `MI_DistField.parent` 직접 조회 |
| Start Distance 노출/Override | PASS | `DFMaskStartDistance = 30.0` | `scalar_parameter_values` 직접 조회 |
| Falloff 노출/Override | PASS | `DFMaskFalloff = 20.0` | `scalar_parameter_values` 직접 조회 |
| 추가 Override | PASS | `RefractionDepthBias = 0.0` | `scalar_parameter_values` 직접 조회 |
| 강좌 예시값 | PASS | Master Falloff `5`, MI Start `30`, MI Falloff `20` | Master/MI 값 직접 조회 |

## 5. 레벨 및 Actor 검사

| 항목 | 판정 | Actor/Level | 설정값 | 근거 |
|---|---|---|---|---|
| 테스트 Level | PASS | `/Game/Maps/EventMap/EventMap.EventMap` | 현재 Editor World | EditorLevelLibrary 직접 조회 |
| Cube 테스트 Actor | PASS | `StaticMeshActor_2` | Mesh `/Engine/BasicShapes/Cube.Cube`; 위치 `(280, 0, 350)`; Material `MI_DistField`; Affect DF Lighting `True` | StaticMeshComponent 속성 직접 조회 |
| Sphere 테스트 Actor | PASS | `StaticMeshActor_4` | Mesh `/Engine/BasicShapes/Sphere.Sphere`; 위치 `(200, 0, 350)`; Material `MI_DistField`; Affect DF Lighting `False` | StaticMeshComponent 속성 직접 조회 |
| 두 Actor 거리 | PASS | 같은 Y/Z, X 차이 `80` | 중심 위치 직접 조회 | Mesh 표면 간 실제 거리는 Scale/Bounds 미조회 |
| Sphere Material 할당 | PASS | `MI_DistField` | Sphere Component `get_materials()` | 없음 |
| Cube Distance Field 제공 설정 | PASS | `Affect Distance Field Lighting=True` | Cube Component 속성 직접 조회 | Mesh Distance Field 생성 자체는 전역 설정과 함께 해석 |
| Sphere 자기 DF 비활성화 | PASS | `Affect Distance Field Lighting=False` | Sphere Component 속성 직접 조회 | 실제 얼룩 제거 외형은 시각 미검증 |

## 6. 프로젝트 설정 검사

| 항목 | 판정 | 값 | 설정 파일 또는 근거 |
|---|---|---|---|
| Generate Mesh Distance Fields | PASS | `r.GenerateMeshDistanceFields=True` | `Project_Eden/Config/DefaultEngine.ini:12` |
| 관련 Renderer 설정 | PASS | 위 Renderer CVar 확인 | 같은 파일 |
| 재시작 필요 미적용 상태 | NOT_VERIFIED | 직접 조회 API 없음 | 현재 Config 값만 읽음 |

## 7. 시각 검증

`VISUALLY_NOT_VERIFIED`

이번 감사에서는 Viewport/Preview를 캡처하거나 Actor를 이동·파라미터를 변경하지 않았다. 따라서 다음은 PASS로 판정하지 않았다.

- Cube 근처 Sphere 영역의 검은 마스크
- Cube를 멀리했을 때 흰색 복귀
- Start Distance 변경 시 범위 변화
- Falloff 변경 시 전환 폭 변화
- Sphere 자기 Distance Field 얼룩 제거의 실제 화면 결과

## 8. 사용자가 Unreal Editor에서 직접 확인할 항목

- 대상 Material: `/Game/Niagara/Dissolve/M_DistField`
  - Material Editor의 **Stats / Compile Results**에서 Error와 Warning이 없는지 확인한다.
- 대상 Level: `/Game/Maps/EventMap/EventMap`
  - `StaticMeshActor_4` Sphere와 `StaticMeshActor_2` Cube를 Viewport에서 확인한다.
  - Cube에 가까운 Sphere 부분이 검게 변하고, 먼 부분이 흰색에 가까운지 확인한다.
  - `MI_DistField`의 `DFMaskStartDistance`를 `50`, `DFMaskFalloff`를 `40`으로 임시 변경했을 때 각각 영향 범위와 전환 폭이 증가하는지 확인한다. 이 감사에서는 값을 변경하지 않았다.

## 9. 최종 상태표

| 항목 | 상태 |
|---|---|
| Example Material | PASS |
| Material Instance | PASS |
| Distance To Nearest Surface | PASS |
| Subtract | PASS |
| Divide | PASS |
| Saturate | PASS |
| Start Distance Parameter | PASS |
| Falloff Parameter | PASS |
| 핵심 핀 연결 | PASS |
| 최종 Base Color 연결 | PASS |
| MI Parent | PASS |
| MI Start/Falloff Override | PASS |
| 테스트 Sphere | PASS |
| 테스트 Cube | PASS |
| MI 할당 | PASS |
| Generate Mesh Distance Fields | PASS |
| Sphere 자기 Distance Field 비활성화 | PASS |
| Cube Distance Field Lighting | PASS |
| Material Compile 상태 | NOT_VERIFIED |
| 거리 반응 | VISUALLY_NOT_VERIFIED |
| Start Distance 반응 | VISUALLY_NOT_VERIFIED |
| Falloff 반응 | VISUALLY_NOT_VERIFIED |
| 자기 간섭 제거 외형 | VISUALLY_NOT_VERIFIED |

보고서 실제 저장 경로: `D:\Unreal Projects\Capstone_Project\.memoc\distance_field_example_audit.md`
