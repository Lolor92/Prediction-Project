#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SCP_EffectTypes.generated.h"

class UGameplayEffect;

USTRUCT(BlueprintType)
struct SYNCCOMBATPREDICTION_API FSCP_GameplayEffectSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit", meta=(DisplayName="Target Effects"))
	TArray<TSubclassOf<UGameplayEffect>> TargetEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit", meta=(DisplayName="Damage Effects"))
	TArray<TSubclassOf<UGameplayEffect>> DamageEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage", meta=(Categories="Damage.Type", DisplayName="Physical Set By Caller Tag"))
	FGameplayTag PhysicalDamageSetByCallerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage", meta=(DisplayName="Physical Attack Percent"))
	float PhysicalAttackPercent = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage", meta=(Categories="Damage.Type", DisplayName="Magical Set By Caller Tag"))
	FGameplayTag MagicalDamageSetByCallerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage", meta=(DisplayName="Magical Attack Percent"))
	float MagicalAttackPercent = 0.f;

	bool HasAnyEffects() const
	{
		return !TargetEffects.IsEmpty() ||
			!DamageEffects.IsEmpty();
	}

	bool HasAnyHitEffects() const
	{
		return HasAnyEffects();
	}
};
