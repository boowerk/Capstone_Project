# L_LandscapeMap 오염도·지역 이벤트 테스트 환경

## 실행 방법

1. 에디터에서 `/Game/Maps/MainMap/L_LandscapeMap`을 엽니다.
2. PIE를 시작합니다. 기존 지형/도로와 겹치지 않도록 테스트 데크와 `PlayerStart`는 월드 Z=5000에 배치되어 있습니다.
3. `P` 키를 누르면 초록색 NavMesh를 확인할 수 있습니다.
4. 바닥 표지 안으로 걸어 들어가 오염도 또는 이벤트를 실행합니다.

맵에는 `GP.TestEnvironment` 태그를 가진 액터가 정확히 12개 있습니다. 구성은 바닥 3개, 오염도 스테이션 3개, 이벤트 스테이션 4개, 안내판 1개, NavMesh Bounds 1개입니다.

## 배치

좌표는 `PlayerStart`의 X/Y를 원점으로 본 상대 좌표입니다.

| 구역 | 상대 좌표 | 색/표지 | 동작 |
| --- | ---: | --- | --- |
| 오염도 0 | (-2000, -1500) | 초록 | 전역 오염도를 정확히 0으로 설정 |
| 오염도 50 | (0, -1500) | 주황 | 전역 오염도를 정확히 50으로 설정 |
| 오염도 100 | (2000, -1500) | 보라 | 전역 오염도를 정확히 100으로 설정 |
| 붉은 균열 | (-7500, 3500) | `RED RIFT` | 몬스터 웨이브 시작 |
| 수정 오염 | (-2500, 3500) | `CRYSTAL CORRUPTION` | 이동 감속 및 파괴할 수정 생성 |
| 신전 잔해 | (2500, 3500) | `SHRINE RUINS` | 강화 선택 UI 요청 |
| 구조물 방어 | (7500, 3500) | `STRUCTURE DEFENSE` | 25초 방어 웨이브 시작 |

오염도 스테이션에 들어가면 수치 비교가 흐려지지 않도록 시간에 따른 자동 증가가 일시 정지됩니다. HUD·미니맵·하늘·안개 색과 적 GAS 능력치를 비교하세요. 기본 설정에서 오염도 100인 적은 `DamageIncreaseRate +0.5`, `Armor +50`을 받습니다. 초기 상태로 완전히 돌아가거나 자동 증가를 다시 확인하려면 PIE를 재시작하는 것이 가장 빠릅니다.

## 이벤트별 확인 항목

### 붉은 균열

- 시작 즉시 근접 2마리와 원거리 1마리가 생성됩니다.
- 약 2초와 8초에 같은 구성이 추가되어 최대 9마리가 생성됩니다.
- 현재 예제는 처치 수와 무관하게 40초가 지나면 `Expired`로 종료됩니다.

### 수정 오염

- 반경 520에 수정 3개가 생성됩니다.
- 반경 1400 안의 플레이어 이동 속도는 55%로 감소합니다.
- 수정 하나는 3회 타격하면 파괴되고, 3개를 모두 파괴하면 감속이 해제되며 완료됩니다.
- 현재 예제에는 자동 시간 제한이 없습니다.

### 신전 잔해

- 반경 850 안에 들어온 첫 유효 플레이어에게 강화 선택 UI를 요청하고 완료됩니다.
- 플레이어가 이벤트 생성 시점에 이미 범위 안에 있어도 다음 틱에 다시 확인합니다.
- `GP_PlayerController`의 `AugmentPoolData`와 `AugmentSelectWidgetClass`가 비어 있으면 UI가 열리지 않습니다.
- 3인 PIE에서도 현재 예제는 첫 플레이어 한 명이 선택권을 얻는 구조입니다.

### 구조물 방어

- 시작 즉시 근접 2마리와 원거리 1마리가 생성됩니다.
- 8초, 16초, 24초에도 같은 웨이브가 생성되어 최대 12마리가 됩니다.
- 25초를 버티면 완료됩니다.
- 현재 예제 구조물에는 HP·파괴 실패·범위 이탈 실패 조건이 없습니다.

맵에 배치된 네 이벤트 인스턴스는 `BeginPlay` 자동 실행이 꺼져 있고 플레이어 오버랩 실행만 켜져 있습니다. `/Game/RegionEvents/Examples`의 개별 테스트 BP를 다른 맵에 직접 배치하면 기본값은 `BeginPlay` 자동 실행입니다.

일부 예제는 완료 후 남은 적이나 지역 상태를 자동 정리하지 않습니다. 각 이벤트를 독립적으로 확인할 때는 이벤트마다 PIE를 재시작하는 것을 권장합니다. 같은 스테이션을 다시 시험할 때는 트리거 범위를 완전히 벗어난 뒤 재진입하세요.

## 문제 확인

- 적이 생성되지 않으면 `P`로 스테이션 주변 NavMesh를 확인합니다.
- Output Log에서 `[CorruptionTest]`, `[RegionEventTest]`, `[RegionEvent]`를 검색합니다.
- `navmesh projection failed`가 보이면 해당 위치에 이어진 NavMesh가 없는 상태입니다.
- 신전만 반응하지 않으면 `GP_PlayerController`의 강화 데이터와 위젯 클래스를 먼저 확인합니다.

## 환경 재생성과 자동화

프로젝트 루트에서 아래 명령을 실행하면 관리 대상 액터만 찾아 같은 위치로 갱신합니다. 디자이너가 만든 다른 맵 액터는 삭제하지 않습니다.

```powershell
& 'C:\Users\boowerk\Downloads\Engine\Windows\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\Capstone_Project\Project_Eden\Project_Eden.uproject' -run=GP_CreateLandscapeTestEnvironment -unattended -nop4 -nosplash -nullrhi
```

회귀 테스트 필터:

- `ProjectEden.Game.LandscapeTestEnvironment`
- `ProjectEden.Game.RegionEvents`
- `ProjectEden.Game.Corruption`

맵 생성 명령은 저장 전에 `PlayerStart`에서 네 이벤트 스테이션까지 완전한 NavMesh 경로가 있는지 검사합니다. 실패 단계도 `[LandscapeTestEnvironment]` 로그로 구분됩니다.
