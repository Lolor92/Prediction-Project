#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/SP_PredictedReactionTypes.h"
#include "SP_PredictionComponent.generated.h"

class AActor;
class UAnimMontage;

UCLASS(ClassGroup=(SyncPrediction), meta=(BlueprintSpawnableComponent))
class SYNCPREDICTION_API USP_PredictionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USP_PredictionComponent();

	UFUNCTION(BlueprintCallable, Category = "SyncPrediction|Reaction")
	bool PlayPredictedReactionOnTargetProxy(AActor* TargetActor, const FSP_PredictedReactionAnimation& Reaction);

	UFUNCTION(BlueprintCallable, Category = "SyncPrediction|Reaction")
	bool PlayPredictedReactionMontageOnTarget(AActor* TargetActor, UAnimMontage* Montage, float PlayRate = 1.f,
		FName StartSection = NAME_None);

private:
	bool CanPlayPredictedReactionOnTargetProxy(AActor* TargetActor, const FSP_PredictedReactionAnimation& Reaction) const;

	UPROPERTY(Transient)
	mutable TMap<TWeakObjectPtr<AActor>, double> LastReactionTimeByTarget;
};
