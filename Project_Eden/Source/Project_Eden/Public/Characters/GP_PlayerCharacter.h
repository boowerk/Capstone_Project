#pragma once

#include "CoreMinimal.h"
#include "Characters/GP_BaseCharacter.h"
#include "AbilitySystemInterface.h"
#include "GP_PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UPDA_WeaponItemCollection;
class UPDA_CharacterAnimationSet;
class UAnimSequenceBase;
class UBlendSpace;

UCLASS()
class PROJECT_EDEN_API AGP_PlayerCharacter : public AGP_BaseCharacter
{
	GENERATED_BODY()

public:
	AGP_PlayerCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue, bool bForce = false) override;
	void EquipSkill(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> NewAbilityClass);
	
	void ToggleSprinting(); 
	bool IsSprinting() const; 
	
	bool TryPerformDash();
	bool IsDashing() const; 
	
	bool IsPrimaryAttacking() const;
	bool IsSprintExitControlLocked() const;

	UPDA_CharacterAnimationSet* GetAnimationSet() const { return AnimationSet; }
	UBlendSpace* GetLocomotionBlendSpace() const;
	UAnimSequenceBase* GetJumpLoopAnimation() const;
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ApplySprintStopLock(float LockTime = 0.2f);

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
protected:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|LockOn")
	bool bIsLockOn = false; 
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|LockOn")
	TObjectPtr<AActor> TargetActor; 
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|LockOn")
	float LockOnRotationInterpSpeed = 10.0f;

	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Weapon")
	TObjectPtr<UPDA_WeaponItemCollection> DefaultWeaponCollection;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Weapon")
	FName DefaultWeaponId = TEXT("WP_Common_Fire_Sword");

	
	// 어빌리티들용 애니메이션 통합 데이터 에셋
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UPDA_CharacterAnimationSet> AnimationSet;

	
	// 이동 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed")
	float NormalWalkSpeed = 210.0f; 

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed")
	float SprintSpeed = 420.0f;

	
	// GAS 태그 이벤트 콜백
	virtual void OnSprintingTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	
};
