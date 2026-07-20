# Distance Field Example Material 검수 결과

## 검수 요약

- 전체 상태: PARTIAL
- 예제 Material: `/Game/Niagara/Dissolve/M_DistField.M_DistField`
- Material Instance: `/Game/Niagara/Dissolve/MI_DistField.MI_DistField`
- Material Graph: 필수 6개 Expression과 Base Color까지의 실제 연결을 확인함.
- 사람이 직접 확인해야 하는 항목: Material Compile 메시지/Warning과 레벨에서의 Distance Field 시각 결과.
- Unreal 에셋 수정 여부: 수정하지 않음

`/Game/Niagara/Dissolve`의 `M_DistanceFieldExample` / `MI_DistanceFieldExample`도 별도 용도의 **방패 Static Mesh Dissolve 테스트** 에셋으로 존재한다. 이 쌍은 오류 후보가 아니라 본 기초 DF Mask 강좌의 대상과 다른 정상 에셋이다. Material의 Scalar Parameter는 `metalic`, `Roughness`, `AO`, `Normal`이고 Base Color 입력은 Texture Sample이었다. 요구한 DF Mask Graph와 Parent/Parameter 구성이 확인된 기초 예제 쌍은 `/Game/Niagara/Dissolve/M_DistField` / `MI_DistField`이다.

## 에셋 목록

| 역할 | 경로 | 클래스 | 존재 | 상태 |
|---|---|---|---|---|
| 확정 예제 Material | `/Game/Niagara/Dissolve/M_DistField.M_DistField` | `Material` | PASS — `.uasset` 파일과 에디터 로드 확인 | PASS |
| 확정 예제 Material Instance | `/Game/Niagara/Dissolve/MI_DistField.MI_DistField` | `MaterialInstanceConstant` | PASS — `.uasset` 파일과 에디터 로드 확인 | PASS |
| 방패 Dissolve Material | `/Game/Niagara/Dissolve/M_DistanceFieldExample.M_DistanceFieldExample` | `Material` | PASS — 별도 Static Mesh Dissolve 테스트용 | NOT_APPLICABLE |
| 방패 Dissolve Material Instance | `/Game/Niagara/Dissolve/MI_DistanceFieldExample.MI_DistanceFieldExample` | `MaterialInstanceConstant` | PASS — 위 방패 Material의 Instance | NOT_APPLICABLE |

- 확정 MI Parent: `/Game/Niagara/Dissolve/M_DistField.M_DistField` — PASS.
- Broken Reference: 두 확정 에셋이 EditorAssetLibrary로 정상 로드됨. 직접적인 Broken Reference 판별 API는 노출되지 않아 참조 그래프 전체는 NOT_VERIFIED.
- Dirty 상태: 현재 Editor Dirty Content Package 목록이 `[]` — PASS. 감사 시점에 저장되지 않은 Content Package는 발견되지 않음.

## Material 기본 설정

| 항목 | 목표 | 실제값 | 상태 | 근거 |
|---|---|---|---|---|
| Material Domain | Surface | `MD_SURFACE` | PASS | Material 속성 직접 조회 |
| Blend Mode | Opaque 또는 시각화에 적절 | `BLEND_OPAQUE` | PASS | Material 속성 직접 조회 |
| Shading Model | 강좌 요구 없음 | `MSM_DEFAULT_LIT` | PASS | Material 속성 직접 조회 |
| Base Color | 최종 DF 계산 연결 | `MaterialExpressionSaturate_0`가 Base Color Source | PASS | MaterialEditingLibrary 역추적 |
| Compile Error | 없음 | 현재 Compile Error 목록 조회 API 없음 | NOT_VERIFIED | 재컴파일/저장은 검수 범위에서 수행하지 않음 |
| Compile Warning | 없음 | 현재 Compile Warning 목록 조회 API 없음 | NOT_VERIFIED | 재컴파일/저장은 검수 범위에서 수행하지 않음 |

## Material Graph 노드

