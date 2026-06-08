# 스킬 비주얼 Cue 구조

이 프로젝트의 스킬 비주얼은 데이터 중심으로 관리한다.

역할 분리:

1. 애니메이션 몽타주/노티파이: 타이밍만 담당
2. Gameplay Ability: 해당 타이밍이 어떤 행동인지 해석
3. `UGP_SkillData`: 어떤 Actor/Niagara를 쓸지 결정

원칙: 재사용되는 전투 몽타주에는 `AnimNotify_PlayNiagaraEffect`처럼 Niagara asset을 직접 넣지 않는다. 스킬, 원소, 강화, 태그에 따라 바뀌어야 하는 VFX는 Data Asset에서 고른다.

## 런타임 흐름

런타임 장착 스킬은 `AGP_PlayerCharacter::EquipSkill`에서 어빌리티를 부여할 때 선택된 `UGP_SkillData`를 `FGameplayAbilitySpec::SourceObject`로 넣는다.

`UGP_SkillBase::GetSkillDataFromSpec`는 SkillData를 다음 순서로 찾는다.

1. Ability spec의 `SourceObject`
2. Ability Blueprint의 `Default Skill Data`

`Default Skill Data`는 Primary처럼 항상 지급되는 기본 어빌리티용 fallback이다. 기본 지급 어빌리티는 `SourceObject` 없이 `StartupAbilities`로 들어올 수 있기 때문이다.

주의: 새 `UPROPERTY`는 Live Coding만으로 Blueprint Details 패널에 바로 보이지 않을 수 있다. 이 경우 에디터 재시작 또는 정식 컴파일 후 `Default Skill Data`를 볼 수 있다. 현재 `GA_Primary`는 재로드 후 `Default Skill Data`가 `DA_Skill_Primary`로 연결되어 있다.

## VisualCues 선택 규칙

`UGP_SkillData.VisualCues`의 각 항목은 아래 정보를 가진다.

- `Cue Tag`: 사용 상황 키. 예: trail, burst, impact, active
- `Element Tag`: 원소 조건. 비워두면 모든 원소
- `Visual Type`: Actor 또는 Niagara
- `Visual Actor Class`: Actor 타입일 때 사용
- `Niagara System`: Niagara 타입일 때 사용

`UGP_SkillBase::GetSkillNiagaraSystem`는 `Cue Tag`와 `Element Tag`를 기준으로 가장 구체적인 Niagara를 선택한다.

우선순위:

1. Cue + Element 둘 다 일치
2. Cue만 일치
3. Element만 일치
4. 둘 다 비어 있는 기본값

## Primary Sword_Light 구조

`UGP_Primary`는 이제 DataAsset 기반 VFX 경로를 사용한다.

Sword_Light 몽타주에는 전투 타이밍 노티파이만 남긴다.

- `GPTags.Event.Player.AttackHit`
- `GPTags.Event.Player.ComboEnable`
- `GPTags.Event.Player.ActionEnd`

Primary는 이 타이밍을 다음 비주얼 동작으로 해석한다.

- 콤보 스윙 시작: `Primary Trail Cue Tag`로 trail Niagara를 찾아 시작
- `AttackHit`: `Primary Burst Cue Tag`로 burst Niagara를 찾아 1회 spawn
- `ActionEnd` 또는 어빌리티 종료: trail Niagara 정지

새 `Default Skill Data` 프로퍼티가 아직 반영되지 않은 세션을 위해 Primary는 아래 asset도 fallback으로 직접 로드한다.

`/Game/GAS_Pattern/AbilitySystem/SkillData/DA_Skill_Primary`

기본 Cue Tag:

- Trail: `GameplayCue.Ability.Trail.Magic`
- Burst: `GameplayCue.Ability.Burst.Magic`

두 태그는 config-only 태그가 아니다. 기존 프로젝트 tag 구조와 맞추기 위해 `Source/Project_Eden/Public/GameplayTags/GP_Tags.h` / `Private/GameplayTags/GP_Tags.cpp`에 native tag로 선언한다.

이름은 `GameplayCue.` prefix를 유지한다. Unreal GameplayCue 계열 asset과 convention이 이 root를 기대하기 때문이다. C++ 접근 이름은 아래와 같다.

- `GPTags::GameplayCue::Ability::Trail_Magic`
- `GPTags::GameplayCue::Ability::Burst_Magic`

