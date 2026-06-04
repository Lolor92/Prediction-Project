// Copyright ProjectLogos

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SyncCombatTagReactionData.generated.h"

class UGameplayEffect;

UENUM(BlueprintType)
enum class ESyncCombatTagReactionPolicy : uint8
{
	OnAdd    UMETA(DisplayName="On Add"),
	OnRemove UMETA(DisplayName="On Remove"),
	Both     UMETA(DisplayName="On Both")
};

USTRUCT(BlueprintType)
struct FSyncCombatTagReactionAbility
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability", meta=(ClampMin="0.0", UIMin="0.0", Units="Seconds"))
	float DelaySeconds = 0.f;
};

USTRUCT(BlueprintType)
struct FSyncCombatTagReactionEffects
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	TArray<TSubclassOf<UGameplayEffect>> Apply;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects", meta=(ClampMin="0.0", UIMin="0.0", Units="Seconds"))
	float ApplyDelaySeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	TArray<TSubclassOf<UGameplayEffect>> Remove;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects", meta=(ClampMin="0.0", UIMin="0.0", Units="Seconds"))
	float RemoveDelaySeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	FName RemoveTimerKey;
};

USTRUCT(BlueprintType)
struct FSyncCombatTagReactionBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trigger")
	FGameplayTag TriggerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trigger")
	ESyncCombatTagReactionPolicy Policy = ESyncCombatTagReactionPolicy::OnAdd;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability", meta=(ShowOnlyInnerProperties))
	FSyncCombatTagReactionAbility Ability;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects", meta=(ShowOnlyInnerProperties))
	FSyncCombatTagReactionEffects Effects;
};

UCLASS(BlueprintType)
class SYNCCOMBAT_API USyncCombatTagReactionData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Tag Reactions", meta=(TitleProperty="TriggerTag"))
	TArray<FSyncCombatTagReactionBinding> Reactions;
};