| 노드/파라미터 | 실제 Expression | 기본값 | 사용 여부 | 상태 |
|---|---|---|---|---|
| Distance To Nearest Surface | `MaterialExpressionDistanceToNearestSurface_0` (`MaterialExpressionDistanceToNearestSurface`) | 기본 World Position 입력 미연결 | `Subtract.A`로 연결 | PASS |
| Subtract | `MaterialExpressionSubtract_0` (`MaterialExpressionSubtract`) | Const A=1, Const B=1 (두 입력은 연결됨) | `Divide.A`로 연결 | PASS |
| Divide | `MaterialExpressionDivide_0` (`MaterialExpressionDivide`) | Const A=1, Const B=2 (두 입력은 연결됨) | Saturate 입력으로 연결 | PASS |
| Saturate | `MaterialExpressionSaturate_0` (`MaterialExpressionSaturate`) | 해당 없음 | Base Color로 연결 | PASS |
| DF Mask Start Distance | `MaterialExpressionScalarParameter_1` | 이름 `DFMaskStartDistance`, 값 `0.0`, Group `None`, Sort Priority `32` | `Subtract.B`로 연결 | PASS |
| DF Mask Falloff | `MaterialExpressionScalarParameter_0` | 이름 `DFMaskFalloff`, 값 `5.0`, Group `None`, Sort Priority `32` | `Divide.B`로 연결 | PASS |

- Material Expression 수는 6개이며, Base Color에서 역추적한 노드 수와 일치한다. 따라서 이 Material 내부에서 확인된 필수 Expression은 고아 노드가 아니다.
- Falloff 기본값은 `5.0`으로 0이 아니다 — PASS.

## Material Graph 연결

| From | To | 실제 연결 | 상태 | 근거 |
|---|---|---|---|---|
| `MaterialExpressionDistanceToNearestSurface_0` | `MaterialExpressionSubtract_0.A` | 확인됨 | PASS | MaterialEditingLibrary가 Subtract의 A upstream으로 반환 |
| `DFMaskStartDistance` | `MaterialExpressionSubtract_0.B` | 확인됨 | PASS | Subtract의 B upstream이 ScalarParameter_1이며 이름 직접 조회 |
| `MaterialExpressionSubtract_0` | `MaterialExpressionDivide_0.A` | 확인됨 | PASS | Divide의 A upstream으로 반환 |
| `DFMaskFalloff` | `MaterialExpressionDivide_0.B` | 확인됨 | PASS | Divide의 B upstream이 ScalarParameter_0이며 이름 직접 조회 |
| `MaterialExpressionDivide_0` | `MaterialExpressionSaturate_0` 입력 | 확인됨 | PASS | Saturate의 유일 입력 upstream으로 반환 |
| `MaterialExpressionSaturate_0` | Material Base Color | 확인됨 | PASS | Base Color Input Source가 Saturate_0 |

확인된 식은 다음과 동등하다.

`Saturate((DistanceToNearestSurface - DFMaskStartDistance) / DFMaskFalloff)`

## Material Instance

| 항목 | 실제값 | 상태 | 근거 |
|---|---|---|---|
| Parent | `/Game/Niagara/Dissolve/M_DistField.M_DistField` | PASS | MI `parent` 속성 직접 조회 |
| Start Distance 노출/Override | `DFMaskStartDistance = 30.0` | PASS | MI `scalar_parameter_values` 직접 조회 |
| Falloff 노출/Override | `DFMaskFalloff = 20.0` | PASS | MI `scalar_parameter_values` 직접 조회 |
| 추가 Override | `RefractionDepthBias = 0.0` | PASS | MI `scalar_parameter_values` 직접 조회 |

## 문제 또는 의심 사항

- `M_DistanceFieldExample` / `MI_DistanceFieldExample` 쌍은 방패 Static Mesh Dissolve 테스트용 별도 정상 에셋이다. Graph/Parameter 구조가 기초 DF Mask 강좌와 다르므로, 이 검수에서 대체 대상으로 사용하지 않았다.
- Material Compile Error/Warning은 현재 읽기 기능으로 신뢰성 있게 조회하지 못했다. 이는 설정 불일치가 아니라 검수 가시성 제한이다. Package Dirty 상태는 후속 재검수에서 `[]`로 확인됐다.

## 사람이 직접 확인할 항목

- Sphere에 `/Game/Niagara/Dissolve/MI_DistField`를 적용한다.
- Sphere Component/Details에서 `Affect Distance Field Lighting`을 비활성화한다.
- Cube가 Distance Field를 생성하는 Static Mesh인지 확인한다.
- Cube에 접근했을 때 Sphere의 가까운 영역이 검게 변하는지 확인한다.
- Cube를 멀리했을 때 Sphere가 흰색으로 돌아오는지 확인한다.
- MI의 `DFMaskStartDistance`를 변경해 영향 범위가 변하는지 확인한다.
- MI의 `DFMaskFalloff`를 변경해 전환 폭이 변하는지 확인한다.
- `M_DistField` Material Editor의 Stats/Compile Results에서 Error와 Warning이 없는지 확인한다.

보고서 실제 저장 경로: `D:\Unreal Projects\Capstone_Project\.memoc\distance_field_example_material_audit.md`
