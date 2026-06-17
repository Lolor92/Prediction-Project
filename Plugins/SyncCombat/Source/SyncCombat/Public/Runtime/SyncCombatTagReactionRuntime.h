// Copyright ProjectLogos

#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "CoreMinimal.h"
#include "Data/SyncCombatTagReactionData.h"

class AActor;
class UAbilitySystemComponent;
class UAnimInstance;
class UGameplayEffect;
class USyncCombatComponent;
class USyncCombatTagReactionData;
struct FSyncCombatAnimBoolBinding;
struct FTimerHandle;

class FSyncCombatTagReactionRuntime
{
public:
	explicit FSyncCombatTagReactionRuntime(USyncCombatComponent& InCombatComponent);
	~FSyncCombatTagReactionRuntime();

	void Initialize(AActor* InOwnerActor, UAbilitySystemComponent* InAbilitySystemComponent,
		USyncCombatTagReactionData* InTagReactionData, TArray<FSyncCombatAnimBoolBinding>& InAnimBoolBindings);
	void Deinitialize();
	void PredictReactionVisuals(const FGameplayTagContainer& TriggerTags, const FGameplayTagContainer& AbilityTags);

private:
	void BindTagReactionEvents();
	void ClearTagReactionEvents();
	void OnReactionTagChanged(FGameplayTag Tag, int32 NewCount);
	bool DoesReactionPassOwnerTagRequirements(const FSyncCombatTagReactionBinding& Binding) const;
	void CacheAnimBoolBindings();
	void SetAnimBool(const FSyncCombatAnimBoolBinding& Binding, bool bValue) const;
	bool IsAnimBoolActive(const FSyncCombatAnimBoolBinding& Binding) const;
	void QueueAbilityActivation(const FSyncCombatTagReactionBinding& Binding, FGameplayTag TriggeredTag);
	void QueueEffectApply(const FSyncCombatTagReactionBinding& Binding, FGameplayTag TriggeredTag);
	void QueueEffectRemove(const FSyncCombatTagReactionBinding& Binding, FGameplayTag TriggeredTag);
	FName GetRemoveTimerKey(const FSyncCombatTagReactionBinding& Binding, const FGameplayTag& TriggeredTag) const;
	void ExecuteDelayed(TFunction<void()> Function, float DelaySeconds, FTimerHandle& TimerHandle);
	FActiveGameplayEffectHandle ApplyEffectToSelf(const TSubclassOf<UGameplayEffect>& GameplayEffectClass, float Level) const;

	TWeakObjectPtr<USyncCombatComponent> CombatComponent;
	TWeakObjectPtr<AActor> OwningActor;
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	TWeakObjectPtr<UAnimInstance> AnimInstance;

	USyncCombatTagReactionData* TagReactionData = nullptr;
	TArray<FSyncCombatAnimBoolBinding>* AnimBoolBindings = nullptr;

	TMap<FGameplayTag, FDelegateHandle> TagReactionDelegateHandles;
	TMap<FGameplayTag, FTimerHandle> AbilityReactionTimers;
	TMap<FGameplayTag, FTimerHandle> ApplyEffectReactionTimers;
	TMap<FName, FTimerHandle> RemoveEffectReactionTimers;
};
