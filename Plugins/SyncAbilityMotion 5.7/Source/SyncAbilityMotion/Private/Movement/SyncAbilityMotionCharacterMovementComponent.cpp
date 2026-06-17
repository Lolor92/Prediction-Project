#include "Movement/SyncAbilityMotionCharacterMovementComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ability/SyncAbilityMotionGameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AnimInstance/SyncAbilityMotionAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSyncAbilityMotionMoveDiag, Log, All);

namespace SyncAbilityMotionFlags
{
	constexpr uint8 SuppressAbilityRootMotion = FSavedMove_Character::FLAG_Custom_0;
	constexpr uint8 SuppressAbilityMovementInput = FSavedMove_Character::FLAG_Custom_1;
	constexpr uint8 PauseAbilityRootMotion = FSavedMove_Character::FLAG_Custom_2;
}

namespace
{
	TAutoConsoleVariable<int32> CVarSyncAbilityMotionMovementDiagnostics(
		TEXT("sync.AbilityMotion.MovementDiagnostics"),
		0,
		TEXT("Enable SyncAbilityMotion movement correction/root-motion diagnostic logs."),
		ECVF_Default);

	bool IsSyncAbilityMotionMovementDiagnosticsEnabled()
	{
		return CVarSyncAbilityMotionMovementDiagnostics.GetValueOnGameThread() != 0;
	}

	const TCHAR* BoolText(const bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	FString DescribeMontageState(const ACharacter* Character)
	{
		const USkeletalMeshComponent* MeshComp = Character ? Character->GetMesh() : nullptr;
		const UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
		const UAnimMontage* Montage = AnimInstance ? AnimInstance->GetCurrentActiveMontage() : nullptr;
		const float MontagePosition = Montage ? AnimInstance->Montage_GetPosition(Montage) : -1.f;
		const float MontageLength = Montage ? Montage->GetPlayLength() : 0.f;
		const float MontagePercent = MontageLength > UE_KINDA_SMALL_NUMBER
			? (MontagePosition / MontageLength) * 100.f
			: -1.f;

		return FString::Printf(
			TEXT("Montage=%s MontagePos=%.3f MontagePct=%.2f"),
			*GetNameSafe(Montage),
			MontagePosition,
			MontagePercent);
	}

	FString DescribeMovementState(const USyncAbilityMotionCharacterMovementComponent* MoveComp)
	{
		const ACharacter* Character = MoveComp ? Cast<ACharacter>(MoveComp->GetOwner()) : nullptr;
		const FVector ActorLocation = Character ? Character->GetActorLocation() : FVector::ZeroVector;
		const FRotator ActorRotation = Character ? Character->GetActorRotation() : FRotator::ZeroRotator;

		return FString::Printf(
			TEXT("Character=%s Local=%s Authority=%s Loc=%s Rot=%s Vel=%s Accel=%s Mode=%d RootSuppressed=%s InputSuppressed=%s RootPausedImpact=%s HasAnimRootMotion=%s HasRootMotionSources=%s Smoothing=%d %s"),
			*GetNameSafe(Character),
			BoolText(Character && Character->IsLocallyControlled()),
			BoolText(Character && Character->HasAuthority()),
			*ActorLocation.ToCompactString(),
			*ActorRotation.ToCompactString(),
			MoveComp ? *MoveComp->Velocity.ToCompactString() : TEXT("None"),
			MoveComp ? *MoveComp->GetCurrentAcceleration().ToCompactString() : TEXT("None"),
			MoveComp ? static_cast<int32>(MoveComp->MovementMode) : -1,
			BoolText(MoveComp && MoveComp->IsAbilityRootMotionSuppressed()),
			BoolText(MoveComp && MoveComp->IsAbilityMovementInputSuppressed()),
			BoolText(MoveComp && MoveComp->IsAbilityRootMotionPausedByCharacterImpact()),
			BoolText(MoveComp && MoveComp->HasAnimRootMotion()),
			BoolText(MoveComp && MoveComp->HasRootMotionSources()),
			MoveComp ? static_cast<int32>(MoveComp->NetworkSmoothingMode) : -1,
			*DescribeMontageState(Character));
	}

	UAbilitySystemComponent* GetAbilitySystemComponent(ACharacter* Character)
	{
		if (!Character)
		{
			return nullptr;
		}

		if (APlayerState* PlayerState = Character->GetPlayerState())
		{
			if (UAbilitySystemComponent* ASC =
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerState))
			{
				return ASC;
			}
		}

		return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Character);
	}

	bool ShouldPauseRootMotionOnCharacterImpact(const USyncAbilityMotionCharacterMovementComponent* MoveComp)
	{
		ACharacter* Character = MoveComp ? Cast<ACharacter>(MoveComp->GetOwner()) : nullptr;
		const UAbilitySystemComponent* ASC = GetAbilitySystemComponent(Character);
		const UGameplayAbility* AnimatingAbility = ASC ? ASC->GetAnimatingAbility() : nullptr;
		const USyncAbilityMotionGameplayAbility* Ability =
			Cast<USyncAbilityMotionGameplayAbility>(AnimatingAbility);
		return Ability && Ability->ShouldPauseRootMotionOnCharacterImpact();
	}
}

