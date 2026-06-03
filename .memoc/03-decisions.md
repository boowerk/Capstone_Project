---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-05-21T07:03:24
status: active
tags:
  - memoc
  - memoc/state
---
# Decisions

Durable project decisions live here. Keep entries short, dated, and useful to future agents.

## Decision Log

### 2026-05-20
- Use `UEFNSourceMesh` as the runtime animation-driving source and let `CharacterMesh0` (`MaskMan`) receive pose via `Retarget Pose From Mesh`, instead of driving MaskMan first.
- Reuse existing UEFN mannequin pose-search assets first; start with a minimal source AnimBP and a single existing PSD before building a fuller chooser/database stack.
- Prefer explicit runtime DB selection in `GP_CharacterAnimInstance` over the stock UEFN chooser for now, because chooser context wiring in this project/tooling path did not switch databases reliably in PIE.
- Prefer separate fixed `Motion Matching` nodes per locomotion state over hot-swapping the `Database` pin on a single node, because runtime DB replacement did not produce visible pose changes in PIE.
- Keep `CurrentMotionMatchState` enum as locomotion branch source of truth; remove temporary `bUse*MotionMatch` helper flags once enum-driven blending is wired.
- Prefer the Blueprint-added `CharacterTrajectoryComponent` on `BP_GP_PlayerCharacter` as trajectory source for `ABP_UEFNSource_Player` over the temporary C++ `GeneratedTrajectory` path when validating/debugging motion matching.
- To avoid fragile Blueprint nested-property wiring, bridge the Blueprint `CharacterTrajectoryComponent` into `GeneratedTrajectory` inside `UGP_CharacterAnimInstance` via reflection, then keep `GeneratedTrajectory -> Pose History` as the AnimGraph input path.
- Do not keep expanding bespoke locomotion branches as the long-term solution; chooser should remain the main database-selection authority once its expected context variables are restored.
- Restore the stock relaxed chooser by matching its input contract in `UGP_CharacterAnimInstance` (`MovementMode`, `Stance`, `MovementState`, `Gait`, direction flags, landing flags, and previous-frame state) rather than cloning chooser logic into code.
- For MaskMan locomotion, treat default movement speed (`500`) as run-family motion and sprint speed (`700`) as sprint-family motion; do not require a walk-family chooser branch in the first custom table pass.
- Build a new custom chooser rooted at `CHT_MM_MaskMan_Root` with embedded `Idle`, `Run`, `Sprint`, and `InAir` nested choosers, using the stock relaxed chooser layouts as templates instead of trying to salvage every original branch directly.
- Use a dedicated `SprintSpeedThreshold` (currently `650`) instead of reusing the broad run threshold, so chooser gait classification does not mark base speed `500` as sprint.
- **2026-05-23**: Elevate the `ContextData(0)` type of `CHT_MM_MaskMan_Root_OriginalStyle` (and other chooser tables) from the specific Blueprint class `ABP_UEFNSource_Player_C` to the common C++ parent class `GP_CharacterAnimInstance`. This prevents type mismatch errors (`expects ABP_UEFNSource_Player_C, but ABP_MaskMan_Player_C was passed in`) when evaluating motion matching for the MaskMan player.
- **2026-05-23**: ?쒖옄由??뚯쟾(Turn In Place) ?곹깭? ?湲?Idle) ?곹깭媛 援먯감?섎ŉ 諛쒖깮?섎뒗 ?좊땲硫붿씠??踰꾨쾮嫄곕┝(Chattering) ?꾩긽???쒖뼱?섍린 ?꾪빐, 理쒖냼 ???좎? ?쒓컙(TurnInPlaceMinDuration = 0.6珥? 諛?紐⑥뀡 留ㅼ묶 ???좊땲硫붿씠???먯뀑 ?대쫫 遺꾩꽍 湲곕컲 ?좉툑(Animation Lock) 濡쒖쭅???꾩엯?섏뿬 遺?쒕윭???쒖옄由??뚯쟾 ?곗텧???좎??섎룄濡?寃곗젙??
# Decisions

Durable project decisions live here. Keep entries short, dated, and useful to future agents.

## Decision Log

### 2026-05-20
- Use `UEFNSourceMesh` as the runtime animation-driving source and let `CharacterMesh0` (`MaskMan`) receive pose via `Retarget Pose From Mesh`, instead of driving MaskMan first.
- Reuse existing UEFN mannequin pose-search assets first; start with a minimal source AnimBP and a single existing PSD before building a fuller chooser/database stack.
- Prefer explicit runtime DB selection in `GP_CharacterAnimInstance` over the stock UEFN chooser for now, because chooser context wiring in this project/tooling path did not switch databases reliably in PIE.
- Prefer separate fixed `Motion Matching` nodes per locomotion state over hot-swapping the `Database` pin on a single node, because runtime DB replacement did not produce visible pose changes in PIE.
- Keep `CurrentMotionMatchState` enum as locomotion branch source of truth; remove temporary `bUse*MotionMatch` helper flags once enum-driven blending is wired.
- Prefer the Blueprint-added `CharacterTrajectoryComponent` on `BP_GP_PlayerCharacter` as trajectory source for `ABP_UEFNSource_Player` over the temporary C++ `GeneratedTrajectory` path when validating/debugging motion matching.
- To avoid fragile Blueprint nested-property wiring, bridge the Blueprint `CharacterTrajectoryComponent` into `GeneratedTrajectory` inside `UGP_CharacterAnimInstance` via reflection, then keep `GeneratedTrajectory -> Pose History` as the AnimGraph input path.
- Do not keep expanding bespoke locomotion branches as the long-term solution; chooser should remain the main database-selection authority once its expected context variables are restored.
- Restore the stock relaxed chooser by matching its input contract in `UGP_CharacterAnimInstance` (`MovementMode`, `Stance`, `MovementState`, `Gait`, direction flags, landing flags, and previous-frame state) rather than cloning chooser logic into code.
- For MaskMan locomotion, treat default movement speed (`500`) as run-family motion and sprint speed (`700`) as sprint-family motion; do not require a walk-family chooser branch in the first custom table pass.
- Build a new custom chooser rooted at `CHT_MM_MaskMan_Root` with embedded `Idle`, `Run`, `Sprint`, and `InAir` nested choosers, using the stock relaxed chooser layouts as templates instead of trying to salvage every original branch directly.
- Use a dedicated `SprintSpeedThreshold` (currently `650`) instead of reusing the broad run threshold, so chooser gait classification does not mark base speed `500` as sprint.
- **2026-05-23**: Elevate the `ContextData(0)` type of `CHT_MM_MaskMan_Root_OriginalStyle` (and other chooser tables) from the specific Blueprint class `ABP_UEFNSource_Player_C` to the common C++ parent class `GP_CharacterAnimInstance`. This prevents type mismatch errors (`expects ABP_UEFNSource_Player_C, but ABP_MaskMan_Player_C was passed in`) when evaluating motion matching for the MaskMan player.
- **2026-05-23**: ?쒖옄由??뚯쟾(Turn In Place) ?곹깭?€ ?€湲?Idle) ?곹깭媛€ 援먯감?섎ŉ 諛쒖깮?섎뒗 ?좊땲硫붿씠??踰꾨쾮嫄곕┝(Chattering) ?꾩긽???쒖뼱?섍린 ?꾪빐, 理쒖젅 ???좎? ?쒓컙(TurnInPlaceMinDuration = 0.6珥? 諛?紐⑥뀡 留ㅼ묶 ???좊땲硫붿씠???먯뀑 ?대쫫 遺꾩꽍 湲곕컲 ?좉툑(Animation Lock) 濡쒖쭅???꾩엯?섏뿬 遺€?쒕윭???쒖옄由??뚯쟾 ?곗텧???좎??섎룄濡?寃곗젙??
- **2026-05-23**: 留덉슦??移대찓?? ?뚯쟾?됱뉙 ?곕Ⅸ ?쒖옄由??뚯쟾 ?? ?좊땲硫붿씠??猷⑦듃 紐⑥뀡 ?뚯쟾???쒓퀎?€ 誘몄꽭 媛곷룄 遺덉쁪移섎줈 ?명븳 ?닿툔??臾몄젣瑜?洹쇱젅?섍린 ?꾪빐, (1) ?뚯쟾 ?곹깭 ?덉텧 媛곷룄 李⑥씠瑜?湲곗〈 5?꾩뿉??2??`ActiveTurnThreshold = 2.0f`)濡??€?댄듃?섍쾶 議곗쑉?섏뿬 ?뺣젹 ?섏???媛뺥솕?섍퀬, (2) `GP_PlayerCharacter::Tick` ?대??먯꽌 ?쒖옄由??곹깭?????좊땲硫붿씠??猷⑦듃 紐⑥뀡 ?뚯쟾??諛섏쁺?섎뒗 ?숈떆??移대찓??諛⑺뼢?쇰줈 誘몄꽭 媛곷룄 ?몄감(15???대궡)媛€ ?⑥븯????`RInterpTo`濡?遺€?쒕읇寃??뺣? ?섎졃(Fine Alignment)?섎뒗 ?섎룞 蹂댁젙 ?쒖옱瑜?異붽??섍린濡?寃곗젙??
- **2026-05-23**: 紐⑥뀡 留ㅼ묶 沅ㅼ쟻 ?앹꽦湲?Trajectory Generator)媛€ ?쒖옄由ъ뿉??留덉슦??移대찓?? ?뚯쟾?됱쓣 ?щ컮瑜닿쾶 ?몄??섏뿬 理쒖쟻???댁씤?뚮젅?댁뒪 ?먯뀑??寃€?됲븷 ???덈룄濡? ?꾨씫?섏뿀??`DesiredControllerYawLastUpdate` 蹂€?섎? ?깅떦 `DesiredYaw` 媛믪쑝濡??ㅼ떆媛?媛깆떊?섎뒗 濡쒖쭅??異붽???
- **2026-05-23**: ?쒖옄由??뚯쟾 ?꾩쨷???몄쐞?곸씤 ?뚯쟾 Interp瑜??쇳빀??寃쎌슦 ?붾뵥諛쒓낵 臾쇰━媛곸씠 ?닿툔??諛쒖씠 誘몃걚?ъ????꾩긽(Slippage)??李⑤떒?섍린 ?꾪빐, ?댁씤?뚮젅?댁뒪媛€ 援щ룞 以묒씪 ?뚮뒗 100% ?쒖닔 猷⑦듃 紐⑥뀡 ?뚯쟾???섏〈?섍퀬, ??紐⑥뀡??醫낅즺??吏곹썑 ?⑥? 誘몄꽭 ?ㅼ감(10???대궡) ?곹솴?먯꽌留??€?€?섍쾶 `RInterpTo`濡?移대찓???뺣갑?μ뿉 諛€李??좉툑(Lock)?섎룄濡??ㅺ퀎瑜??몃텇?뷀븿.
- **2026-05-23**: 湲곗〈 35踰덉쓽 臾쇰━-?좊땲硫붿씠???щ┰ ?덈갑 ?ㅺ퀎瑜?諛뷀깢?쇰줈 `GP_PlayerCharacter::Tick` ?대????댁씤?뚮젅?댁뒪 ?뚯쟾 濡쒖쭅??理쒖쥌 蹂댁셿?? `bInTurnInPlace`媛€ 李몄씤 ?숈븞 ?좊땲硫붿씠??猷⑦듃 紐⑥뀡 ?뚯쟾(`RootMotionDeltaRot`)???≫꽣??100% ?⑥쟾???꾩쟻 諛섏쁺(`AddActorWorldRotation`)?섎뗏 ?숈떆?? ?ㅼ떆媛?移대찓??Yaw 諛⑺뼢?쇰줈??誘몄꽭 ?몄감 異붿쥌???꾪빐 遺€?쒕윭??媛먮룄(`InterpSpeed = 4.0f`)??`RInterpTo` 蹂댁젙 ?뚯쟾???섏씠釉뚮━?쒕줈 寃고빀?? ?먰븳 ???곹깭媛€ ?댁젣??吏곹썑(`bInTurnInPlace == false`?닿퀬 ?띾룄媛€ 0???€湲??곹깭?????먮룄 ?⑥? 誘몄꽭 ?ㅼ감(15???대궡)???€??`RInterpTo(InterpSpeed = 3.0f)` ?섎졃 諛€李⑹쓣 ?섑뻾?섏뿬, 留덉슦???뚯쟾 ???댁씤?뚮젅?댁뒪?먯꽌 Idle濡?釉붾젋?⑸릺???쒖젏???댁깋?⑥씠???좊ℓ???닿툔?⑥쓣 ?꾩쟾??洹쇱젅?섎룄濡?理쒖쥌 寃곗젙??
- **2026-05-23**: ?댁씤?뚮젅?댁뀡???몃━嫄곕릺??利됱떆 C++ RInterpTo ?뚯쟾??媛쒖엯?섎㈃ ?좊땲硫붿씠?섏쓽 ?덈퉬 ?숈옉(Anticipation) ?④퀎?먯꽌 諛쒖씠 誘몃걚?ъ????꾩긽(Initial Slide)???좊컻?⑥쓣 ?뺤씤?섍퀬, ???쒖젉 ??`0.25珥? ?대궡??珥덈컲 援ш컙 ?숈븞?€ ?몄쐞?곸씤 ?섎룞 C++ ?뚯쟾 蹂댁젙???꾨㈃ 李⑤떒?섍린 ?ㅻ줈吏€ ?좊땲硫붿씠???먯껜???쒖닔 猷⑦듃 紐⑥뀡 ?뚯쟾?μ뿉留??섏〈?섎룄濡?媛쒖꽑?? ?붾뵥諛??뚯쟾??蹂멸꺽?곸쑝濡??쒖젉?섎뒗 `0.25珥? ?댄썑 以?諛섎? ?쒖젉遺€??蹂댁젉 媛곷룄(`InterpSpeed = 5.0f`)瑜?遺€?쒕읇寃?媛€誘명븯?? 珥덇린 諛??щ┰ ?꾩긽??0% ?꾨꼍???쒓굅?④낵 ?숈떆??理쒖쥌 1:1 ?뺣젹 ?깅뒫???뺣낫?섎뗏 援ъ“濡??쒕떇?섍린濡?寃곗젙??
- **2026-05-23**: ?꾩쓽 吏€??蹂댁젉 ?꾩엯 ?댄썑?먮룄 留덉슦?ㅻ? 誘몄꽭?섍쾶 議곌툑???뚯쟾?쒗궎???곹솴?먯꽌 ?≫꽣 濡쒗뀒?댁뀡??媛뺤젣濡?移대찓??諛⑺뼢???쇨툑?쇨툑 ?곕씪媛€硫??꾩＜ 誘몄꽭?섍쾶 諛쒖씠 誘몃걚?ъ????꾩긽(Micro-slippage)???ъ쟾??媛먯??⑥쓣 ?몄??? ?대? ?꾨꼍??洹쇱젅?섍린 ?꾪빐 AAA湲??덉쭏??Yaw Hysteresis & One-shot Snap 湲곕쾿???꾩엯?섏뿬, (1) ?쒖옄由??€湲?Idle) 以묒뿉???섎룞 RInterpTo ?뚯쟾 蹂댁젉??100% ?꾩쟾??李⑤떒(OFF)?섏뿬 ?≫꽣 ?뚯쟾??移쇨컳??怨좎젙?⑥쑝濡쒖뜥 諛?誘몃걚?ъ쭚 0% ?먯쿇 遊됱뇙?섍퀬, (2) ?ㅼ쭅 ?댁옉?뚮젅?댁뒪媛€ ?뺤긽 ?쒖꽦?붾릺?덉쓣 ?뚮쭔 猷⑦듃 紐⑥뀡?쇰줈 ?먯뿰?ㅻ읇寃??뚭쾶 ?섎떎媛€, ?댁옉 嫄곗쓽 留덈Т由щ릺???꾨컲遺€(`0.4珥? 寃쎄낵 ?쒖젉)?먮쭔 怨좎냽 ?뚯쟾(`InterpSpeed = 12.0f`)??二쇱뼱 移대찓???뺣㈃ 諛⑺뼢?쇰줈 ????踰덉뿉 鍮좊Ⅴ怨??꾨꼍?섍쾶 ?ㅻ깄(Snap to Camera) ?뺣젹?섎룄濡?理쒖쥌 蹂댁셿??
- **2026-05-23**: 理쒖쥌 寃€利?寃곌낵 ?꾨컲遺€ 怨좎샽 ?ㅻ깄(`RInterpTo 12.0f`)??媛강제 媛쒖엯?섏뿬 ?≫꽣媛€ ???€??遺덉셿???꾩꽦??珥덈옒?⑥쓣 ?몄??섍퀬, C++ 痢≪쓽 ?몄쐞?곸씤 ?섎룞 蹂닿컙 肄붾뱶瑜??꾨㈃ 泥좏룓?? ?쒖옄由??곹깭???뚯쟾?€ ?ㅼ쭅 **100% 紐⑥뀡 留ㅼ묶 ?좊땲硫붿씠??怨좎쑀??猷⑦듃 紐⑥뀡**?먮쭔 ?꾩엫?섏뿬 誘몃걚?ъ쭚怨??뺢????숈떆??洹쇱젅?섍룄濡?寃곗젙?? ?먰븳 ?뚯쟾 媛곷룄 ?쒓퀎濡??명븳 理쒖쥌 1:1 移대찓??Yaw 議곗? ?ㅼ감??C++ 蹂닿컙 ?€??**Pose Search Schema(PSS)??`Trajectory Channel` ??Heading(Yaw) 媛€以묒튂瑜?3.0?쇰줈 ?곹뼢 ?쒕떇**?섏뿬 紐⑥뀡 留ㅼ묶 ?붿쭊??移대찓???뺣갑??議곗???遺€?⑺븯?꾨줉 ?좊퀎?섍쾶 留뚮뱶??**?먯뀑 二쇰룄 ?쒕떇(Asset-driven Alignment)**???뺤꽍 ?닿격을쑝濡?梨꾪깮?섍퀬 ?먮룞???뚯쟠???ㅽ겕리쏀듃(`pss_tune_guide.py`)瑜??쒓났??

### 2026-05-27
- White Void 트랜지션 시 발생할 수 있는 멀티플레이어 simulated proxy 동기화 한계를 차단하기 위해 `bIsInWhiteVoid`를 `ReplicatedUsing` 방식으로 업그레이드하고 `OnRep_IsInWhiteVoid()` 복제 핸들러를 도입하기로 결정.
- 트랜지션 중 카메라 렉 보정 기능 작동 시 로컬 타이머 오작동을 해결하고자 `FTimerHandle`을 헤더 클래스 멤버 변수로 격상 및 중복 타이머 해제 보호막(`ClearTimer`) 추가.
- 모션 매칭 궤적 히스토리 리셋 로직(`ResetMotionTrajectoryAfterWhiteVoidTransition`)이 특정 구조체 버전에 강결합되어 발생할 수 있는 메모리 손상 위험을 방지하기 위해, 리플렉션을 통해 속성 이름 `Position`을 먼저 탐색하여 갱신하는 동적 구조체 검증 로직으로 설계 보완.
- 애니메이션 인스턴스 `NativeUpdateAnimation` 전체 틱이 Suppression 기간에 중단되어 상태 갱신이 마비되는 부작용을 해결하기 위해, 틱은 정상 작동하도록 두고 모션 매칭 노드 평가 지점(`ApplyRuntimeDatabaseToMotionMatchingNode`)에서만 선별적으로 포즈 서치를 무시하도록 차단 구조를 개편.
- 런타임 컴포넌트 동적 생성 로직(`EnsureChildComponents`)에서 불필요하게 사용되던 무거운 에디터 유틸리티 `Owner->AddInstanceComponent`를 완전 철폐하고, 가비지 컬렉션(GC) 보호를 받는 Transient UPROPERTY와 SetupAttachment/RegisterComponent 표준 계층 구조만을 사용하여 결함을 근절.

### 2026-06-04
- 캐릭터 웅크리기(Crouch) 로코모션을 모션 매칭에 통합하기 위해, UGP_CharacterAnimInstance C++ 클래스의 기존 Stance 및 IsStopping 변수를 활용하여 Chooser Table(CHT_MM_MaskMan_Root_OriginalStyle)에 조건 바인딩을 추가하기로 결정.
- Sparse_Crouch_Idles 및 Sparse_Crouch_TurnInPlace PSD 에셋의 결손을 Dense/Extreme_Sparse PSD를 이용한 LOD 대체 매핑(Fallback)으로 해결하여 개발 효율을 높임.
- 루트 테이블 및 신규 서브 테이블(Crouch Idles / Crouch Walks)을 설계하여 에디터 상에서 최종 구성하도록 지침 수립.
