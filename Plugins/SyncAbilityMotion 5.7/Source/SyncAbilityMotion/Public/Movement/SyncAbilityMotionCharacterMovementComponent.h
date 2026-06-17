#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SyncAbilityMotionCharacterMovementComponent.generated.h"

UCLASS()
class SYNCABILITYMOTION_API USyncAbilityMotionCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	void SetAbilityRootMotionSuppressed(const bool bInSuppressed);
	bool IsAbilityRootMotionSuppressed() const { return bAbilityRootMotionSuppressed; }
	void RefreshAbilityRootMotionMode();

	void SetAbilityMovementInputSuppressed(bool bInSuppressed);
	bool IsAbilityMovementInputSuppressed() const { return bAbilityMovementInputSuppressed; }

	void SetAbilityRootMotionPausedByCharacterImpact(bool bInPaused);
	bool IsAbilityRootMotionPausedByCharacterImpact() const { return bAbilityRootMotionPausedByCharacterImpact; }

	virtual float GetMaxSpeed() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FVector ScaleInputAcceleration(const FVector& InputAcceleration) const override;
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void SmoothCorrection(const FVector& OldLocation, const FQuat& OldRotation,
		const FVector& NewLocation, const FQuat& NewRotation) override;
	virtual void ServerMoveHandleClientError(float ClientTimeStamp, float DeltaTime, const FVector& Accel,
		const FVector& RelativeClientLocation, UPrimitiveComponent* ClientMovementBase,
		FName ClientBaseBoneName, uint8 ClientMovementMode) override;
	virtual void ClientAdjustPosition_Implementation(float TimeStamp, FVector NewLoc, FVector NewVel,
		UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition,
		uint8 ServerMovementMode, TOptional<FRotator> OptionalRotation = TOptional<FRotator>()) override;
	virtual void ClientVeryShortAdjustPosition_Implementation(float TimeStamp, FVector NewLoc,
		UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition,
		uint8 ServerMovementMode) override;
	virtual void ClientAdjustRootMotionPosition_Implementation(float TimeStamp, float ServerMontageTrackPosition,
		FVector ServerLoc, FVector_NetQuantizeNormal ServerRotation, float ServerVelZ,
		UPrimitiveComponent* ServerBase, FName ServerBoneName, bool bHasBase, bool bBaseRelativePosition,
		uint8 ServerMovementMode) override;
	virtual void ClientAdjustRootMotionSourcePosition_Implementation(float TimeStamp,
		FRootMotionSourceGroup ServerRootMotion, bool bHasAnimRootMotion, float ServerMontageTrackPosition,
		FVector ServerLoc, FVector_NetQuantizeNormal ServerRotation, float ServerVelZ,
		UPrimitiveComponent* ServerBase, FName ServerBoneName, bool bHasBase, bool bBaseRelativePosition,
		uint8 ServerMovementMode) override;
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice = 0.f,
		const FVector& MoveDelta = FVector::ZeroVector) override;

private:
	UPROPERTY(Transient)
	bool bAbilityRootMotionSuppressed = false;

	UPROPERTY(Transient)
	bool bAbilityMovementInputSuppressed = false;

	UPROPERTY(Transient)
	bool bAbilityRootMotionPausedByCharacterImpact = false;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sync Ability Motion|Networking")
	float AbilityStopCorrectionSnapDistance = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Directional Speed", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0"))
	float BackwardSpeedMultiplier = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Directional Speed", meta = (ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "0.0"))
	float BackwardDotThreshold = -0.5f;
};
