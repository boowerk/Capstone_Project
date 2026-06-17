---
memoc: true
type: core
scope: project-memory
created: 2026-06-17T07:46:35
updated: 2026-06-17T07:46:35
status: active
tags:
  - memoc
  - memoc/core
---
# Project Brief

This is the shortest project summary for a fresh agent. Keep it factual and easy to scan.

## Identity

<!-- memoc:identity:start -->
- Project name: `Capstone_Project`
- Detected stack: Not detected
<!-- memoc:identity:end -->

## Current Direction

3D 액션 RPG 로그라이크. 핵심 코어 루프는 다음 세 가지로 구성한다:
1. 스킬 빌드 + 증폭(Augment) 로그라이크: 적 처치 → XP/레벨업 → 증폭 선택으로 빌드를 키워가며 점점 강해짐.
2. 보스 러시: Crystal Seraph 같은 패턴 보스 공략이 한 판의 목표. 기본 적은 보스로 가는 길의 장애물.
3. 코업(Co-op) 협동: 친구와 함께 적/보스를 잡는 멀티플레이 지원 (데디케이티드 서버 대응 작업이 이를 뒷받침).

진행 흐름(확정 2026-06-17): **선형 도시 진행** — `[도시] 지정 적 전부 처치 → [보스룸] 보스 처치 → [다음 도시] → ...`. 적은 지정된 도시(District) 안에서만 스폰(주변 지속 스폰 아님), 진행은 팀이 정한 한 방향. 정화는 코어 메커닉 아님 — Region State는 "도시 클리어 결과로 주변 식생이 약간 변하는" 시각 피드백으로만 활용. 보스룸은 텔포/부지예약 둘 다 가능(슬라이스는 텔포 권장, 결정 보류). 자기장 안은 폐기. 상세 설계·NavMesh 주의는 [[pcg-region-system-and-gameplay-flow]] 참조.

폐기된 방향: **원소(Element) 시스템은 폐기됨** (Pyros/Hydro/Volt 등 원소 전환/게이트는 더 이상 코어 디자인이 아님). 코드/메모리에 남은 원소 잔재(`RequiredElementTag`/`GrantedElementTag`, Tech 위젯 원소 버튼)는 정리 대상.

## How To Approach

- Start from `session-summary.md`; search before opening more files.
- Open status, handoff, rules, map, project wiki, or knowledge wiki only when the task needs them.
- After durable work, update the smallest relevant memory set.
- Do not treat generated output folders as source unless the user explicitly asks.

## Next Useful Work

최우선 목표: **플레이 가능한 수직 슬라이스(Vertical Slice)** — "적 처치 → 레벨업 → 증폭 선택 → 보스전"까지 한 판이 끊김 없이 돌아가는 완결된 플레이 흐름을 먼저 만든다. 새 기능 추가보다 기존 시스템을 슬라이스로 엮어 실제 플레이되게 하는 것이 우선.

수직 슬라이스에 필요한 작업 순서:
1. 원소 시스템 잔재 정리/비활성화 (증폭 원소 게이트, Tech 위젯 원소 버튼) — 디자인 방향과 코드 일치시키기.
2. 기본 적 BP 3종 PIE 검증 (근거리/원거리/비행 추격·공격).
3. 증폭 선택 UI 흐름 검증 (적 처치 → XP → 레벨업 → 증폭 선택).
4. Crystal Seraph 보스 BP 자식/BT/아레나 배치 완성 → 보스전 진입 가능하게.
5. 위 단계를 하나의 맵에서 처음부터 끝까지 플레이 테스트.

## Important Notes

_None yet._
