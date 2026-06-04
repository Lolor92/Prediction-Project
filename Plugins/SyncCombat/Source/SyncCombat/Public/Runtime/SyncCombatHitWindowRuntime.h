// Copyright ProjectLogos

#pragma once

#include "CoreMinimal.h"
#include "Data/SyncCombatHitWindowTypes.h"
#include "UObject/ObjectKey.h"

class AActor;
class UAbilitySystemComponent;
struct FAnimNotifyEvent;
class UAnimNotifyState;
class UAnimSequenceBase;
class UGameplayEffect;
class USceneComponent;
class USkeletalMeshComponent;
class USyncCombatComponent;

struct FSyncCombatScopedHitWindowKey
{
	FObjectKey SourceActorKey;
	FObjectKey AbilityKey;
	FObjectKey AnimationKey;
	const FAnimNotifyEvent* NotifyEvent = nullptr;
	int16 ActivationPredictionKey = 0;
	int16 ActivationPredictionBase = 0;

	bool operator==(const FSyncCombatScopedHitWindowKey& Other) const
	{
		return SourceActorKey == Other.SourceActorKey
			&& AbilityKey == Other.AbilityKey
			&& AnimationKey == Other.AnimationKey
			&& NotifyEvent == Other.NotifyEvent
			&& ActivationPredictionKey == Other.ActivationPredictionKey
			&& ActivationPredictionBase == Other.ActivationPredictionBase;
	}
};

FORCEINLINE uint32 GetTypeHash(const FSyncCombatScopedHitWindowKey& Key)
{
	uint32 Hash = GetTypeHash(Key.SourceActorKey);
	Hash = HashCombine(Hash, GetTypeHash(Key.AbilityKey));
	Hash = HashCombine(Hash, GetTypeHash(Key.AnimationKey));
	Hash = HashCombine(Hash, PointerHash(Key.NotifyEvent));
	Hash = HashCombine(Hash, GetTypeHash(Key.ActivationPredictionKey));
	Hash = HashCombine(Hash, GetTypeHash(Key.ActivationPredictionBase));
	return Hash;
}

class FSyncCombatHitWindowRuntime
{
public:
	explicit FSyncCombatHitWindowRuntime(USyncCombatComponent& InCombatComponent);

	void Deinitialize();
	void Tick();
	void SetLastCombatReferenceActor(AActor* InActor);
	bool BeginHitDetectionWindow(const UAnimNotifyState* NotifyState, USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation, const FAnimNotifyEvent* NotifyEvent, float TotalDuration, FName TraceSocketName,
		const FSyncCombatHitWindowSettings& HitWindowSettings);
	void EndHitDetectionWindow(const UAnimNotifyState* NotifyState, USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation);

private:
	bool BuildScopedHitWindowKey(UAnimSequenceBase* Animation, const FAnimNotifyEvent* NotifyEvent,
		FSyncCombatScopedHitWindowKey& OutKey) const;
	void RunHitDebugQuery(const FTransform& StartTransform, const FTransform& EndTransform, bool bDrawDebug);
	void DebugSweepActiveHitWindow();
	void ResetActiveHitDebugWindow();

	FTransform GetHitTraceWorldTransform(USkeletalMeshComponent* MeshComp, FName SocketName,
		const FSyncCombatHitWindowShapeSettings& HitShapeSettings) const;

