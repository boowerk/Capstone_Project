#include "Player/GP_PlayerState.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"


AGP_PlayerState::AGP_PlayerState()
{
	SetNetUpdateFrequency(100.f);

	AbilitySystemComponent = CreateDefaultSubobject<UGP_AbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

}

UAbilitySystemComponent* AGP_PlayerState::GetAbilitySystemComponent() const {
	return AbilitySystemComponent;
}