class FSavedMove_SyncAbilityMotion final : public FSavedMove_Character
{
public:
	typedef FSavedMove_Character Super;

	virtual void Clear() override
	{
		Super::Clear();

		bSavedAbilityRootMotionSuppressed = false;
		bSavedAbilityMovementInputSuppressed = false;
		bSavedAbilityRootMotionPaused = false;
	}

	virtual uint8 GetCompressedFlags() const override
	{
		uint8 Result = Super::GetCompressedFlags();

		if (bSavedAbilityRootMotionSuppressed)
		{
			Result |= SyncAbilityMotionFlags::SuppressAbilityRootMotion;
		}

		if (bSavedAbilityMovementInputSuppressed)
		{
			Result |= SyncAbilityMotionFlags::SuppressAbilityMovementInput;
		}

		if (bSavedAbilityRootMotionPaused)
		{
			Result |= SyncAbilityMotionFlags::PauseAbilityRootMotion;
		}

		return Result;
	}

	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override
	{
		const FSavedMove_SyncAbilityMotion* NewSyncMove =
			static_cast<const FSavedMove_SyncAbilityMotion*>(NewMove.Get());

		if (bSavedAbilityRootMotionSuppressed != NewSyncMove->bSavedAbilityRootMotionSuppressed)
		{
			return false;
		}

		if (bSavedAbilityMovementInputSuppressed != NewSyncMove->bSavedAbilityMovementInputSuppressed)
		{
			return false;
		}

		if (bSavedAbilityRootMotionPaused != NewSyncMove->bSavedAbilityRootMotionPaused)
		{
			return false;
		}

		return Super::CanCombineWith(NewMove, Character, MaxDelta);
	}

	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel,
		FNetworkPredictionData_Client_Character& ClientData) override
	{
		Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

		if (const USyncAbilityMotionCharacterMovementComponent* MoveComp =
			Cast<USyncAbilityMotionCharacterMovementComponent>(Character->GetCharacterMovement()))
		{
			bSavedAbilityRootMotionSuppressed = MoveComp->IsAbilityRootMotionSuppressed();
			bSavedAbilityMovementInputSuppressed = MoveComp->IsAbilityMovementInputSuppressed();
			bSavedAbilityRootMotionPaused = MoveComp->IsAbilityRootMotionPausedByCharacterImpact();
		}
	}

private:
	uint8 bSavedAbilityRootMotionSuppressed : 1 = false;
	uint8 bSavedAbilityMovementInputSuppressed : 1 = false;
	uint8 bSavedAbilityRootMotionPaused : 1 = false;
};

class FNetworkPredictionData_Client_SyncAbilityMotion final : public FNetworkPredictionData_Client_Character
{
public:
	explicit FNetworkPredictionData_Client_SyncAbilityMotion(const UCharacterMovementComponent& ClientMovement)
		: FNetworkPredictionData_Client_Character(ClientMovement)
	{
	}

	virtual FSavedMovePtr AllocateNewMove() override
	{
		return FSavedMovePtr(new FSavedMove_SyncAbilityMotion());
	}
};

