# Skeletal Mesh Niagara 전체 검수 결과

## 1. 검수 요약

- 전체 상태: INSUFFICIENT_VISIBILITY
- 실제 검수 가능: 저장 파일 존재, 파일 크기/저장 시각, 직렬화된 Script·Function 참조, Skeletal Mesh Skeleton/원본 Material 참조.
- MCP에서 읽지 못함: Niagara Stack 순서·Usage·입력값·Renderer Properties·Parameter Writes, Material Texture UV/Normal pin 연결, Compile Message/시각 결과.
- 실제 오류 또는 불일치: 현재 읽기 범위에서 확인 못함.
- 에셋 수정 여부: 수정하지 않음.

## 2. MCP 검수 능력 평가

| 검수 영역 | 읽기 가능 여부 | 사용한 방법 | 신뢰도 | 한계 |
|---|---|---|---|---|
| Asset 존재 | 가능 | 저장 `.uasset` | 높음 | Class reflection 미실행 |
| Skeletal Mesh 설정 | 부분 | 직렬화 참조 | 중간 | CPU Access/LOD/slot 세부 불가 |
| Niagara 기본 설정 | 부분 | 저장 System 파일 | 낮음 | GPU/Loop/Rate 값 불가 |
| Niagara Stack 모듈 | 부분 | Script 참조 문자열 | 중간 | Stack instance/순서/Usage 불가 |
| Niagara Module 입력 | 불가 | 없음 | 낮음 | Mesh/size/parameter 불가 |
| Sprite Renderer | 불가 | 없음 | 낮음 | 전 항목 불가 |
| Material 기본 설정 | 부분 | 저장 Graph 참조 | 중간 | Domain/Usage 직접 조회 미실행 |
| Material 노드 | 부분 | 직렬화 참조 | 중간 | 전체 node 목록 불가 |
| Material 핀 연결 | 불가 | 없음 | 낮음 | UV/Normal 연결 불가 |
| Compile 결과 | 불가 | 없음 | 낮음 | 이번 검수에서는 compile 호출 안 함 |
| 시각 결과 | 불가 | Preview/PIE 미실행 | 낮음 | 시각 검수 없음 |

## 3. 에셋 검수

- `/Game/Niagara/Dissolve_SK/NS_Simple_SK.NS_Simple_SK`: 저장 파일 존재, 534,505 bytes.
- `/Game/Niagara/Dissolve_SK/SKM_Manny_Simple_DissolveSample.SKM_Manny_Simple_DissolveSample`: 저장 파일 존재, 2,417,037 bytes. `SK_UEFN_Mannequin` Skeleton 및 `M_UEFN_Mannequin` 참조 확인.
- `/Game/Niagara/Dissolve_SK/M_Mannequin_Simple_SK.M_Mannequin_Simple_SK`: 저장 파일 존재, 18,869 bytes. Mesh Reproduction UV Function·SphereMask 참조 확인.
- `/Game/Niagara/Dissolve_SK/M_UEFN_Mannequin_Default.M_UEFN_Mannequin_Default`: 저장 파일 존재, 15,293 bytes. 원본 Material 참조 확인.
- 원본 에셋 및 `NS_Sample_SkeletalMesh`는 이번 검수에서 수정하지 않음.

## 4. Niagara 기본 설정

| 항목 | 목표값 | 실제 확인값 | 상태 | 근거 |
|---|---|---|---|---|
| 대상 System | NS_Simple_SK | 저장 파일 존재 | PASS | 파일 경로 |
| GPU Compute Sim | 활성화 | 읽지 못함 | NOT_VERIFIED | Stack/Emitter property 불가 |
| Fixed Bounds | 활성화 | 읽지 못함 | NOT_VERIFIED | 동상 |
| Loop Behavior | Infinite | 읽지 못함 | NOT_VERIFIED | 동상 |
| Loop Duration | 5 | 읽지 못함 | NOT_VERIFIED | 동상 |
| Spawn Burst | 없음 | 읽지 못함 | NOT_VERIFIED | 동상 |
| Spawn Rate | 20000 | 읽지 못함 | NOT_VERIFIED | 동상 |
| Lifetime | 5 | 읽지 못함 | NOT_VERIFIED | 동상 |
| Initialize Sprite Size | 2 | 읽지 못함 | NOT_VERIFIED | 동상 |

