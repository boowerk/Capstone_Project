# Project Eden

> 3명의 플레이어가 오염된 세계를 탐험하고 함께 성장하는 Unreal Engine 5 협동 액션 프로젝트

`Project Eden`은 전투, 스킬 조합, 지역 탐험을 하나의 멀티플레이 흐름으로 연결한 3인 팀 캡스톤 프로젝트입니다.
서버 권한형 전투와 진행 상태, 절차적 월드 구성, 보스 AI, UI, 전용 서버 배포 과정을 C++과 Blueprint로 구현했습니다.

## 프로젝트 정보

| 항목 | 내용 |
| --- | --- |
| 개발 형태 | 3인 팀 프로젝트 |
| 개발 기간 | 2025.10 ~ 2026.07 |
| 장르 | 3인 협동 액션 |
| 엔진 | Unreal Engine 5.7.2 기반 팀 커스텀 빌드 (`ProjectEden_Engine`) |
| 플랫폼 | Windows Client / Dedicated Server |
| 주요 기술 | C++, Blueprint, GAS, UMG, PCG, Niagara, Enhanced Input |
| 김도윤 | 게임 진행·PCG/Region 런타임·플레이어 시스템·멀티플레이·빌드 자동화 |

## 게임 개요

플레이어는 3인 파티를 구성해 넓은 랜드스케이프를 탐험하고 적과 전투합니다.
전투에서 획득한 경험치로 성장하며 스킬과 증강을 선택하고,
지역별 전투와 보스전을 거쳐 오염된 지역을 되살리는 흐름을 목표로 개발했습니다.

## 팀 구현 기능

### 3인 멀티플레이

- 최대 3명으로 고정된 세션과 Ready 기반 로비 시작 조건
- Main Menu에서 Lobby에 접속하고, 3명 전원 Ready 후 Seamless Travel로 Gameplay Map에 이동
- 서버 권한형 전투 판정, 적 생성, 진행 상태 및 결과 처리
- 클라이언트 접속 실패·호스트 종료 시 메인 메뉴 복귀
- 전용 서버와 패키징 클라이언트를 이용한 로컬·원격 접속 흐름

### 전투와 성장

- Gameplay Ability System 기반 능력치, 피해, 쿨다운, Gameplay Tag 구성
- 투사체, 범위 공격, 지면 지정, 지속 피해 등 서로 다른 스킬 실행 방식
- Data Asset 기반 스킬 정보와 두 개의 장착 슬롯
- 경험치·레벨업, 스킬 선택, 원소 선택 및 증강 효과
- 피해량·범위·사거리·쿨다운·투사체 수를 바꾸는 증강 데이터
- 근접·원거리 적과 여러 보스의 공격 패턴, 피격, 그로기, 사망 처리

### 월드와 진행

- Region 상태에 따라 지형·식생 표현을 갱신하는 PCG 기반 월드 구성
- Run Seed를 이용한 마을 프리셋 선택과 멀티플레이 레이아웃 동기화
- Outer → Middle → Center → Colosseum 단계용 진행 시스템
- 구역 진입, 적 처치, 포털 통과와 파티 집결 조건을 서버에서 판정
- 런 승리·패배, 탈락 플레이어 관전과 로비 복귀를 위한 상태 처리

### UI와 표현

- 메인 메뉴, 3인 로비, 로딩 화면
- 체력·경험치·스킬 슬롯·쿨다운 HUD
- 스킬·증강·원소 선택 화면과 캐릭터 능력치 화면
- 미니맵, 데미지 숫자, 적 체력바
- 탈락·승리·패배 결과 화면
- Niagara 기반 스킬·피격·보스 전투 VFX

## 현재 실행 흐름

```text
MainMenuMap
  └─ Host / IP Join
      └─ LobbyMap
          └─ 3명 접속 + 전원 Ready
              └─ L_LandscapeMap
                  └─ 자유 탐색 / 전투 / 성장
```

