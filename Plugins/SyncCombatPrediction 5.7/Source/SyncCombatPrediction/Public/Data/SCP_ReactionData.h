#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SCP_ReactionData.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType, meta=(DisplayName="Predicted Reaction"))
struct FSCP_ReactionMontageEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reaction")
	FGameplayTag ReactionTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reaction")
	TObjectPtr<UAnimMontage> Montage = nullptr;
};

UCLASS(BlueprintType)
class SYNCCOMBATPREDICTION_API USCP_ReactionData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Reaction", meta=(TitleProperty="ReactionTag"))
	TArray<FSCP_ReactionMontageEntry> Reactions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Reaction")
	TObjectPtr<UAnimMontage> DefaultReactionMontage = nullptr;

	UFUNCTION(BlueprintPure, Category="Sync Combat Prediction|Reaction")
	UAnimMontage* FindReactionMontage(FGameplayTag ReactionTag) const;
};