void USyncAbilityMotionCharacterMovementComponent::SetAbilityRootMotionSuppressed(const bool bInSuppressed)
{
	if (bAbilityRootMotionSuppressed == bInSuppressed) return;

	const bool bBecameSuppressed = !bAbilityRootMotionSuppressed && bInSuppressed;

	if (IsSyncAbilityMotionMovementDiagnosticsEnabled())
	{
		UE_LOG(
			LogSyncAbilityMotionMoveDiag,
			Log,
			TEXT("SetAbilityRootMotionSuppressed New=%s Previous=%s %s"),
			BoolText(bInSuppressed),
			BoolText(bAbilityRootMotionSuppressed),
			*DescribeMovementState(this));
	}

	bAbilityRootMotionSuppressed = bInSuppressed;

	if (bBecameSuppressed)
	{
		StopMovementImmediately();
		ClearAccumulatedForces();

		Acceleration = FVector::ZeroVector;
		LastUpdateVelocity = FVector::ZeroVector;

		CurrentRootMotion.Clear();

		if (CharacterOwner)
		{
			CharacterOwner->ForceNetUpdate();
		}
	}

	if (!bInSuppressed)
	{
		SetAbilityRootMotionPausedByCharacterImpact(false);
	}

	RefreshAbilityRootMotionMode();
}

void USyncAbilityMotionCharacterMovementComponent::RefreshAbilityRootMotionMode()
{
	if (!CharacterOwner) return;

	USkeletalMeshComponent* MeshComp = CharacterOwner->GetMesh();
	if (!MeshComp) return;

	USyncAbilityMotionAnimInstance* AnimInstance =
		Cast<USyncAbilityMotionAnimInstance>(MeshComp->GetAnimInstance());
	if (!AnimInstance) return;

	const bool bRootMotionEnabled = !bAbilityRootMotionSuppressed;
	AnimInstance->bRootMotionEnabled = bRootMotionEnabled;
	AnimInstance->SetRootMotionMode(bRootMotionEnabled
		? ERootMotionMode::RootMotionFromMontagesOnly
		: ERootMotionMode::IgnoreRootMotion);
}

void USyncAbilityMotionCharacterMovementComponent::SetAbilityMovementInputSuppressed(bool bInSuppressed)
{
	if (bAbilityMovementInputSuppressed == bInSuppressed) return;

	if (IsSyncAbilityMotionMovementDiagnosticsEnabled())
	{
		UE_LOG(
			LogSyncAbilityMotionMoveDiag,
			Log,
			TEXT("SetAbilityMovementInputSuppressed New=%s Previous=%s %s"),
			BoolText(bInSuppressed),
			BoolText(bAbilityMovementInputSuppressed),
			*DescribeMovementState(this));
	}

	bAbilityMovementInputSuppressed = bInSuppressed;
}

void USyncAbilityMotionCharacterMovementComponent::SetAbilityRootMotionPausedByCharacterImpact(bool bInPaused)
{
	if (bAbilityRootMotionPausedByCharacterImpact == bInPaused) return;

	if (IsSyncAbilityMotionMovementDiagnosticsEnabled())
	{
		UE_LOG(
			LogSyncAbilityMotionMoveDiag,
			Log,
			TEXT("SetAbilityRootMotionPausedByCharacterImpact New=%s Previous=%s %s"),
			BoolText(bInPaused),
			BoolText(bAbilityRootMotionPausedByCharacterImpact),
			*DescribeMovementState(this));
	}

	bAbilityRootMotionPausedByCharacterImpact = bInPaused;
}

void USyncAbilityMotionCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	const bool bIsLocallyControlled = CharacterOwner && CharacterOwner->IsLocallyControlled();
	if (!bIsLocallyControlled)
	{
		const bool bNewRootMotionSuppressed = (Flags & SyncAbilityMotionFlags::SuppressAbilityRootMotion) != 0;
		const bool bNewInputSuppressed = (Flags & SyncAbilityMotionFlags::SuppressAbilityMovementInput) != 0;
		const bool bNewAbilityRootMotionPaused = (Flags & SyncAbilityMotionFlags::PauseAbilityRootMotion) != 0;

		SetAbilityRootMotionSuppressed(bNewRootMotionSuppressed);
		SetAbilityMovementInputSuppressed(bNewInputSuppressed);
		SetAbilityRootMotionPausedByCharacterImpact(bNewAbilityRootMotionPaused);
	}
}

FNetworkPredictionData_Client* USyncAbilityMotionCharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr);

	if (ClientPredictionData == nullptr)
	{
		USyncAbilityMotionCharacterMovementComponent* MutableThis =
			const_cast<USyncAbilityMotionCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_SyncAbilityMotion(*this);
	}

	return ClientPredictionData;
}

