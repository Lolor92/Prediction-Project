#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SCP_ReactionData.generated.h"

class UGameplayEffect;
class UAnimMontage;

USTRUCT(BlueprintType, meta=(DisplayName="Predicted Reaction"))
struct FSCP_ReactionMontageEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reaction")
	FGameplayTag ReactionTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reaction")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reaction", meta=(TitleProperty="GameplayEffectClass"))
	TArray<TSubclassOf<UGameplayEffect>> TargetEffects;
};

UCLASS(BlueprintType)
class SYNCCOMBATPREDICTION_API USCP_ReactionData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Reaction", meta=(TitleProperty="ReactionTag"))
	TArray<FSCP_ReactionMontageEntry> Reactions;

	UFUNCTION(BlueprintPure, Category="Sync Combat Prediction|Reaction")
	UAnimMontage* FindReactionMontage(FGameplayTag ReactionTag) const;

	const FSCP_ReactionMontageEntry* FindReaction(FGameplayTag ReactionTag) const;
};
