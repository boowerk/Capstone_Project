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
}

void AGP_DamageNumberActor::BeginPlay()
{
    Super::BeginPlay();

    StartLocation = GetActorLocation() + FVector(0.0f, 0.0f, SpawnHeightOffset);
    SetActorLocation(StartLocation);

    // 매 타격 숫자가 완전히 겹치지 않도록 살짝 옆으로 퍼지게 만든다.
    const float RandomYaw = FMath::FRandRange(0.0f, 360.0f);
    DriftOffset = FRotator(0.0f, RandomYaw, 0.0f).Vector() * FMath::FRandRange(12.0f, LateralSpread);

    DamageWidget = CreateWidget<UGP_DamageNumberWidget>(GetWorld(), UGP_DamageNumberWidget::StaticClass());
    WidgetComponent->SetWidget(DamageWidget);
    RefreshWidget();
}

void AGP_DamageNumberActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    ElapsedSeconds += DeltaSeconds;
    const float Alpha = FMath::Clamp(ElapsedSeconds / LifetimeSeconds, 0.0f, 1.0f);

    // 원신식처럼 위로 떠오르면서 끝부분에서 자연스럽게 사라지도록 보간한다.
    const float VerticalOffset = FMath::InterpEaseOut(0.0f, FloatHeight, Alpha, 1.7f);
    const FVector NewLocation = StartLocation + DriftOffset * Alpha + FVector(0.0f, 0.0f, VerticalOffset);
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

void AGP_DamageNumberActor::RefreshWidget()
{
    if (!DamageWidget)
    {
        return;
    }

    // 복제된 값이 도착해도 같은 경로로 위젯 내용을 다시 갱신한다.
    DamageWidget->SetDamageData(DamageValue, DamageElement);
}