void USyncAbilityMotionCharacterMovementComponent::SmoothCorrection(
	const FVector& OldLocation,
	const FQuat& OldRotation,
	const FVector& NewLocation,
	const FQuat& NewRotation)
{
	const float CorrectionDist2D = FVector::Dist2D(OldLocation, NewLocation);

	if (IsSyncAbilityMotionMovementDiagnosticsEnabled())
	{
		UE_LOG(
			LogSyncAbilityMotionMoveDiag,
			Warning,
			TEXT("SmoothCorrection Dist=%.2f OldLoc=%s NewLoc=%s OldRot=%s NewRot=%s %s"),
			CorrectionDist2D,
			*OldLocation.ToCompactString(),
			*NewLocation.ToCompactString(),
			*OldRotation.Rotator().ToCompactString(),
			*NewRotation.Rotator().ToCompactString(),
			*DescribeMovementState(this));
	}

	const bool bIsSimulatedProxy =
		CharacterOwner &&
		!CharacterOwner->IsLocallyControlled() &&
		!CharacterOwner->HasAuthority();

	const bool bInAbilityStopWindow =
		bAbilityRootMotionSuppressed ||
		bAbilityMovementInputSuppressed ||
		bAbilityRootMotionPausedByCharacterImpact;

	const bool bShouldSnapSmallAbilityCorrection =
		bIsSimulatedProxy &&
		bInAbilityStopWindow &&
		CorrectionDist2D <= AbilityStopCorrectionSnapDistance;

	if (bShouldSnapSmallAbilityCorrection)
	{
		const ENetworkSmoothingMode SavedSmoothingMode = NetworkSmoothingMode;
		NetworkSmoothingMode = ENetworkSmoothingMode::Disabled;

		Super::SmoothCorrection(OldLocation, OldRotation, NewLocation, NewRotation);

		NetworkSmoothingMode = SavedSmoothingMode;
		return;
	}

	Super::SmoothCorrection(OldLocation, OldRotation, NewLocation, NewRotation);
}

void USyncAbilityMotionCharacterMovementComponent::ServerMoveHandleClientError(
	float ClientTimeStamp,
	float DeltaTime,
	const FVector& Accel,
	const FVector& RelativeClientLocation,
	UPrimitiveComponent* ClientMovementBase,
	FName ClientBaseBoneName,
	uint8 ClientMovementMode)
{
	if (IsSyncAbilityMotionMovementDiagnosticsEnabled())
	{
		const FVector ServerLocation = CharacterOwner ? CharacterOwner->GetActorLocation() : FVector::ZeroVector;
		UE_LOG(
			LogSyncAbilityMotionMoveDiag,
			Warning,
			TEXT("ServerMoveHandleClientError ClientTime=%.3f DeltaTime=%.3f ClientLoc=%s ServerLoc=%s Dist=%.2f ClientAccel=%s ClientMode=%d Base=%s Bone=%s %s"),
			ClientTimeStamp,
			DeltaTime,
			*RelativeClientLocation.ToCompactString(),
			*ServerLocation.ToCompactString(),
			FVector::Dist(RelativeClientLocation, ServerLocation),
			*Accel.ToCompactString(),
			static_cast<int32>(ClientMovementMode),
			*GetNameSafe(ClientMovementBase),
			*ClientBaseBoneName.ToString(),
			*DescribeMovementState(this));
	}

	Super::ServerMoveHandleClientError(
		ClientTimeStamp,
		DeltaTime,
		Accel,
		RelativeClientLocation,
		ClientMovementBase,
		ClientBaseBoneName,
		ClientMovementMode);
}

void USyncAbilityMotionCharacterMovementComponent::ClientAdjustPosition_Implementation(
	float TimeStamp,
	FVector NewLoc,
	FVector NewVel,
	UPrimitiveComponent* NewBase,
	FName NewBaseBoneName,
	bool bHasBase,
	bool bBaseRelativePosition,
	uint8 ServerMovementMode,
	TOptional<FRotator> OptionalRotation)
{
	if (IsSyncAbilityMotionMovementDiagnosticsEnabled())
	{
		const FVector ClientLocation = CharacterOwner ? CharacterOwner->GetActorLocation() : FVector::ZeroVector;
		UE_LOG(
			LogSyncAbilityMotionMoveDiag,
			Warning,
			TEXT("ClientAdjustPosition Time=%.3f ClientLoc=%s ServerLoc=%s Dist=%.2f ServerVel=%s ServerMode=%d Base=%s HasBase=%s BaseRelative=%s OptionalRot=%s %s"),
			TimeStamp,
			*ClientLocation.ToCompactString(),
			*NewLoc.ToCompactString(),
			FVector::Dist(ClientLocation, NewLoc),
			*NewVel.ToCompactString(),
			static_cast<int32>(ServerMovementMode),
			*GetNameSafe(NewBase),
			BoolText(bHasBase),
			BoolText(bBaseRelativePosition),
			OptionalRotation.IsSet() ? *OptionalRotation.GetValue().ToCompactString() : TEXT("None"),
			*DescribeMovementState(this));
	}

	Super::ClientAdjustPosition_Implementation(
		TimeStamp,
		NewLoc,
		NewVel,
		NewBase,
		NewBaseBoneName,
		bHasBase,
		bBaseRelativePosition,
		ServerMovementMode,
		OptionalRotation);
}

