# Project Eden 멀티 스레드 개발 운영 규칙

## 목적

현재 Codex 메인 스레드를 총괄·통합 책임자로 두고, 전문 하위 스레드가
읽기 전용 분석과 교차 검토를 수행한다. 실제 파일 수정, 검증, 스테이징,
커밋은 메인 스레드만 담당한다.

## 상시 역할

| 스레드 | 모델 | 책임 |
| --- | --- | --- |
| `[EDEN-MAIN]` | 현재 메인 스레드 | 우선순위, 최종 결정, 구현, 검증, 커밋 |
| `[EDEN-DESIGN]` | `gpt-5.6-terra / high` | 게임 루프, 밸런스, 레벨·UX 명세와 수용 기준 |
| `[EDEN-CLIENT]` | `gpt-5.6-sol / high` | Unreal C++·Blueprint·GAS·네트워크 영향 분석 |
| `[EDEN-WORLD]` | `gpt-5.6-terra / high` | 월드·PCG·UI·애니메이션·VFX 완성도와 에셋 소유권 |
| `[EDEN-QA]` | `gpt-5.6-terra / high` | 회귀, PIE·멀티플레이, Cook·패키징, 릴리스 게이트 |

AI·GAS·네트워크·아키텍처처럼 고위험 전문 판단이 필요한 티켓은
`gpt-5.6-sol / xhigh` 임시 전문가 스레드를 추가한다. 대규모 로그 분류나
단순 현황 조사는 `gpt-5.6-luna / medium` 임시 스레드로 분리할 수 있다.

## 권한

- 메인 스레드만 소스, 문서, 설정, 바이너리 에셋을 수정한다.
- 하위 스레드는 명시적 위임 전까지 파일 수정, 빌드, 에디터 조작,
  스테이징, 커밋을 하지 않는다.
- 하위 스레드의 승인은 전문 검토 완료를 뜻하며 변경 권한을 뜻하지 않는다.
- 메인이 승인된 명세를 확정한 뒤 구현한다.
- 코드 추가 또는 수정 시 의도와 제약을 설명하는 주석을 남긴다.

## 티켓 수명주기

```text
BACKLOG
→ BRIEFING
→ DISCOVERY
→ PROPOSED
→ CROSS_REVIEW
→ MAIN_APPROVED
→ IMPLEMENTING
→ VERIFYING
→ COMMITTED
→ ACCEPTED
→ DOCUMENTED/CLOSED
```

보조 상태는 `BLOCKED`, `SUPERSEDED`, `ROLLBACK`을 사용한다. 티켓 ID는
`EDEN-YYYYMMDD-NNN` 형식이다.

각 티켓은 다음 내용을 포함한다.

- 목표와 사용자 가치
- 포함 범위와 제외 범위
- 기준 Git SHA와 알려진 미커밋 변경
- 영향 파일과 보호 파일
- 담당 스레드와 검토 스레드
- 수용 기준과 검증 방법
- 의존성, 위험, 다음 행동
- 승인된 명세 리비전

## 스레드 간 대화

하위 스레드의 컨텍스트는 자동 공유되지 않으므로 메인 스레드가 질문과
결과를 중계한다. 교차 검토는 기본 두 번의 왕복으로 제한하며, 합의되지
않으면 메인이 근거와 트레이드오프를 비교해 결정한다.

```text
[MSG v1]
message_id: EDEN-20260718-001/DESIGN/03
type: TASK | QUESTION | REVIEW | DECISION | BLOCKER | RESULT
ticket: EDEN-20260718-001
from: MAIN
to: DESIGN
reply_to:
snapshot:

goal:
scope:
facts_and_evidence:
proposal:
alternatives_and_tradeoffs:
risks:
requested_action:
acceptance_criteria:
```

하위 스레드의 결과는 `결론 / 근거 / 가정 / 영향 파일 / 위험 / 검증법 /
남은 질문 / 확신도`를 포함한다.

## 작업 및 커밋 규칙

모든 구현 작업은 독립적으로 검토하고 되돌릴 수 있는 최소 기능 단위로
쪼갠다.

- 원칙적으로 `티켓의 구현 하위 단위 1개 = 기능 커밋 1개`로 만든다.
- 하나의 티켓이 여러 계층을 건드리면 의존 순서대로 하위 단위와 커밋을
  나눈다.
- 기능 동작과 그 동작을 보장하는 테스트는 같은 커밋에 포함한다.
- 관련 없는 리팩터링, 포맷 변경, 에셋 수정은 기능 커밋에 섞지 않는다.
- 커밋 전 `git status`와 대상 diff를 다시 확인하고 파일 경로를 명시해
  스테이징한다.
- 가능한 경우 각 커밋은 자체적으로 빌드되거나 해당 범위 테스트를
  통과해야 한다.
- 불가피하게 여러 기능을 한 커밋에 묶어야 하면 티켓에 이유와 분리 불가
  근거를 기록한다.
- 메인 스레드는 검증 결과를 확인한 뒤에만 커밋한다.

커밋 제목은 저장소 기록에 맞춰 `type(scope): short summary` 형식을 쓴다.

- `feat`: 사용자에게 보이는 새 기능
- `fix`: 결함 수정
- `test`: 테스트 또는 회귀 방지
- `content`: 맵·DataAsset·콘텐츠 에셋
- `docs`: 문서와 프로젝트 메모리
- `refactor`: 동작을 유지하는 구조 개선
- `chore`: 빌드·도구·유지보수

## 사용자 재승인 게이트

다음 작업은 하위 스레드와 메인 합의만으로 진행하지 않고 사용자에게 다시
확인한다.

- 핵심 게임 루프 또는 졸업작품 범위 변경
- 큰 아키텍처 교체나 일정에 영향을 주는 변경
- 기존 사용자 작업을 덮어쓸 가능성이 있는 변경
- 삭제, 외부 배포, 게시처럼 되돌리기 어려운 작업
- 마일스톤 결과의 최종 채택

## 보호 대상

2026-07-18 기준 다음 파일은 사용자 작업으로 보고 명시적 허락 없이
수정하거나 스테이징하지 않는다.

- `Project_Eden/Content/Maps/DemoMap/TestMap.umap`
- `Project_Eden/Content/Maps/MainMap/DA_RegionEventData.uasset`
- `Project_Eden/Content/Maps/MainMap/L_LandscapeMap.umap`
- `Project_Eden/Content/Maps/MainMap/L_MainMap.umap`

## 현재 게임 흐름 경계

2026-07-24 사용자 결정으로 고정 데모 흐름은 제거됐다. 현재 기본 진입은
다음과 같으며, 새로운 자동 진행을 추가하려면 별도 티켓과 사용자 승인이
필요하다.

```text
메인/로비
→ 3인 Ready 및 L_LandscapeMap 이동
→ authored PlayerStart 주변의 안전한 3인 시작
→ 자유 탐험
```

`L_LandscapeMap`에는 일반 Zone이 없으므로 자동 목표·이벤트·보스·승리 전환이
발생하지 않는다. 일반 Zone/Portal/결과 코드는 유지되며, 실제 진행형 맵에
사용하려면 Zone을 저작하거나 목적 맵을 변경하는 별도 작업으로 다룬다.
