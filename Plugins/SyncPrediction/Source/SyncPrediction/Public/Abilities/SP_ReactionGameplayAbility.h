#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "SP_ReactionGameplayAbility.generated.h"

UCLASS()
class SYNCPREDICTION_API USP_ReactionGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION(BlueprintCallable, Category="Sync Prediction|Reaction")
	bool ShouldSkipPredictedReactionReplay(FGameplayTag ReactionTag) const;

	FGameplayTag ResolveReactionTagFromActivationData(const FGameplayEventData* TriggerEventData) const;
};
