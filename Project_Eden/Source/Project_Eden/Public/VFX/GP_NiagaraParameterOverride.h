#pragma once

#include "CoreMinimal.h"
#include "GP_NiagaraParameterOverride.generated.h"

UENUM(BlueprintType)
enum class EGP_NiagaraParameterType : uint8
{
	Float,
	Integer,
	Boolean,
	Vector2D,
	Vector3,
	Color
};

USTRUCT(BlueprintType)
struct FGP_NiagaraParameterOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara")
	FName ParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara")
	EGP_NiagaraParameterType Type = EGP_NiagaraParameterType::Float;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara", meta = (EditCondition = "Type == EGP_NiagaraParameterType::Float", EditConditionHides))
	float FloatValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara", meta = (EditCondition = "Type == EGP_NiagaraParameterType::Integer", EditConditionHides))
	int32 IntegerValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara", meta = (EditCondition = "Type == EGP_NiagaraParameterType::Boolean", EditConditionHides))
	bool BooleanValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara", meta = (EditCondition = "Type == EGP_NiagaraParameterType::Vector2D", EditConditionHides))
	FVector2D Vector2DValue = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara", meta = (EditCondition = "Type == EGP_NiagaraParameterType::Vector3", EditConditionHides))
	FVector Vector3Value = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara", meta = (EditCondition = "Type == EGP_NiagaraParameterType::Color", EditConditionHides))
	FLinearColor ColorValue = FLinearColor::White;

	static FGP_NiagaraParameterOverride MakeFloat(FName Name, float Value)
	{
		FGP_NiagaraParameterOverride Override;
		Override.ParameterName = Name;
		Override.Type = EGP_NiagaraParameterType::Float;
		Override.FloatValue = Value;
		return Override;
	}
};
