#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Types/SCP_PredictionTypes.h"
#include "SCP_CombatPredictionComponent.generated.h"

class UGameplayAbility;
class UAnimMontage;
class USCP_ReactionData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCP_PredictedHitSignature, const FSCP_PredictedHit&, Hit);

USTRUCT()
struct FSCP_ProcessedPredictionTarget
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY()
	int32 AbilityPredictionKey = 0;

	UPROPERTY()
	int32 AbilitySpecHandle = INDEX_NONE;

	UPROPERTY()
	int32 PredictionEventId = 0;
};

USTRUCT()
struct FSCP_PendingPredictedReaction
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY()
	int32 AbilityPredictionKey = 0;

	UPROPERTY()
	int32 AbilitySpecHandle = INDEX_NONE;

	UPROPERTY()
	int32 PredictionEventId = 0;
};

UCLASS(ClassGroup=(SyncCombatPrediction), meta=(BlueprintSpawnableComponent))
class SYNCCOMBATPREDICTION_API USCP_CombatPredictionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USCP_CombatPredictionComponent();

	UFUNCTION(BlueprintCallable, Category="Sync Combat Prediction|GAS")
	FSCP_CombatPredictionContext StartPredictionFromAbility(const UGameplayAbility* Ability);

	UFUNCTION(BlueprintCallable, Category="Sync Combat Prediction")
	void ClearActivePrediction();

	UFUNCTION(BlueprintPure, Category="Sync Combat Prediction")
	bool HasActivePrediction() const { return bHasActivePrediction; }

	UFUNCTION(BlueprintCallable, Category="Sync Combat Prediction|Reaction")
	bool IsPlayingPredictedTargetReaction() const { return LocalPredictedTargetReactionCount > 0; }

	UFUNCTION(BlueprintPure, Category="Sync Combat Prediction")
	FSCP_CombatPredictionContext GetActivePredictionContext() const { return ActivePredictionContext; }

	UFUNCTION(BlueprintCallable, Category="Sync Combat Prediction")
	FSCP_CombatPredictionContext BeginPredictionEvent();

	UFUNCTION(BlueprintPure, Category="Sync Combat Prediction")
	bool HasProcessedTarget(const FSCP_CombatPredictionContext& Context, AActor* TargetActor) const;

	UFUNCTION(BlueprintCallable, Category="Sync Combat Prediction")
	bool MarkTargetProcessed(const FSCP_CombatPredictionContext& Context, AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category="Sync Combat Prediction")
	bool ReportHit(
		const FSCP_CombatPredictionContext& Context,
		const FHitResult& HitResult,
		FGameplayTag ReactionTag,
		const FSCP_GameplayEffectSettings& GameplayEffects);

	UFUNCTION(BlueprintCallable, Category="Sync Combat Prediction")
	bool ReportHitWithTransformSettings(
		const FSCP_CombatPredictionContext& Context,
		const FHitResult& HitResult,
		FGameplayTag ReactionTag,
		const FSCP_GameplayEffectSettings& GameplayEffects,
		const FSCP_HitTransformSettings& TransformSettings);

	UFUNCTION(BlueprintCallable, Category="Sync Combat Prediction")
	bool ReportHitWithSettings(
		const FSCP_CombatPredictionContext& Context,
		const FHitResult& HitResult,
		FGameplayTag ReactionTag,
		const FSCP_GameplayEffectSettings& GameplayEffects,
		const FSCP_HitTransformSettings& TransformSettings,
		const FSCP_HitDefenseSettings& DefenseSettings,
		const FSCP_HitDamageDefenseSettings& DamageDefenseSettings);

	UFUNCTION(BlueprintCallable, Category="Sync Combat Prediction|Reaction")
	bool PredictTargetReaction(const FSCP_PredictedHit& Hit, UAnimMontage* ReactionMontage);

	UFUNCTION(BlueprintCallable, Category="Sync Combat Prediction|Reaction")
	bool PredictTargetReactionFromHit(const FSCP_PredictedHit& Hit);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Sync Combat Prediction|Reaction")
	void ServerConfirmTargetReaction(
		FSCP_CombatPredictionContext Context,
		AActor* TargetActor,
		UAnimMontage* ReactionMontage);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Sync Combat Prediction|Reaction")
	void ServerConfirmTargetReactionWithTransform(
		FSCP_CombatPredictionContext Context,
		AActor* TargetActor,
		UAnimMontage* ReactionMontage,
		FSCP_HitTransformSettings TransformSettings);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Sync Combat Prediction|Reaction")
	void ServerConfirmTargetReactionWithSettings(
		FSCP_CombatPredictionContext Context,
		AActor* TargetActor,
		UAnimMontage* ReactionMontage,
		FSCP_HitTransformSettings TransformSettings,
		FSCP_HitDefenseSettings DefenseSettings);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayConfirmedTargetReaction(
		FSCP_CombatPredictionContext Context,
		AActor* TargetActor,
		UAnimMontage* ReactionMontage,
		bool bCancelActiveAbilityOnCleanHit,
		float ServerStartTime);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayConfirmedTargetReactionWithTransform(
		FSCP_CombatPredictionContext Context,
		AActor* InstigatorActor,
		AActor* TargetActor,
		UAnimMontage* ReactionMontage,
		bool bCancelActiveAbilityOnCleanHit,
		float ServerStartTime,
		FSCP_HitTransformSettings TransformSettings,
		FSCP_HitDefenseSettings DefenseSettings);

	UFUNCTION(Client, Reliable)
	void ClientPlayOwnerTargetReaction(UAnimMontage* ReactionMontage, bool bCancelActiveAbilityOnCleanHit);

	UFUNCTION(Client, Reliable)
	void ClientPlayOwnerTargetReactionWithTransform(
		AActor* InstigatorActor,
		UAnimMontage* ReactionMontage,
		bool bCancelActiveAbilityOnCleanHit,
		FSCP_HitTransformSettings TransformSettings,
		FSCP_HitDefenseSettings DefenseSettings);

	UPROPERTY(BlueprintAssignable, Category="Sync Combat Prediction")
	FSCP_PredictedHitSignature OnPredictedHit;

	UPROPERTY(BlueprintAssignable, Category="Sync Combat Prediction")
	FSCP_PredictedHitSignature OnAuthorityHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sync Combat Prediction|Debug")
	bool bLogReportedHits = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Reaction")
	TObjectPtr<USCP_ReactionData> ReactionData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sync Combat Prediction|Reaction")
	bool bAutoPredictReactions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sync Combat Prediction|Reaction")
	bool bAutoConfirmAuthorityReactions = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sync Combat Prediction|Defense")
	FGameplayTag BlockingTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sync Combat Prediction|Defense")
	FGameplayTag DodgingTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sync Combat Prediction|Defense")
	FGameplayTag SuperArmorTag1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sync Combat Prediction|Defense")
	FGameplayTag SuperArmorTag2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sync Combat Prediction|Defense")
	FGameplayTag SuperArmorTag3;

