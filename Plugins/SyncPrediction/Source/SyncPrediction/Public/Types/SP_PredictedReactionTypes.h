#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SP_PredictedReactionTypes.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType)
struct FSP_PredictedReactionAnimation
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SyncPrediction|Reaction")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SyncPrediction|Reaction")
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SyncPrediction|Reaction")
	FName StartSection = NAME_None;

	// Prevents overlap ticks from replaying the same montage every frame.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SyncPrediction|Reaction")
	float MinReplayInterval = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SyncPrediction|Reaction")
	FGameplayTag ReactionTag;
};