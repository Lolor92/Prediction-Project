#pragma once

#include "CoreMinimal.h"
#include "SI_CooldownTypes.generated.h"

USTRUCT(BlueprintType)
struct FSI_SlotCooldownState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Cooldown")
	bool bIsOnCooldown = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Cooldown")
	float TimeRemaining = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Cooldown")
	float TotalDuration = 0.f;

	float GetNormalizedRemaining() const
	{
		if (TotalDuration <= KINDA_SMALL_NUMBER)
		{
			return 0.f;
		}

		return FMath::Clamp(TimeRemaining / TotalDuration, 0.f, 1.f);
	}
};