void USyncAbilityMotionCharacterMovementComponent::ClientVeryShortAdjustPosition_Implementation(
	float TimeStamp,
	FVector NewLoc,
	UPrimitiveComponent* NewBase,
	FName NewBaseBoneName,
	bool bHasBase,
	bool bBaseRelativePosition,
	uint8 ServerMovementMode)
{
	if (IsSyncAbilityMotionMovementDiagnosticsEnabled())
	{
		const FVector ClientLocation = CharacterOwner ? CharacterOwner->GetActorLocation() : FVector::ZeroVector;
		UE_LOG(
			LogSyncAbilityMotionMoveDiag,
			Warning,
			TEXT("ClientVeryShortAdjustPosition Time=%.3f ClientLoc=%s ServerLoc=%s Dist=%.2f ServerMode=%d Base=%s HasBase=%s BaseRelative=%s %s"),
			TimeStamp,
			*ClientLocation.ToCompactString(),
			*NewLoc.ToCompactString(),
			FVector::Dist(ClientLocation, NewLoc),
			static_cast<int32>(ServerMovementMode),
			*GetNameSafe(NewBase),
			BoolText(bHasBase),
			BoolText(bBaseRelativePosition),
			*DescribeMovementState(this));
	}

	Super::ClientVeryShortAdjustPosition_Implementation(
		TimeStamp,
		NewLoc,
		NewBase,
		NewBaseBoneName,
		bHasBase,
		bBaseRelativePosition,
		ServerMovementMode);
}

void USyncAbilityMotionCharacterMovementComponent::ClientAdjustRootMotionPosition_Implementation(
	float TimeStamp,
	float ServerMontageTrackPosition,
	FVector ServerLoc,
	FVector_NetQuantizeNormal ServerRotation,
	float ServerVelZ,
	UPrimitiveComponent* ServerBase,
	FName ServerBoneName,
	bool bHasBase,
	bool bBaseRelativePosition,
	uint8 ServerMovementMode)
{
	if (IsSyncAbilityMotionMovementDiagnosticsEnabled())
	{
		const FVector ClientLocation = CharacterOwner ? CharacterOwner->GetActorLocation() : FVector::ZeroVector;
		UE_LOG(
			LogSyncAbilityMotionMoveDiag,
			Warning,
			TEXT("ClientAdjustRootMotionPosition Time=%.3f ClientLoc=%s ServerLoc=%s Dist=%.2f ServerMontagePos=%.3f ServerRot=%s ServerVelZ=%.2f ServerMode=%d Base=%s HasBase=%s BaseRelative=%s %s"),
			TimeStamp,
			*ClientLocation.ToCompactString(),
			*ServerLoc.ToCompactString(),
			FVector::Dist(ClientLocation, ServerLoc),
			ServerMontageTrackPosition,
			*ServerRotation.ToCompactString(),
			ServerVelZ,
			static_cast<int32>(ServerMovementMode),
			*GetNameSafe(ServerBase),
			BoolText(bHasBase),
			BoolText(bBaseRelativePosition),
			*DescribeMovementState(this));
	}

	Super::ClientAdjustRootMotionPosition_Implementation(
		TimeStamp,
		ServerMontageTrackPosition,
		ServerLoc,
		ServerRotation,
		ServerVelZ,
		ServerBase,
		ServerBoneName,
		bHasBase,
		bBaseRelativePosition,
		ServerMovementMode);
}