private:
	static constexpr int32 MaxPredictionEventId = 32767;
	static constexpr int32 MaxProcessedTargets = 64;
	static constexpr int32 MaxPendingReactions = 32;

	UPROPERTY(Transient)
	FSCP_CombatPredictionContext ActivePredictionContext;

	UPROPERTY(Transient)
	bool bHasActivePrediction = false;

	UPROPERTY(Transient)
	int32 NextPredictionEventId = 0;

	UPROPERTY(Transient)
	TArray<FSCP_ProcessedPredictionTarget> ProcessedTargets;

	UPROPERTY(Transient)
	TArray<FSCP_PendingPredictedReaction> PendingPredictedReactions;

	bool bSavedIgnoreClientMovementErrorChecksAndCorrection = false;
	bool bSavedServerAcceptClientAuthoritativePosition = false;
	int32 MovementCorrectionToleranceCount = 0;
	int32 MovementCorrectionAcceptClientAuthoritativeCount = 0;
	int32 LocalPredictedTargetReactionCount = 0;

	int32 AllocatePredictionEventId();
	void ResetPredictionEvents();
	FString BuildActorDebugString(const AActor* Actor) const;
	void AddPendingPredictedReaction(const FSCP_CombatPredictionContext& Context, AActor* TargetActor);
	bool ConsumePendingPredictedReaction(const FSCP_CombatPredictionContext& Context, AActor* TargetActor);
	bool DoesReactionKeyMatch(
		const FSCP_PendingPredictedReaction& Entry,
		const FSCP_CombatPredictionContext& Context,
		const AActor* TargetActor) const;
	bool PlayReactionMontageOnActor(
		AActor* TargetActor,
		UAnimMontage* ReactionMontage,
		float StartPosition,
		bool bForceRestart,
		bool bCancelActiveAbilityOnCleanHit) const;
	void CancelTargetAnimatingAbilityForReaction(AActor* TargetActor) const;
	bool ShouldCancelActiveAbilityForReaction(FGameplayTag ReactionTag, const UAnimMontage* ReactionMontage) const;
	void ConfirmTargetReaction(
		FSCP_CombatPredictionContext Context,
		AActor* TargetActor,
		UAnimMontage* ReactionMontage,
		const FSCP_HitTransformSettings& TransformSettings,
		const FSCP_HitDefenseSettings& DefenseSettings);
	void ApplyGameplayEffectsFromHit(const FSCP_PredictedHit& Hit);
	void ApplyReactionDataTargetEffectsFromHit(const FSCP_PredictedHit& Hit);
	void ApplyEffectClassesToActor(
		AActor* TargetActor,
		const TArray<TSubclassOf<UGameplayEffect>>& EffectClasses,
		float Level,
		const FGameplayEffectContextHandle& EffectContext,
		const FSCP_GameplayEffectSettings* DamageSettings = nullptr) const;
	void ApplyHitTransformEffects(
		AActor* InstigatorActor,
		AActor* TargetActor,
		const FSCP_HitTransformSettings& TransformSettings,
		const FSCP_HitDefenseSettings& DefenseSettings) const;
	void ApplyHitMovement(
		AActor* InstigatorActor,
		AActor* TargetActor,
		const FSCP_HitMovementSettings& MovementSettings,
		const FSCP_HitDefenseSettings& DefenseSettings) const;
	void ApplyHitRotation(
		AActor* InstigatorActor,
		AActor* TargetActor,
		const FSCP_HitRotationSettings& RotationSettings,
		const FSCP_HitDefenseSettings& DefenseSettings) const;
	void ApplyMovementToActor(
		AActor* RecipientActor,
		AActor* ReferenceActor,
		const FSCP_HitMovementSettings& MovementSettings) const;
	void ApplyRotationToActor(
		AActor* RecipientActor,
		AActor* ReferenceActor,
		const FSCP_HitRotationSettings& RotationSettings) const;
	bool CalculateMovementTargetLocation(
		AActor* RecipientActor,
		AActor* ReferenceActor,
		const FSCP_HitMovementSettings& MovementSettings,
		FVector& OutLocation) const;
	bool CalculateRotationTargetRotation(
		AActor* RecipientActor,
		AActor* ReferenceActor,
		const FSCP_HitRotationSettings& RotationSettings,
		FRotator& OutRotation) const;
	static bool DoesTransformTimingMatch(
		ESCP_HitTransformTriggerTiming ConfiguredTiming,
		ESCP_HitTransformTriggerTiming InvocationTiming);
	static bool TransformRecipientIncludesTarget(ESCP_HitTransformRecipient Recipient);
	static AActor* ResolveTransformReferenceActor(
		ESCP_HitTransformReferenceActorSource ReferenceSource,
		AActor* InstigatorActor,
		AActor* TargetActor);
	bool IsAttackBlocked(AActor* InstigatorActor, AActor* TargetActor, const FSCP_HitDefenseSettings& DefenseSettings) const;
	bool IsAttackDodged(AActor* TargetActor, const FSCP_HitDefenseSettings& DefenseSettings) const;
	bool HasRequiredSuperArmor(AActor* TargetActor, const FSCP_HitDefenseSettings& DefenseSettings) const;
	ESCP_HitSuperArmorLevel GetTargetSuperArmorLevel(AActor* TargetActor) const;
	bool ShouldApplyCleanHitReaction(AActor* InstigatorActor, AActor* TargetActor, const FSCP_HitDefenseSettings& DefenseSettings) const;
	bool ShouldApplyDamageEffects(AActor* InstigatorActor, AActor* TargetActor, const FSCP_HitDefenseSettings& DefenseSettings,
		const FSCP_HitDamageDefenseSettings& DamageDefenseSettings) const;
	static bool IsWithinBlockAngle(const AActor* DefenderActor, const AActor* AttackerActor, float BlockAngleDegrees);
	void BeginPredictedTargetReaction(float Duration);
	void EndPredictedTargetReaction();
	void BeginTargetReactionMovementTolerance(float Duration);
	void BeginMovementCorrectionTolerance(float Duration, bool bAcceptClientAuthoritativePosition);
	void EndMovementCorrectionTolerance(bool bAcceptClientAuthoritativePosition);
};
