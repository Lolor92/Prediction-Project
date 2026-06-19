#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/SP_ReactionData.h"
#include "SP_PredictionComponent.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct FSP_ReactionPredictionContext
{
	GENERATED_BODY()

	UPROPERTY()
	int32 PredictionId = INDEX_NONE;

	bool IsValid() const
	{
		return PredictionId != INDEX_NONE;
	}
};

USTRUCT()
struct FSP_PendingPredictedReaction
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY()
	FGameplayTag ReactionTag;

	UPROPERTY()
	int32 PredictionId = INDEX_NONE;

	UPROPERTY()
	double TimeSeconds = 0.0;
};

USTRUCT()
struct FSP_DeferredPredictedReactionCorrection
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY()
	FGameplayTag ReactionTag;

	UPROPERTY()
	int32 PredictionId = INDEX_NONE;

	UPROPERTY()
	double TimeSeconds = 0.0;
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

	UFUNCTION(Server, Reliable)
	void ServerConfirmPredictedReaction(FSP_ReactionPredictionContext Context, AActor* TargetActor,
		FGameplayTag ReactionTag);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayConfirmedReaction(FSP_ReactionPredictionContext Context, AActor* TargetActor,
		FGameplayTag ReactionTag, float ServerStartTime);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastFinishConfirmedReaction(FSP_ReactionPredictionContext Context, AActor* TargetActor,
		FGameplayTag ReactionTag, FVector ServerFinalLocation);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastStopRootMotionFromContact();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastRestoreRootMotionAfterContact();

	UFUNCTION(Client, Reliable)
	void ClientPlayOwnerConfirmedReaction(FSP_ReactionPredictionContext Context, AActor* ExpectedTargetActor,
		AActor* InstigatorActor, FGameplayTag ReactionTag);

private:
	bool CanPlayPredictedReactionOnTargetProxy(AActor* TargetActor, const FSP_ReactionDataEntry& Reaction) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SyncPrediction|Reaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USP_ReactionData> ReactionData = nullptr;

	UPROPERTY(Transient)
	mutable TMap<TWeakObjectPtr<AActor>, double> LastReactionTimeByTarget;

	static constexpr int32 MaxPredictionId = 32767;
	static constexpr int32 MaxPendingPredictedReactions = 32;

	UPROPERTY(Transient)
	int32 NextPredictionId = 0;

	UPROPERTY(Transient)
	TArray<FSP_PendingPredictedReaction> PendingPredictedReactions;

	UPROPERTY(Transient)
	TArray<FSP_DeferredPredictedReactionCorrection> DeferredPredictedReactionCorrections;

	UPROPERTY(EditAnywhere, Category="SyncPrediction|Reaction", meta=(ClampMin="0.0", Units="Seconds"))
	float PendingPredictedReactionTimeout = 2.0f;

	UPROPERTY(EditAnywhere, Category="SyncPrediction|Reaction", meta=(ClampMin="0.0", Units="Seconds"))
	float DeferredPredictedCorrectionTimeout = 3.0f;

	UPROPERTY(EditAnywhere, Category="SyncPrediction|Reaction", meta=(ClampMin="0.0", Units="Centimeters"))
	float FinalCorrectionTolerance = 2.0f;

	UPROPERTY(EditAnywhere, Category="SyncPrediction|Reaction", meta=(ClampMin="0.0", Units="Centimeters"))
	float MaxInstantFinalCorrectionDistance = 35.0f;

	FSP_ReactionPredictionContext MakeReactionPredictionContext();

	void AddPendingPredictedReaction(const FSP_ReactionPredictionContext& Context, AActor* TargetActor,
		FGameplayTag ReactionTag);

	bool ConsumePendingPredictedReaction(const FSP_ReactionPredictionContext& Context, AActor* TargetActor,
		FGameplayTag ReactionTag);

	void RemoveExpiredPendingPredictedReactions();

	void AddDeferredPredictedReactionCorrection(const FSP_ReactionPredictionContext& Context, AActor* TargetActor,
		FGameplayTag ReactionTag);

	bool ConsumeDeferredPredictedReactionCorrection(const FSP_ReactionPredictionContext& Context, AActor* TargetActor,
		FGameplayTag ReactionTag);

	void RemoveExpiredDeferredPredictedReactionCorrections();

	bool PlayReactionMontageOnActor(AActor* TargetActor, const FSP_ReactionDataEntry& Reaction, float StartPosition,
		bool bForceRestart) const;

	float GetReactionStartPosition(const FSP_ReactionDataEntry& Reaction) const;

	float GetServerWorldTimeSecondsSafe() const;
};
