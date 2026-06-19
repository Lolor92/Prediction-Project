#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/SP_ReactionData.h"
#include "GameplayEffectTypes.h"
#include "SP_PredictionComponent.generated.h"

class AActor;
class UAnimInstance;
class UAnimMontage;

USTRUCT()
struct FSP_PendingPredictedReaction
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY()
	FGameplayTag ReactionTag;

	UPROPERTY()
	float TimeSeconds = 0.f;
};

USTRUCT()
struct FSP_ActivePredictedReaction
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY()
	FGameplayTag ReactionTag;

	UPROPERTY()
	float StartTimeSeconds = 0.f;

	UPROPERTY()
	float MontageStartPositionSeconds = 0.f;

	UPROPERTY()
	bool bIgnoreNextMontageStarted = false;
};

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

	void AddPendingPredictedReaction(AActor* TargetActor, FGameplayTag ReactionTag);
	bool ConsumePendingPredictedReaction(AActor* TargetActor, FGameplayTag ReactionTag, float* OutAgeSeconds = nullptr);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category="Sync Prediction|Reaction")
	float PendingPredictedReactionTimeout = 1.0f;

	UPROPERTY(EditAnywhere, Category="Sync Prediction|Reaction")
	float ActivePredictedReactionTimeout = 3.0f;

private:
	bool CanPlayPredictedReactionOnTargetProxy(AActor* TargetActor, const FSP_ReactionDataEntry& Reaction) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SyncPrediction|Reaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USP_ReactionData> ReactionData = nullptr;

	UPROPERTY(Transient)
	mutable TMap<TWeakObjectPtr<AActor>, double> LastReactionTimeByTarget;

	UPROPERTY(Transient)
	TArray<FSP_PendingPredictedReaction> PendingPredictedReactions;

	TWeakObjectPtr<UAnimInstance> BoundAnimInstance;

	void BindToOwnerAnimInstance();
	UFUNCTION()
	void HandleOwnerMontageStarted(UAnimMontage* Montage);

	FGameplayTag FindReactionTagForMontage(const UAnimMontage* Montage) const;

	UPROPERTY(Transient)
	TArray<FSP_ActivePredictedReaction> ActivePredictedReactions;

	void AddActivePredictedReaction(AActor* TargetActor, UAnimMontage* Montage, FGameplayTag ReactionTag,
		float MontageStartPositionSeconds);

	void RemoveActivePredictedReaction(AActor* TargetActor, UAnimMontage* Montage,
		FGameplayTag ReactionTag);

	FSP_ActivePredictedReaction* FindActivePredictedReaction(AActor* TargetActor, UAnimMontage* Montage,
		FGameplayTag ReactionTag);

	void RemoveExpiredActivePredictedReactions();

	float GetActivePredictedReactionMontagePosition(const FSP_ActivePredictedReaction& ActiveReaction) const;
};