void USyncAbilityMotionCharacterMovementComponent::ClientAdjustRootMotionSourcePosition_Implementation(
	float TimeStamp,
	FRootMotionSourceGroup ServerRootMotion,
	bool bHasAnimRootMotion,
	float ServerMontageTrackPosition,
	FVector ServerLoc,
	FVector_NetQuantizeNormal ServerRotation,
	float ServerVelZ,
	UPrimitiveComponent* ServerBase,
	FName ServerBoneName,
	bool bHasBase,
	bool bBaseRelativePosition,
	uint8 ServerMovementMode)
{
	if (IsSyncAbilityMotionMovementDiagnosticsEnabled())
	{
		const FVector ClientLocation = CharacterOwner ? CharacterOwner->GetActorLocation() : FVector::ZeroVector;
		UE_LOG(
			LogSyncAbilityMotionMoveDiag,
			Warning,
			TEXT("ClientAdjustRootMotionSourcePosition Time=%.3f ClientLoc=%s ServerLoc=%s Dist=%.2f HasAnimRootMotion=%s ServerMontagePos=%.3f ServerRot=%s ServerVelZ=%.2f ServerMode=%d Base=%s HasBase=%s BaseRelative=%s %s"),
			TimeStamp,
			*ClientLocation.ToCompactString(),
			*ServerLoc.ToCompactString(),
			FVector::Dist(ClientLocation, ServerLoc),
			BoolText(bHasAnimRootMotion),
			ServerMontageTrackPosition,
			*ServerRotation.ToCompactString(),
			ServerVelZ,
			static_cast<int32>(ServerMovementMode),
			*GetNameSafe(ServerBase),
			BoolText(bHasBase),
			BoolText(bBaseRelativePosition),
			*DescribeMovementState(this));
	}

	Super::ClientAdjustRootMotionSourcePosition_Implementation(
		TimeStamp,
		ServerRootMotion,
		bHasAnimRootMotion,
		ServerMontageTrackPosition,
		ServerLoc,
		ServerRotation,
		ServerVelZ,
		ServerBase,
		ServerBoneName,
		bHasBase,
		bBaseRelativePosition,
		ServerMovementMode);
}

float USyncAbilityMotionCharacterMovementComponent::GetMaxSpeed() const
{
	float MaxSpeed = Super::GetMaxSpeed();
	if (MaxSpeed <= 0.f) return MaxSpeed;

	const APawn* PawnOwnerPtr = PawnOwner;
	if (!PawnOwnerPtr) return MaxSpeed;

	const FVector CurrentAcceleration = Acceleration.GetSafeNormal2D();
	if (CurrentAcceleration.IsNearlyZero()) return MaxSpeed;

	const FVector Forward = PawnOwnerPtr->GetActorForwardVector().GetSafeNormal2D();
	if (Forward.IsNearlyZero()) return MaxSpeed;

	const float ForwardDot = FVector::DotProduct(Forward, CurrentAcceleration);
	if (ForwardDot <= BackwardDotThreshold)
	{
		return MaxSpeed * BackwardSpeedMultiplier;
	}

	return MaxSpeed;
}

FVector USyncAbilityMotionCharacterMovementComponent::ScaleInputAcceleration(const FVector& InputAcceleration) const
{
	if (bAbilityMovementInputSuppressed)
	{
		return FVector::ZeroVector;
	}

	return Super::ScaleInputAcceleration(InputAcceleration);
}

void USyncAbilityMotionCharacterMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice,
	const FVector& MoveDelta)
{
	if (Cast<ACharacter>(Hit.GetActor())
		&& !bAbilityRootMotionSuppressed
		&& bAbilityMovementInputSuppressed
		&& ShouldPauseRootMotionOnCharacterImpact(this))
	{
		if (IsSyncAbilityMotionMovementDiagnosticsEnabled())
		{
			UE_LOG(
				LogSyncAbilityMotionMoveDiag,
				Warning,
				TEXT("HandleImpact pausing ability root motion HitActor=%s HitComp=%s ImpactPoint=%s Normal=%s MoveDelta=%s TimeSlice=%.3f %s"),
				*GetNameSafe(Hit.GetActor()),
				*GetNameSafe(Hit.GetComponent()),
				*Hit.ImpactPoint.ToCompactString(),
				*Hit.ImpactNormal.ToCompactString(),
				*MoveDelta.ToCompactString(),
				TimeSlice,
				*DescribeMovementState(this));
		}

		SetAbilityRootMotionPausedByCharacterImpact(true);
		SetAbilityRootMotionSuppressed(true);
	}

	Super::HandleImpact(Hit, TimeSlice, MoveDelta);
}
