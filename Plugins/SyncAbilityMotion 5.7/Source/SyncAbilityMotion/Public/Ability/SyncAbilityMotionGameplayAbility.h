#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Data/SyncAbilityMotionTypes.h"
#include "TimerManager.h"
#include "SyncAbilityMotionGameplayAbility.generated.h"

class ACharacter;
class UAbilitySystemComponent;

USTRUCT()
struct FSyncAbilityMotionGameplayEffectWindowRuntime
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHasEntered = false;

	UPROPERTY()
	bool bHasExited = false;
};

UCLASS()
class SYNCABILITYMOTION_API USyncAbilityMotionGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USyncAbilityMotionGameplayAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	UPROPERTY(EditDefaultsOnly, Category="Ability|Montage", meta=(DisplayName="Montage Lockout"))
	FSyncAbilityMotionMontageLockout MontageLockout;

	UPROPERTY(EditDefaultsOnly, Category="Ability|Activation")
	bool bBypassReactionActivationSuppression = false;

	UPROPERTY(EditDefaultsOnly, Category="Ability|Gameplay Effects", meta=(DisplayName="Montage Percent Effect Windows"))
	TArray<FSyncAbilityMotionGameplayEffectWindow> GameplayEffectWindows;

	UPROPERTY(EditDefaultsOnly, Category="Ability|Gameplay Effects", meta=(ClampMin="0.01", Units="Seconds"))
	float GameplayEffectWindowUpdateInterval = 0.05f;

	UFUNCTION(BlueprintPure, Category="Ability|Combo")
	TSubclassOf<UGameplayAbility> GetComboAbilityClass() const { return ComboAbilityClass; }

	UFUNCTION(BlueprintPure, Category="Ability|Combo")
	float GetComboWindowDuration() const { return ComboWindowDuration; }

	UFUNCTION(BlueprintPure, Category="Ability|Combo")
	bool IsComboWindowOpen() const { return bComboWindowOpen; }

	UFUNCTION(BlueprintPure, Category="Ability|Tags")
	bool DoesAbilityUseGameplayTag(FGameplayTag GameplayTag) const;

	uint32 GetActivationSequenceId() const { return ActivationSequenceId; }
	bool ShouldPauseRootMotionForCharacterCollision(const ACharacter* Character) const;
	bool ShouldPauseRootMotionOnCharacterImpact() const { return bPauseRootMotionOnCharacterImpact; }

protected:
	bool CanInterruptAnimatingAbility(const FGameplayAbilityActorInfo* ActorInfo) const;
	void InterruptOtherActiveAbilities() const;

	UFUNCTION(BlueprintCallable, Category="Ability|Rotation")
	void RotateAvatarToControllerYawOnActivate() const;

	UPROPERTY(EditDefaultsOnly, Category="Ability|Rotation")
	bool bRotateToControllerYawOnActivate = false;

	UPROPERTY(EditDefaultsOnly, Category="Ability|Interrupt")
	bool bInterruptOtherAbilitiesOnActivate = false;

	UPROPERTY(Transient)
	bool bComboSupportAvailable = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Combo",
		meta=(EditCondition="bComboSupportAvailable"))
	TSubclassOf<UGameplayAbility> ComboAbilityClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Combo",
		meta=(EditCondition="bComboSupportAvailable", ClampMin="0.0", Units="Seconds"))
	float ComboWindowDuration = 2.f;

	UFUNCTION(BlueprintCallable, Category="Ability|Combo")
	void OpenComboWindow();

	UFUNCTION(BlueprintCallable, Category="Ability|Combo")
	void CloseComboWindow();

	UPROPERTY(EditDefaultsOnly, Category="Ability|Root Motion")
	bool bPauseRootMotionOnCharacterCollision = true;

	UPROPERTY(EditDefaultsOnly, Category="Ability|Root Motion")
	bool bPauseRootMotionOnCharacterImpact = true;

	UPROPERTY(EditDefaultsOnly, Category="Ability|Root Motion", meta=(EditCondition="bPauseRootMotionOnCharacterCollision",
		ClampMin="0.0", ClampMax="180.0", UIMin="0.0", UIMax="180.0", Units="Degrees"))
	float RootMotionCharacterCollisionForwardAngleDegrees = 40.f;

	UPROPERTY(EditDefaultsOnly, Category="Ability|Root Motion", meta=(EditCondition="bPauseRootMotionOnCharacterCollision",
		ClampMin="0.0", UIMin="0.0", Units="cm"))
	float RootMotionCharacterCollisionProbeDistance = 25.f;

private:
	void ResetComboWindow();
	void ResetGameplayEffectWindows();
	void ScheduleGameplayEffectWindowUpdate();
	void ClearGameplayEffectWindowUpdate();
	void UpdateGameplayEffectWindows();
	void EnterGameplayEffectWindow(int32 WindowIndex);
	void ExitGameplayEffectWindow(int32 WindowIndex);
	void CleanupActiveGameplayEffectWindows();
	float GetCurrentMontagePlayedPercent() const;
	void ApplyGameplayEffectsToSelf(const TArray<TSubclassOf<UGameplayEffect>>& EffectClasses) const;
	void RemoveGameplayEffectsFromSelf(const TArray<TSubclassOf<UGameplayEffect>>& EffectClasses) const;

	FTimerHandle ComboWindowTimerHandle;
	FTimerHandle GameplayEffectWindowTimerHandle;
	bool bComboWindowOpen = false;

	uint32 ActivationSequenceId = 0;

	UPROPERTY(Transient)
	TArray<FSyncAbilityMotionGameplayEffectWindowRuntime> GameplayEffectWindowRuntime;
};
