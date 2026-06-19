#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SP_ReactionData.generated.h"

class UAnimMontage;
class UGameplayEffect;

USTRUCT(BlueprintType, meta=(DisplayName="Sync Predicted Reaction"))
struct FSP_ReactionDataEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SyncPrediction|Reaction")
	FGameplayTag ReactionTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SyncPrediction|Reaction")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SyncPrediction|Reaction")
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SyncPrediction|Reaction")
	FName StartSection = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SyncPrediction|Reaction")
	float MinReplayInterval = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SyncPrediction|Reaction")
	bool bCancelActiveAbilityOnCleanHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SyncPrediction|Reaction", meta=(TitleProperty="GameplayEffectClass"))
	TArray<TSubclassOf<UGameplayEffect>> TargetEffects;
};

UCLASS(BlueprintType)
class SYNCPREDICTION_API USP_ReactionData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SyncPrediction|Reaction", meta=(TitleProperty="ReactionTag"))
	TArray<FSP_ReactionDataEntry> Reactions;

	UFUNCTION(BlueprintPure, Category="SyncPrediction|Reaction")
	const FSP_ReactionDataEntry& FindReactionChecked(FGameplayTag ReactionTag) const;

	UFUNCTION(BlueprintPure, Category="SyncPrediction|Reaction")
	bool FindReaction(FGameplayTag ReactionTag, FSP_ReactionDataEntry& OutReaction) const;

	const FSP_ReactionDataEntry* FindReactionPtr(FGameplayTag ReactionTag) const;
};
