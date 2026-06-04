#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SyncAbilityMotionCharacterMovementComponent.generated.h"

UCLASS()
class SYNCABILITYMOTION_API USyncAbilityMotionCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	void SetAbilityRootMotionSuppressed(bool bInSuppressed);
	bool IsAbilityRootMotionSuppressed() const { return bAbilityRootMotionSuppressed; }
	void RefreshAbilityRootMotionMode();

	void SetAbilityMovementInputSuppressed(bool bInSuppressed);
	bool IsAbilityMovementInputSuppressed() const { return bAbilityMovementInputSuppressed; }

	void SetAbilityRootMotionPausedByCharacterImpact(bool bInPaused);
	bool IsAbilityRootMotionPausedByCharacterImpact() const { return bAbilityRootMotionPausedByCharacterImpact; }

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FVector ScaleInputAcceleration(const FVector& InputAcceleration) const override;
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice = 0.f,
		const FVector& MoveDelta = FVector::ZeroVector) override;

private:
	UPROPERTY(Transient)
	bool bAbilityRootMotionSuppressed = false;

	UPROPERTY(Transient)
	bool bAbilityMovementInputSuppressed = false;

	UPROPERTY(Transient)
	bool bAbilityRootMotionPausedByCharacterImpact = false;
};
