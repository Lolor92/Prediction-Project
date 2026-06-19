#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimTypes.h"
#include "TimerManager.h"
#include "SP_GameplayAbility.generated.h"

class ACharacter;
class AActor;

UCLASS()
class SYNCPREDICTION_API USP_GameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USP_GameplayAbility();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SyncPrediction|RootMotion Contact")
	bool bStopRootMotionOnPawnContact = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SyncPrediction|RootMotion Contact", meta=(EditCondition="bStopRootMotionOnPawnContact", ClampMin="0.0", ClampMax="180.0", Units="Degrees"))
	float RootMotionStopContactAngleDegrees = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SyncPrediction|RootMotion Contact", meta=(EditCondition="bStopRootMotionOnPawnContact", ClampMin="0.001", Units="Seconds"))
	float RootMotionContactCheckInterval = 0.01f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SyncPrediction|RootMotion Contact", meta=(EditCondition="bStopRootMotionOnPawnContact", ClampMin="0.0", Units="Centimeters"))
	float RootMotionContactCapsuleInflation = 4.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SyncPrediction|RootMotion Contact", meta=(EditCondition="bStopRootMotionOnPawnContact"))
	bool bClearVelocityWhenRootMotionStops = true;

private:
	UPROPERTY()
	TWeakObjectPtr<ACharacter> CachedCharacter;

	FTimerHandle RootMotionContactTimerHandle;

	ERootMotionMode::Type PreviousRootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;

	bool bRootMotionStoppedByContact = false;

	void StartRootMotionContactCheck();
	void StopRootMotionContactCheck();

	void CheckRootMotionContact();
	bool ShouldStopRootMotionForContact(ACharacter* Character, AActor*& OutBlockingActor, float& OutContactAngle) const;

	void StopRootMotionFromContact(AActor* BlockingActor, float ContactAngle);
	void RestoreRootMotionMode();
};
