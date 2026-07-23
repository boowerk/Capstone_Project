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
# Niagara Skeletal Mesh Sampling 완료 상태

## 기준 에셋

- Niagara System: `/Game/Niagara/Dissolve_SK/NS_Simple_SK`
- 작업용 Skeletal Mesh: `/Game/Niagara/Dissolve_SK/SKM_Manny_Simple_DissolveSample`
- Niagara Material: `/Game/Niagara/Dissolve_SK/M_Mannequin_Simple_SK`
- 원본 보관 Material: `/Game/Niagara/Dissolve_SK/M_UEFN_Mannequin_Default`

`NS_Simple_SK`는 BASELINE COMPLETE다. 이전 시험용 `/Game/Niagara/Dissolve_SK/NS_Sample_SkeletalMesh`는 이후 실제 작업 기준으로 사용하지 않는다.

## 완료된 기능

- Skeletal Mesh 표면 Mesh Sampling
- Mesh Animation/변형 추종
- GPU Compute Sim, Fixed Bounds, Infinite Emitter, Spawn Rate, Initialize Particle
- Initialize/Update Mesh Reproduction Sprite
- 작업용 Skeletal Mesh 및 CPU Access 처리
- Custom Alignment, Custom Facing Vector, `Particles.SpriteFacing`
- Mesh Texture UV와 Normal 처리
- 원형 Sprite Mask
- Curl Noise Force 기반 파티클 이탈
- Position/Velocity Lerp
- 원본 마네킹 색상 유지
- Particle Color 전환
- 파란 발광색 전환
- Particle Alpha Fade

## 검증 방식

- 사용자 Unreal Editor 수작업 확인
- Niagara Preview 시각 검증
- 메시 추종, Curl Noise 이탈, 원본 색상→파란 발광 전환, Alpha Fade 정상 동작 확인
- MCP Stack/Material pin readback 제한 때문에 사용자 직접 확인을 이 단계의 최종 근거로 사용

## 중요한 스택 순서

Particle Update 핵심 순서:

1. Particle State
2. Scale Color
3. Curl Noise Force
4. Solve Forces and Velocity
5. Update Mesh Reproduction Sprite
6. Lerp Particle Attributes
7. Dynamic Material Parameters

다음 순서로 임의 재배열 금지:

`Update Mesh Reproduction Sprite → Curl Noise Force → Solve Forces and Velocity`

현재 정상 관계:

`Curl Noise Force → Solve Forces and Velocity → Update Mesh Reproduction Sprite → Lerp Particle Attributes`

`Lerp Particle Attributes` 구성:

- Position A: Update Mesh Reproduction Sprite의 Mesh Position
- Position B: Solve Forces and Velocity의 Position
- Velocity A: Update Mesh Reproduction Sprite의 Mesh Triangle Velocity
- Velocity B: Solve Forces and Velocity의 Velocity
- Alpha: 시간 Curve

## 색상 및 페이드 기준

- Material Emissive: 원본 마네킹 Emissive와 Particle Color RGB를 Dynamic Material Parameter Alpha로 Lerp.
- Niagara: 수명 Curve 기반 Dynamic Material Parameter Alpha와 파란 계열 Particle Color.
- Opacity Mask: 원형 Sprite Mask와 Particle Color Alpha 결합. 필요 시 DitherTemporalAA 사용.
- Scale Color Alpha Curve로 수명 후반 Fade.

## 완료 판정

- Skeletal Mesh Sampling: COMPLETE
- Mesh Reproduction 위치 추종: COMPLETE
- Custom Facing/Alignment: COMPLETE
- Mesh Texture UV 및 Normal 처리: COMPLETE
- Curl Noise 기반 파티클 이탈: COMPLETE
- Position/Velocity Lerp: COMPLETE
- 원본 색상과 Particle Color 전환: COMPLETE
- 파란 발광색 전환: COMPLETE
- Particle Alpha Fade: COMPLETE
- `NS_Simple_SK`: BASELINE COMPLETE

## 다음 작업

Distance Field 기반 캐릭터 Dissolve를 진행한다.

- `NS_Simple_SK`를 직접 파괴적으로 수정하지 않는다.
- 필요 시 전용 복제 System/Material에서 작업한다.
- Distance Field Alpha를 `Lerp Particle Attributes` Alpha와 색상 전환용 Dynamic Material Parameter Alpha에 연결한다.
- 목표: 표면 근처 메시 추종→Force 이동, 원본 재질색→파란 Particle Color 전환.

실제 저장 경로: `D:\Unreal Projects\Capstone_Project\.memoc\niagara_skeletal_mesh_sampling_complete.md`
