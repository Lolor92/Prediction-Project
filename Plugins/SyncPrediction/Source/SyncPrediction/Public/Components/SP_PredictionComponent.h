#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/SP_ReactionData.h"
#include "SP_PredictionComponent.generated.h"

class AActor;

UCLASS(ClassGroup=(SyncPrediction), meta=(BlueprintSpawnableComponent))
class SYNCPREDICTION_API USP_PredictionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USP_PredictionComponent();

	UFUNCTION(BlueprintCallable, Category = "SyncPrediction|Reaction")
	bool PlayPredictedReactionOnTargetProxy(AActor* TargetActor, FGameplayTag ReactionTag);

	UFUNCTION(BlueprintCallable, Category = "SyncPrediction|Reaction")
	bool ApplyReactionEffectsToTarget(AActor* TargetActor, FGameplayTag ReactionTag);

private:
	bool CanPlayPredictedReactionOnTargetProxy(
		AActor* TargetActor,
		const FSP_ReactionDataEntry& Reaction) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SyncPrediction|Reaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USP_ReactionData> ReactionData = nullptr;

	UPROPERTY(Transient)
	mutable TMap<TWeakObjectPtr<AActor>, double> LastReactionTimeByTarget;
};