## 5. 실제 Niagara Stack

실제 Stack 순서/Usage/입력은 현재 MCP가 읽지 못한다. 저장 System 파일에 다음 Script 참조가 존재한다.

- `/Niagara/Modules/Spawn/MeshInterface/Initialize_MeshReproductionSprite`
- `/Niagara/Modules/Update/MeshInterface/Update_MeshReproductionSprite`

직렬화 Script 참조는 Stack 인스턴스 위치·활성 상태·입력값의 PASS 근거가 아니다.

## 6. Mesh Reproduction 검수

| 항목 | 목표 | 실제 확인값 | 상태 | 근거 |
|---|---|---|---|---|
| Initialize 모듈 | Particle Spawn | Script 참조 존재 | PARTIAL | Stack Usage 미확인 |
| Update 모듈 | Particle Update | Script 참조 존재 | PARTIAL | Stack Usage 미확인 |
| Initialize Mesh Source | 작업용 Mesh | 읽지 못함 | NOT_VERIFIED | module input 불가 |
| Update Mesh Source | 작업용 Mesh | 읽지 못함 | NOT_VERIFIED | module input 불가 |
| 동일 Mesh Source | 동일 | 읽지 못함 | NOT_VERIFIED | 동상 |
| Update Sprite Size | 약 5 | 읽지 못함 | NOT_VERIFIED | 동상 |
| CPU Access Error | 없음 | 읽지 못함 | NOT_VERIFIED | error stack 불가 |

## 7. Renderer 및 Facing 검수

Renderer 수, Alignment, Facing Mode, Renderer Material, `Particles.SpriteFacing`, Facing 중복 작성은 모두 NOT_VERIFIED. Renderer Stack Properties readback 미지원.

## 8. Material 검수

| 항목 | 목표 | 실제 확인값 | 상태 | 근거 |
|---|---|---|---|---|
| Domain | Surface | 읽지 못함 | NOT_VERIFIED | Property read 미실행 |
| Blend Mode | Masked | SphereMask 참조 | PARTIAL | Blend enum 직접 미조회 |
| Niagara Sprite Usage | 활성화 | 읽지 못함 | NOT_VERIFIED | usage flag 미조회 |
| Mesh Reproduction Function | Graph 존재 | Function 참조 존재 | PARTIAL | FunctionCall node/핀 미확인 |
| Base Color UV | Function UV | 읽지 못함 | NOT_VERIFIED | pin readback 불가 |
| Normal Texture UV | Function UV | 읽지 못함 | NOT_VERIFIED | pin readback 불가 |
| Mesh Reproduction Normal | 결합 | 읽지 못함 | NOT_VERIFIED | pin readback 불가 |
| 원형 Mask | Opacity Mask 연결 | SphereMask 참조 | PARTIAL | Output pin 미확인 |
| Material Compile | 오류 없음 | 읽지 못함 | NOT_VERIFIED | compile log 미조회 |

## 9. 불필요하거나 의심스러운 구성

읽을 수 있는 범위에서 중복/고아 module, renderer, DI, example movement module을 확인할 수 없었다. 추측해 기록하지 않음.

## 10. 시각 검수

- 수행 여부: 미수행.
- 이유: Preview/PIE를 수정 없이 읽는 안전한 MCP readback 없음.

## 11. 실제 수정이 필요한 항목

실제 목표값과 다른 항목을 읽지 못했으므로 FAIL 판정·수정 필요 항목 없음.

## 12. 사람이 직접 확인해야 하는 항목

- `NS_Simple_SK` Niagara Stack: GPU/Bounds/Loop/Spawn/Lifetime/Size, module Usage/입력/순서, Renderer Alignment/Facing/Material, SpriteFacing writes.
- `M_Mannequin_Simple_SK` Material Editor: Domain/Blend/Usage, FunctionCall node, Base/Normal UV pins, Normal 결합, SphereMask→Opacity Mask, compile 결과.
- Niagara Preview/PIE: 실제 표면 샘플링·추종·방향·원형·텍스처 시각 결과.

보고서 실제 저장 경로: `D:\Unreal Projects\Capstone_Project\.memoc\niagara_skeletal_mesh_full_audit.md`
