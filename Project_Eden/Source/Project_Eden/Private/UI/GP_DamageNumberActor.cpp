#include "UI/GP_DamageNumberActor.h"

#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "UI/GP_DamageNumberWidget.h"

AGP_DamageNumberActor::AGP_DamageNumberActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
    WidgetComponent->SetupAttachment(SceneRoot);
    WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    WidgetComponent->SetDrawAtDesiredSize(true);
    WidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
    WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    DamageNumberWidgetClass = UGP_DamageNumberWidget::StaticClass();
    WidgetComponent->SetWidgetClass(DamageNumberWidgetClass);
}

void AGP_DamageNumberActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    SyncWidgetComponentClass();
    RefreshWidget();
}

void AGP_DamageNumberActor::BeginPlay()
{
    Super::BeginPlay();

    StartLocation = GetActorLocation() + FVector(0.0f, 0.0f, SpawnHeightOffset);
    SetActorLocation(StartLocation);

    // 같은 위치에 여러 숫자가 뜰 때 전부 겹치지 않도록 살짝 퍼뜨린다.
    const float RandomYaw = FMath::FRandRange(0.0f, 360.0f);
    DriftOffset = FRotator(0.0f, RandomYaw, 0.0f).Vector() * FMath::FRandRange(12.0f, LateralSpread);

    const int32 Direction = FMath::RandBool() ? 1 : -1;
    const float HorizontalDistance = FMath::FRandRange(30.0f, 70.0f);

    // 오른쪽/왼쪽 랜덤
    DriftOffset = FVector(0.0f, Direction * HorizontalDistance, 0.0f);

    SyncWidgetComponentClass();
    RefreshWidget();
}

void AGP_DamageNumberActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    ElapsedSeconds += DeltaSeconds;
    const float Alpha = FMath::Clamp(ElapsedSeconds / LifetimeSeconds, 0.0f, 1.0f);

    // 좌우 랜덤 이동 (BeginPlay나 Spawn 시 정해둔 DriftOffset 사용)
    const FVector HorizontalOffset = DriftOffset * Alpha;

    // 포물선 높이 계산
    // Alpha: 0 ~ 1
    // 4 * Alpha * (1 - Alpha) 는 0 -> 1 -> 0 형태의 포물선
    const float ArcZ = ArcHeight * 4.0f * Alpha * (1.0f - Alpha);

    // 시작 높이보다 약간 아래로 떨어지게 하고 싶으면 FallOffset 추가
    const float FallZ = -FallDistance * Alpha;

    const FVector NewLocation = StartLocation + HorizontalOffset + FVector(0.0f, 0.0f, ArcZ + FallZ);
    SetActorLocation(NewLocation);

    if (DamageWidget)
    {
        const float Opacity = Alpha < 0.65f ? 1.0f : 1.0f - ((Alpha - 0.65f) / 0.35f);
        const float Scale = 0.92f + (FMath::Sin(Alpha * PI) * 0.18f);

        DamageWidget->SetDisplayOpacity(Opacity);
        DamageWidget->SetDisplayScale(Scale);
    }

    if (Alpha >= 1.0f)
    {
        Destroy();
    }
}

void AGP_DamageNumberActor::InitializeDamageNumber(int32 InDamageValue, EWeaponElement InElement)
{
    DamageValue = InDamageValue;
    DamageElement = InElement;
    RefreshWidget();
}

void AGP_DamageNumberActor::OnRep_DamageData()
{
    RefreshWidget();
}

void AGP_DamageNumberActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AGP_DamageNumberActor, DamageValue);
    DOREPLIFETIME(AGP_DamageNumberActor, DamageElement);
}

void AGP_DamageNumberActor::SyncWidgetComponentClass()
{
    if (!WidgetComponent)
    {
        return;
    }

    UClass* WidgetClassToUse = DamageNumberWidgetClass.Get();
    if (!WidgetClassToUse)
    {
        WidgetClassToUse = UGP_DamageNumberWidget::StaticClass();
    }

    WidgetComponent->SetWidgetClass(WidgetClassToUse);
    WidgetComponent->InitWidget();
    DamageWidget = Cast<UGP_DamageNumberWidget>(WidgetComponent->GetUserWidgetObject());
}

void AGP_DamageNumberActor::RefreshWidget()
{
    if (!DamageWidget && WidgetComponent)
    {
        DamageWidget = Cast<UGP_DamageNumberWidget>(WidgetComponent->GetUserWidgetObject());
    }

    if (!DamageWidget)
    {
        return;
    }

    UWorld* World = GetWorld();
    const bool bUsePreviewData = !World || !World->IsGameWorld();
    const int32 DisplayDamageValue = bUsePreviewData ? PreviewDamageValue : DamageValue;
    const EWeaponElement DisplayElement = bUsePreviewData ? PreviewElement : DamageElement;

    // 복제 시점과 무관하게 마지막 데미지 데이터를 같은 경로로 반영한다.
    DamageWidget->SetDamageData(DisplayDamageValue, DisplayElement);
}
