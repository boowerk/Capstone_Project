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

    SyncWidgetComponentClass();
    RefreshWidget();
}

void AGP_DamageNumberActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    ElapsedSeconds += DeltaSeconds;
    const float Alpha = FMath::Clamp(ElapsedSeconds / LifetimeSeconds, 0.0f, 1.0f);

    // 위로 부드럽게 떠오르면서 후반부에 자연스럽게 사라지도록 보간한다.
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
