#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Types/SCP_PredictionTypes.h"
#include "SCP_CombatPredictionBlueprintLibrary.generated.h"

class UGameplayAbility;
class USCP_CombatPredictionComponent;
class AActor;

UCLASS()
class SYNCCOMBATPREDICTION_API USCP_CombatPredictionBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Sync Combat Prediction|GAS",
		meta=(DefaultToSelf="Ability", DisplayName="Make Combat Prediction Context From Ability"))
	static FSCP_CombatPredictionContext MakePredictionContextFromAbility(
		const UGameplayAbility* Ability);

	UFUNCTION(BlueprintCallable, Category="Sync Combat Prediction|GAS",
		meta=(DefaultToSelf="Ability", DisplayName="Start Combat Prediction From Ability"))
	static bool StartCombatPredictionFromAbility(
		const UGameplayAbility* Ability,
		FSCP_CombatPredictionContext& OutContext);

	UFUNCTION(BlueprintPure, Category="Sync Combat Prediction", meta=(DefaultToSelf="Actor"))
	static USCP_CombatPredictionComponent* GetCombatPredictionComponent(const AActor* Actor);

	UFUNCTION(BlueprintPure, Category="Sync Combat Prediction|Reaction", meta=(DefaultToSelf="Actor"))
	static bool IsPlayingPredictedTargetReaction(const AActor* Actor);

	UFUNCTION(BlueprintPure, Category="Sync Combat Prediction|Reaction", meta=(DefaultToSelf="Actor"))
	static void ApplyPredictedTargetReactionAnimLock(
		const AActor* Actor,
		float CurrentGroundSpeed,
		bool bCurrentIsAccelerating,
		float CurrentMovementOffsetYaw,
		float& OutGroundSpeed,
		bool& bOutIsAccelerating,
		float& OutMovementOffsetYaw,
		bool& bOutApplied);

	UFUNCTION(BlueprintPure, Category="Sync Combat Prediction")
	static bool IsValidPredictionContext(const FSCP_CombatPredictionContext& Context);

	UFUNCTION(BlueprintPure, Category="Sync Combat Prediction")
	static FString PredictionContextToString(const FSCP_CombatPredictionContext& Context);
};