기본 attach 대상:

- 보이는 캐릭터 메시 `GetMesh()`
- 소켓/본: `hand_r`
- 위치/회전 보정: `Primary VFX Location Offset`, `Primary VFX Rotation Offset`

## Primary 설정 방법

Live Coding compile 후 Primary 어빌리티 블루프린트를 연다.

보통 경로:

`/Game/GAS_Pattern/AbilitySystem/Abilities/PlayerAbilities/GA_Primary`

설정할 값:

- `Default Skill Data`: Primary용 `UGP_SkillData`
- `Primary Trail Cue Tag`: `GameplayCue.Ability.Trail.Magic`
- `Primary Burst Cue Tag`: `GameplayCue.Ability.Burst.Magic`
- `Primary VFX Socket Name`: `hand_r`

Primary용 `UGP_SkillData`에는 `Visual Cues`를 추가한다.

Trail 항목:

- `Cue Tag`: `GameplayCue.Ability.Trail.Magic`
- `Visual Type`: `Niagara`
- `Niagara System`: 예시 `NS_ArrowTrail_Magic`

Burst 항목:

- `Cue Tag`: `GameplayCue.Ability.Burst.Magic`
- `Visual Type`: `Niagara`
- `Niagara System`: 예시 `NS_Free_Magic_Slash`

현재 `DA_Skill_Primary`는 Cue Tag까지 세팅되어 있다.

- `Visual Cues[0]`: trail Niagara
- `Visual Cues[1]`: burst Niagara

그래도 Primary code에는 fallback index가 남아 있다. Cue Tag가 누락된 세션에서도 `Visual Cues[0]`은 trail, `Visual Cues[1]`은 burst로 동작하게 하기 위한 안전장치다.

원소별 VFX를 쓰려면 같은 Cue Tag에 `Element Tag`만 추가한다.

예시:

- `Cue Tag`: `GameplayCue.Ability.Burst.Magic`
- `Element Tag`: `GPTags.Tech.Element.Pyros`
- `Niagara System`: 화염 burst Niagara

플레이어의 현재 tech element가 `Pyros`면 이 항목이 일반 burst보다 우선된다.

## 노티파이는 어디까지 관리하나

노티파이가 관리하는 것:

- hit frame
- combo window
- action end
- cast frame
- impact frame
- 특정 cue frame

노티파이가 관리하지 않는 것:

- 어떤 Niagara asset을 쓸지
- 어떤 원소 VFX를 쓸지
- 어떤 강화 상태의 VFX를 쓸지
- 어떤 액터 클래스를 spawn할지

이 정보는 `UGP_SkillData`가 관리한다.

## 새 타이밍 VFX 추가 절차

새로운 타이밍에 VFX가 필요하면 다음 순서로 추가한다.

1. 몽타주에 `UGP_AnimNotify_SendGameplayEvent`를 추가한다.
2. 원하는 프레임에 gameplay event tag를 설정한다.
3. 어빌리티에서 해당 event를 `UAbilityTask_WaitGameplayEvent`로 받는다.
4. 어빌리티에 해당 상황용 Cue Tag 프로퍼티를 둔다.
5. `GetSkillNiagaraSystem(SkillData, ElementTag, CueTag)`로 Niagara를 찾는다.
6. 찾은 Niagara를 spawn 또는 attach한다.
7. `UGP_SkillData.VisualCues`에 Cue Tag와 Niagara를 등록한다.

이렇게 하면 몽타주는 타이밍만, 어빌리티는 행동만, DataAsset은 asset 선택만 담당한다.

## 예외

아래 경우에는 직접 Niagara notify를 써도 된다.

- 특정 시네마틱 몽타주에만 들어가는 일회성 이펙트
- 스킬/원소/강화에 따라 절대 바뀌지 않는 애니메이션 장식
- GAS나 SkillData와 무관한 순수 애니메이션 프리뷰용 효과

전투 스킬 VFX는 기본적으로 DataAsset 경로를 사용한다.

## 현재 한계

현재 Primary trail은 콤보 스윙 시작 시 켜지고 `ActionEnd`에서 꺼진다.

나중에 trail 시작 시점을 몽타주 안의 더 정확한 프레임으로 제어해야 하면, direct Niagara notify를 넣지 말고 별도 gameplay event notify를 추가한 뒤 같은 DataAsset cue 조회 흐름으로 연결한다.
