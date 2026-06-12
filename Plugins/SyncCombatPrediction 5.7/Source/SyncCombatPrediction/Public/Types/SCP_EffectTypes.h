#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SCP_EffectTypes.generated.h"

class UGameplayEffect;

USTRUCT(BlueprintType)
struct SYNCCOMBATPREDICTION_API FSCP_GameplayEffectSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GameplayEffect")
	TArray<TSubclassOf<UGameplayEffect>> ActivationSelfEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GameplayEffect")
	TArray<TSubclassOf<UGameplayEffect>> TargetEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GameplayEffect")
	TArray<TSubclassOf<UGameplayEffect>> DamageEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GameplayEffect", meta=(Categories="Damage.Type"))
	FGameplayTag PhysicalDamageSetByCallerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GameplayEffect")
	float PhysicalAttackPercent = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GameplayEffect", meta=(Categories="Damage.Type"))
	FGameplayTag MagicalDamageSetByCallerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GameplayEffect")
	float MagicalAttackPercent = 0.f;

	bool HasAnyEffects() const
	{
		return !ActivationSelfEffects.IsEmpty() ||
			!TargetEffects.IsEmpty() ||
			!DamageEffects.IsEmpty();
	}
};