소스에는 `Outer → Middle → Center → Colosseum → Victory/Defeat` 진행 프레임워크와 테스트가 있습니다.
다만 최신 `main`의 `L_LandscapeMap`에는 구역·적 생성·목표가 배치되지 않아 보스전과 승패·결과 전환이 자동 실행되지 않습니다.
현재 기본 실행 범위는 **3인 자유 탐색·전투**입니다.

## 김도윤

김도윤은 아래 영역을 중심으로 구현했으며, 게임 UI와 자동화 테스트는 공동 작업했습니다.

| 담당 영역 | 구현 내용 | 코드 |
| --- | --- | --- |
| 게임 진행 | 구역 진입·적 생성·클리어·포털·파티 전멸·결과 상태를 연결하고 단계별 진행을 통합 | [`GP_GameMode`](Project_Eden/Source/Project_Eden/Private/Game/GP_GameMode.cpp), [`GP_GameState`](Project_Eden/Source/Project_Eden/Private/Game/GP_GameState.cpp) |
| 월드 런타임 | Region 상태와 PCG 식생, Run Seed 기반 마을 선택·동기화 및 단계별 월드 이동을 통합 | [`Region`](Project_Eden/Source/Project_Eden/Private/Game/Regions/GP_RegionSpatialSubsystem.cpp), [`WorldLayout`](Project_Eden/Source/Project_Eden/Private/Game/WorldLayout/GP_VillageLayoutDirector.cpp) |
| 플레이어 성장 | 경험치·레벨, 스킬 장착, 원소·증강 선택 상태를 PlayerState와 GAS에 연결 | [`GP_PlayerState`](Project_Eden/Source/Project_Eden/Private/Player/GP_PlayerState.cpp), [`GP_SkillBase`](Project_Eden/Source/Project_Eden/Public/AbilitySystem/Abilities/GP_SkillBase.h) |
| 게임 UI | 스킬·증강 선택, 슬롯 HUD, 로딩, 탈락 관전 및 런 결과 화면을 팀원과 공동 구현·통합 | [`UI`](Project_Eden/Source/Project_Eden/Public/UI/GP_PlayerHUDWidget.h), [`Run Result`](Project_Eden/Source/Project_Eden/Private/UI/GP_RunResultWidget.cpp) |
| 네트워크 흐름 | IP 기반 3인 로비·세션, Seamless Travel, 포털 집결과 클라이언트 입력·상태 동기화 구현·보강 | [`Lobby`](Project_Eden/Source/Project_Eden/Private/Game/GP_LobbyGameMode.cpp), [`Session`](Project_Eden/Source/Project_Eden/Private/Game/GP_ThreePlayerGameSession.cpp) |
| 빌드·배포 | Windows Client·Dedicated Server의 개발·시연용 빌드, Cook, 패키지, 산출물 검증을 자동화 | [`Dedicated Server Scripts`](Project_Eden/Scripts/DedicatedServer/README.md), [`Final Deploy`](Project_Eden/Scripts/DedicatedServer/BuildCookDeployFinal.bat) |
| 안정성 검증 | 진행·세션·전멸·UI·보스 전투 회귀를 확인하는 Unreal Automation Test를 팀원과 공동 작성·보강 | [`Tests`](Project_Eden/Source/Project_Eden/Private/Tests/ZoneProgressionTests.cpp) |

## 핵심 기술 문제

### 1. 3인 파티 상태를 서버 기준으로 일치시키기

로비 인원과 Ready 상태, 전투 진행, 포털 통과, 런 결과를 각 클라이언트가 따로 판단하면 서로 다른 화면이 보일 수 있습니다.
권한 서버가 진행을 결정하고 `GameState`와 `PlayerState`가 필요한 상태를 복제하도록 역할을 분리했습니다.
포털은 현재 접속 중인 모든 플레이어의 통과를 확인한 뒤 다음 진행을 허용합니다.

### 2. 맵 전환 후에도 접속과 플레이 상태 유지하기