	void TryApplyHitGameplayEffects(AActor* HitActor, const FHitResult& HitResult);
	void ApplyHitWindowGameplayEffectListToTarget(
		AActor* HitActor,
		const FHitResult& HitResult,
		const TArray<FSyncCombatHitWindowGameplayEffect>& GameplayEffects
	) const;
	void ScheduleDelayedHitWindowGameplayEffects(
		AActor* HitActor,
		const FHitResult& HitResult,
		const TArray<FSyncCombatHitWindowDelayedGameplayEffect>& DelayedGameplayEffects
	) const;
	static void ApplyDelayedGameplayEffectToTarget(
		TWeakObjectPtr<UAbilitySystemComponent> SourceASC,
		TWeakObjectPtr<AActor> SourceActor,
		TWeakObjectPtr<AActor> TargetActor,
		const FHitResult HitResult,
		TSubclassOf<UGameplayEffect> GameplayEffectClass,
		float EffectLevel
	);
	bool ShouldApplyDamageGameplayEffects(
		bool bWasBlocked,
		bool bWasParried,
		bool bWasDodged,
		ESyncCombatHitWindowSuperArmorLevel TargetSuperArmorLevel
	) const;
	void ApplyActivationTransformEffects() const;
	void ApplyHitWindowTransformEffects(AActor* HitActor, bool bWasBlocked, bool bWasDodged, bool bHasSuperArmor) const;
	void ApplyHitWindowMovement(AActor* HitActor, ESyncCombatHitWindowTransformTriggerTiming InvocationTiming,
		bool bWasBlocked, bool bWasDodged, bool bHasSuperArmor) const;
	void ApplyHitWindowRotation(AActor* HitActor, ESyncCombatHitWindowTransformTriggerTiming InvocationTiming,
		bool bWasBlocked, bool bWasDodged, bool bHasSuperArmor) const;
	bool CanApplyHitWindowMovement(ESyncCombatHitWindowTransformTriggerTiming InvocationTiming,
		bool bWasBlocked, bool bWasDodged, bool bHasSuperArmor) const;
	bool CanApplyHitWindowRotation(ESyncCombatHitWindowTransformTriggerTiming InvocationTiming,
		bool bWasBlocked, bool bWasDodged, bool bHasSuperArmor) const;
	void ApplyMovementToActor(AActor* RecipientActor, AActor* ReferenceActor,
		const FSyncCombatHitWindowMovementSettings& MovementSettings) const;
	void ApplyRotationToActor(AActor* RecipientActor, AActor* ReferenceActor,
		const FSyncCombatHitWindowRotationSettings& RotationSettings) const;
	bool CalculateMovementTargetLocation(AActor* RecipientActor, AActor* ReferenceActor,
		const FSyncCombatHitWindowMovementSettings& MovementSettings, FVector& OutLocation) const;
	bool CalculateRotationTargetRotation(AActor* RecipientActor, AActor* ReferenceActor,
		const FSyncCombatHitWindowRotationSettings& RotationSettings, FRotator& OutRotation) const;
	void SendTargetClientTransformPrediction(AActor* HitActor, bool bWasBlocked, bool bWasDodged,
		bool bHasSuperArmor) const;
	AActor* ResolveTransformReferenceActor(ESyncCombatHitWindowReferenceActorSource ReferenceSource, AActor* HitActor,
		ESyncCombatHitWindowTransformTriggerTiming InvocationTiming) const;
	void ExecuteHitWindowGameplayCues(AActor* HitActor, const FHitResult* HitResult,
		ESyncCombatHitWindowCueTriggerTiming TriggerTiming) const;
	void ExecuteGameplayCueOnASC(UAbilitySystemComponent* ASC, UAbilitySystemComponent* TargetASC,
		const FSyncCombatHitWindowGameplayCue& Cue, const FHitResult* HitResult) const;
	void ExecuteLocalCameraShakeCue(const FSyncCombatHitWindowGameplayCue& Cue, const FHitResult* HitResult) const;
	bool ShouldExecuteLocalCameraShakeCue() const;
	FVector GetGameplayCueSpawnLocation(const FSyncCombatHitWindowGameplayCue& Cue, const FHitResult* HitResult) const;
	USceneComponent* GetGameplayCueAttachComponent(UAbilitySystemComponent* ASC, UAbilitySystemComponent* TargetASC,
		const FSyncCombatHitWindowGameplayCue& Cue, const FHitResult* HitResult) const;
	bool IsAttackBlocked(AActor* HitActor) const;
	bool IsAttackParried(AActor* HitActor) const;
	bool IsAttackDodged(AActor* HitActor) const;
	bool HasRequiredSuperArmor(AActor* HitActor) const;
	void ApplyDefenseGameplayEffects(AActor* HitActor, const FHitResult& HitResult,
		bool bWasBlocked, bool bWasParried, bool bWasDodged, bool bHasSuperArmor) const;
	void ApplyGameplayEffectToActor(AActor* RecipientActor, const TSubclassOf<UGameplayEffect>& GameplayEffectClass,
		float EffectLevel, const FHitResult* HitResult) const;

	static bool IsWithinBlockAngle(const AActor* DefenderActor, const AActor* AttackerActor, float BlockAngleDegrees);
	static bool DoesTransformTimingMatch(ESyncCombatHitWindowTransformTriggerTiming ConfiguredTiming,
		ESyncCombatHitWindowTransformTriggerTiming InvocationTiming);
	static USyncCombatComponent* FindCombatComponent(AActor* Actor);
	ESyncCombatHitWindowSuperArmorLevel GetTargetSuperArmorLevel(AActor* HitActor) const;

	USyncCombatComponent& CombatComponent;

	TMap<FObjectKey, FName> ActiveHitDetectionWindows;
	TMap<FObjectKey, int32> ActiveHitDetectionWindowCounts;

	TObjectPtr<USkeletalMeshComponent> ActiveHitDebugMesh = nullptr;
	FName ActiveHitDebugSocketName = NAME_None;
	FTransform PreviousHitDebugTransform = FTransform::Identity;
	bool bHitDebugWindowActive = false;
	bool bHasPreviousHitDebugLocation = false;
	TSet<FObjectKey> HitActorsThisWindow;
	TMap<FSyncCombatScopedHitWindowKey, TSet<FObjectKey>> ScopedHitActors;
	FSyncCombatScopedHitWindowKey ActiveScopedHitWindowKey;
	bool bActiveScopedHitWindowKeyValid = false;
	int32 ActiveHitDebugWindowDepth = 0;

	FSyncCombatHitWindowSettings ActiveHitWindowSettings;

	TWeakObjectPtr<AActor> LastCombatReferenceActor;
};
