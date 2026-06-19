#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimTypes.h"
#include "Engine/HitResult.h"
#include "TimerManager.h"
#include "SP_GameplayAbility.generated.h"

class ACharacter;
class AActor;
class UPrimitiveComponent;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SyncPrediction|RootMotion Contact", meta=(EditCondition="bStopRootMotionOnPawnContact", ClampMin="0.0", Units="Centimeters"))
	float RootMotionContactCapsuleInflation = 4.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SyncPrediction|RootMotion Contact", meta=(EditCondition="bStopRootMotionOnPawnContact", ClampMin="0.01", Units="Seconds"))
	float RootMotionContactReleaseCheckInterval = 0.03f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SyncPrediction|RootMotion Contact", meta=(EditCondition="bStopRootMotionOnPawnContact", ClampMin="0.0", Units="Seconds"))
	float RootMotionContactReleaseDelay = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SyncPrediction|RootMotion Contact", meta=(EditCondition="bStopRootMotionOnPawnContact", ClampMin="0.0", Units="Centimeters"))
	float RootMotionContactReleaseExtraTolerance = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SyncPrediction|RootMotion Contact", meta=(EditCondition="bStopRootMotionOnPawnContact"))
	bool bClearVelocityWhenRootMotionStops = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SyncPrediction|RootMotion Contact", meta=(EditCondition="bStopRootMotionOnPawnContact"))
	bool bBlockMovementInputWhenRootMotionStops = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SyncPrediction|RootMotion Contact", meta=(EditCondition="bBlockMovementInputWhenRootMotionStops"))
	bool bRestoreMovementInputWhenAbilityEnds = true;

private:
	UPROPERTY()
	TWeakObjectPtr<ACharacter> CachedCharacter;

	UPROPERTY()
	TWeakObjectPtr<AActor> RootMotionContactBlockingActor;

	FTimerHandle RootMotionContactReleaseTimerHandle;
	FTimerHandle RootMotionContactReleaseDelayTimerHandle;

	ERootMotionMode::Type PreviousRootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;

	bool bRootMotionStoppedByContact = false;
	bool bMovementInputBlockedByContact = false;
	bool bContactEventBound = false;

	void StartRootMotionContactCheck();
	void StopRootMotionContactCheck();

	void CheckInitialRootMotionContact();
	void StartRootMotionContactReleaseCheck();
	void StopRootMotionContactReleaseCheck();
	void CheckRootMotionContactRelease();
	void StartRootMotionContactReleaseDelay();
	void CancelRootMotionContactReleaseDelay();

	UFUNCTION()
	void HandleCapsuleHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	bool IsValidRootMotionStopContact(ACharacter* Character, AActor* OtherActor, float& OutContactAngle) const;
	bool IsStillInRootMotionStopContact(ACharacter* Character, AActor* BlockingActor) const;
	bool ShouldStopRootMotionForContact(ACharacter* Character, AActor*& OutBlockingActor, float& OutContactAngle) const;

	void StopRootMotionFromContact(AActor* BlockingActor, float ContactAngle);
	void RestoreRootMotionFromContactRelease();
	void RestoreRootMotionMode();
	void BlockMovementInputFromContact();
	void RestoreMovementInputFromContact();
};
