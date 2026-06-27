#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SP_CharacterMovementComponent.generated.h"

UCLASS()
class SYNCPREDICTION_API USP_CharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	void SetRootMotionContactBlock(bool bBlocked, const FVector& InBlockedDirection, float Duration = 0.0f);
	void ClearRootMotionContactBlock();

	bool IsRootMotionContactBlocked() const { return bSPRootMotionContactBlocked; }

protected:
	virtual void ApplyRootMotionToVelocity(float DeltaTime) override;

	virtual FVector CalcAnimRootMotionVelocity(
		const FVector& RootMotionDeltaMove,
		float DeltaSeconds,
		const FVector& CurrentVelocity) const override;

private:
	FVector RemoveVelocityIntoRootMotionContactBlock(const FVector& InVelocity, const TCHAR* LogContext) const;

	UPROPERTY(Transient)
	bool bSPRootMotionContactBlocked = false;

	UPROPERTY(Transient)
	FVector SPBlockedRootMotionDirection = FVector::ZeroVector;

	FTimerHandle SPRootMotionContactBlockTimerHandle;

	double SPRootMotionContactBlockStartTime = -1.0;

	mutable double LastSPRootMotionContactClampLogTime = -1000.0;
};