큰 맵 로딩 중 비연속 이동을 사용하면 클라이언트가 시간 초과로 이탈할 수 있었습니다.
로비에서 게임 맵으로 이동할 때 Seamless Travel을 사용하고,
실패 시 로딩 UI를 복구하거나 연결 오류를 기록한 뒤 메인 메뉴로 돌아가도록 처리했습니다.

### 3. 에디터 결과를 재현 가능한 서버·클라이언트 패키지로 만들기

전용 서버는 C++ 실행 파일뿐 아니라 Blueprint, 맵, PCG 플러그인과 Cook된 에셋이 함께 맞아야 합니다.
빌드 전 엔진·플러그인·Git 상태를 검사하고 Client/Server를 각각 Build → Cook → Package한 뒤
실행 파일과 Pak/IoStore 산출물까지 확인하는 개발·시연용 배포 스크립트를 구성했습니다.

## 코드 구조

```text
Project_Eden/
├─ Config/                         # 맵, 렌더링, 입력, 세션 설정
├─ Content/                        # Blueprint, 맵, UI, VFX, 외부 에셋
├─ Scripts/
│  ├─ DedicatedServer/             # Client/Server 빌드·Cook·실행 자동화
│  └─ Editor/                      # 반복 에셋 설정용 Python 도구
├─ Source/Project_Eden/
│  ├─ AbilitySystem/               # GAS, 스킬, 능력치, 증강
│  ├─ AI/ · Characters/           # 적·보스 AI와 캐릭터
│  ├─ Game/                        # 로비, 세션, 런, 지역·마을 진행
│  ├─ PCG/                         # 절차적 월드 데이터와 컨트롤러
│  ├─ Player/                      # PlayerController / PlayerState
│  ├─ UI/ · VFX/                  # HUD, 선택 화면, 전투 표현
│  └─ Tests/                       # Unreal Automation Test
└─ Project_Eden.uproject
```

## 빌드 및 실행

### 준비 사항

- Windows와 Visual Studio C++ 게임 개발 워크로드
- 팀 커스텀 엔진 `ProjectEden_Engine` (검증 환경: Unreal Engine 5.7.2)
- Git LFS
- PCGExtendedToolkit 등 프로젝트에서 요구하는 플러그인

### 저장소 받기

```powershell
git lfs install
git clone https://github.com/boowerk/Capstone_Project.git
cd Capstone_Project
git lfs pull
```

`Project_Eden/Project_Eden.uproject`를 `ProjectEden_Engine`으로 등록된 엔진에 연결합니다.
동일한 커스텀 엔진과 플러그인 환경이 없으면 기본 Unreal Engine 설치만으로 바로 빌드할 수 없습니다.
프로젝트 파일을 생성한 뒤 `Project_EdenEditor | Development Editor | Win64`로 빌드하고 에디터에서 실행합니다.
기본 시작 맵은 `MainMenuMap`, 서버 시작 맵은 `LobbyMap`입니다.

### 전용 서버·패키지 클라이언트

```powershell
cd Project_Eden\Scripts\DedicatedServer
./BuildDevServer.bat
./CookDevServer.bat
./BuildDevClient.bat
./CookDevClient.bat
```

서버는 `StartDevServer_Cooked.bat`, 로컬 클라이언트는 `StartDevClient_Cooked_Localhost.bat`로 실행합니다.
전체 배포본은 `BuildCookDeployFinal.bat`가 Client와 Server를 함께 빌드·패키징하고 필수 산출물을 확인합니다.
세부 사용법은 [`Dedicated Server Scripts`](Project_Eden/Scripts/DedicatedServer/README.md)에 정리되어 있습니다.

최신 `main`에서 `Project_EdenEditor`, `Project_EdenServer`, `Project_Eden` Win64 Development 빌드를 확인했습니다.

일부 그래픽·VFX에는 Unreal Marketplace/Fab 외부 에셋을 사용했습니다.
