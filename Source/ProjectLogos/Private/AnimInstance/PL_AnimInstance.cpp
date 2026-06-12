#include "AnimInstance/PL_AnimInstance.h"
#include "BlueprintLibrary/SCP_CombatPredictionBlueprintLibrary.h"
#include "GameFramework/Character.h"

UPL_AnimInstance::UPL_AnimInstance()
{
}

void UPL_AnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UPL_AnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	float LockedGroundSpeed = GroundSpeed;
	bool bLockedIsAccelerating = bIsAccelerating;
	float LockedMovementOffsetYaw = MovementOffsetYaw;
	bool bAppliedReactionLock = false;

	USCP_CombatPredictionBlueprintLibrary::ApplyPredictedTargetReactionAnimLock(
		Character.Get(),
		GroundSpeed,
		bIsAccelerating,
		MovementOffsetYaw,
		LockedGroundSpeed,
		bLockedIsAccelerating,
		LockedMovementOffsetYaw,
		bAppliedReactionLock);

	if (bAppliedReactionLock)
	{
		GroundSpeed = LockedGroundSpeed;
		bIsAccelerating = bLockedIsAccelerating;
		MovementOffsetYaw = LockedMovementOffsetYaw;
	}
}
