#include "AnimInstance/SyncAbilityMotionAnimInstance.h"
#include "Ability/SyncAbilityMotionGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimMontage.h"
#include "Component/SyncAbilityMotionComponent.h"
#include "Data/SyncAbilityMotionTypes.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Movement/SyncAbilityMotionCharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogSyncAbilityMotionMontage, Log, All);

void USyncAbilityMotionAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	APawn* PawnOwner = TryGetPawnOwner();
	if (!PawnOwner) return;

	Character = Cast<ACharacter>(PawnOwner);
	if (!Character) return;

	CharacterMovementComponent = Character->GetCharacterMovement();
	AbilitySystemComponent = GetAbilitySystemComponentSafe();
	MotionComponent = GetMotionComponentSafe();
}

void USyncAbilityMotionAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character || !CharacterMovementComponent) return;

	bIsAccelerating = CharacterMovementComponent->GetCurrentAcceleration().Size() > 0.f;
	GroundSpeed = UKismetMathLibrary::VSizeXY(CharacterMovementComponent->Velocity);
	IsAirBorne = CharacterMovementComponent->IsFalling();

	AimRotation = Character->GetBaseAimRotation();

	if (!Character->GetVelocity().IsNearlyZero())
	{
		MovementRotation = UKismetMathLibrary::MakeRotFromX(Character->GetVelocity());
		MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
	}
	else
	{
		MovementOffsetYaw = 0.f;
	}

	UpdateAbilityMotionReplication();

	float Percent = 0.f;
	USyncAbilityMotionGameplayAbility* Ability = nullptr;
	const bool bHasAbilityContext = GetAbilityPercentMontagePlayed(Percent, Ability);

	const bool bAbilityOwnsRotation = bHasAbilityContext && bRootMotionEnabled && !bShouldBlendLowerBody;
	const bool bAllowControllerYaw = !bAbilityOwnsRotation && bIsAccelerating;

	if (bDriveControllerYawFromAbilityState)
	{
		CacheControllerYawSettings();

		Character->bUseControllerRotationYaw = false;
		CharacterMovementComponent->bUseControllerDesiredRotation = bAllowControllerYaw;
		CharacterMovementComponent->bOrientRotationToMovement = bAllowControllerYaw ? false : bCachedOrientRotationToMovement;
		CharacterMovementComponent->RotationRate = CachedRotationRate;

		if (bAllowControllerYaw && bOverrideControllerYawRotationRate && ControllerYawRotationRate > 0.f)
		{
			FRotator DesiredRotationRate = CharacterMovementComponent->RotationRate;
			DesiredRotationRate.Yaw = ControllerYawRotationRate;
			CharacterMovementComponent->RotationRate = DesiredRotationRate;
		}

		bUseControllerRotationYaw = bAllowControllerYaw;
	}
	else
	{
		RestoreControllerYawSettings();
	}
}

void USyncAbilityMotionAnimInstance::CacheControllerYawSettings()
{
	if (bHasCachedControllerYawSettings || !Character || !CharacterMovementComponent) return;

	bCachedUseControllerRotationYaw = Character->bUseControllerRotationYaw;
	bCachedUseControllerDesiredRotation = CharacterMovementComponent->bUseControllerDesiredRotation;
	bCachedOrientRotationToMovement = CharacterMovementComponent->bOrientRotationToMovement;
	CachedRotationRate = CharacterMovementComponent->RotationRate;
	bHasCachedControllerYawSettings = true;
}

void USyncAbilityMotionAnimInstance::RestoreControllerYawSettings()
{
	if (!bHasCachedControllerYawSettings || !Character || !CharacterMovementComponent) return;

	Character->bUseControllerRotationYaw = bCachedUseControllerRotationYaw;
	CharacterMovementComponent->bUseControllerDesiredRotation = bCachedUseControllerDesiredRotation;
	CharacterMovementComponent->bOrientRotationToMovement = bCachedOrientRotationToMovement;
	CharacterMovementComponent->RotationRate = CachedRotationRate;
	bUseControllerRotationYaw = bCachedUseControllerRotationYaw;
	bHasCachedControllerYawSettings = false;
}

void USyncAbilityMotionAnimInstance::UpdateAbilityMotionReplication()
{
	if (!Character || !Character->IsLocallyControlled()) return;

	USyncAbilityMotionComponent* SyncMotion = GetMotionComponentSafe();
	if (!SyncMotion) return;

	float Percent = 0.f;
	USyncAbilityMotionGameplayAbility* Ability = nullptr;
	const bool bHasAbilityContext = GetAbilityPercentMontagePlayed(Percent, Ability);

	if (!bHasAbilityContext || !Ability)
	{
		if (LastTrackedAbility || LastTrackedMontage)
		{
			UE_LOG(LogSyncAbilityMotionMontage, Display,
				TEXT("MontageTrackClear Character=%s LastAbility=%s LastSeq=%u LastMontage=%s"),
				*GetNameSafe(Character),
				*GetNameSafe(LastTrackedAbility),
				LastTrackedAbilityActivationSequenceId,
				*GetNameSafe(LastTrackedMontage));
		}

		if (USyncAbilityMotionCharacterMovementComponent* SyncMoveComp =
			Cast<USyncAbilityMotionCharacterMovementComponent>(CharacterMovementComponent))
		{
			SyncMoveComp->SetAbilityRootMotionPausedByCharacterImpact(false);
		}

		LastTrackedAbility = nullptr;
		LastTrackedAbilityActivationSequenceId = 0;
		LastTrackedMontage = nullptr;
		bReleasedRootMotionThisMontage = false;
		SyncMotion->ResetAbilityMotionState();
		return;
	}

	const UAnimMontage* CurrentMontage = Ability->GetCurrentMontage();
	const uint32 CurrentActivationSequenceId = Ability->GetActivationSequenceId();

	if (Ability != LastTrackedAbility
		|| CurrentActivationSequenceId != LastTrackedAbilityActivationSequenceId
		|| CurrentMontage != LastTrackedMontage)
	{
		LastTrackedAbility = Ability;
		LastTrackedAbilityActivationSequenceId = CurrentActivationSequenceId;
		LastTrackedMontage = CurrentMontage;
		bReleasedRootMotionThisMontage = false;

		UE_LOG(LogSyncAbilityMotionMontage, Display,
			TEXT("MontageTrackChange Character=%s Ability=%s Seq=%u Montage=%s Position=%.3f Percent=%.2f Local=%d Authority=%d"),
			*GetNameSafe(Character),
			*GetNameSafe(Ability),
			CurrentActivationSequenceId,
			*GetNameSafe(CurrentMontage),
			CurrentMontage ? Montage_GetPosition(CurrentMontage) : -1.f,
			Percent,
			Character && Character->IsLocallyControlled(),
			Character && Character->HasAuthority());

		if (USyncAbilityMotionCharacterMovementComponent* SyncMoveComp =
			Cast<USyncAbilityMotionCharacterMovementComponent>(CharacterMovementComponent))
		{
			SyncMoveComp->SetAbilityRootMotionPausedByCharacterImpact(false);
		}
	}

	const bool bReachedReleasePoint =
		!Ability->MontageLockout.bUseMontageProgressLockout ||
		Percent >= Ability->MontageLockout.MontageProgressBeforeInterrupt;

	const bool bReleaseWasAlreadyReached = bReachedReleasePoint || bReleasedRootMotionThisMontage;

	const bool bHasMovementInput =
		CharacterMovementComponent &&
		!CharacterMovementComponent->GetCurrentAcceleration().IsNearlyZero(0.01f);

	if (bReleaseWasAlreadyReached && bHasMovementInput)
	{
		bReleasedRootMotionThisMontage = true;
	}

	USyncAbilityMotionCharacterMovementComponent* SyncMoveComp =
		Cast<USyncAbilityMotionCharacterMovementComponent>(CharacterMovementComponent);
	if (bReleaseWasAlreadyReached && SyncMoveComp)
	{
		SyncMoveComp->SetAbilityRootMotionPausedByCharacterImpact(false);
	}

	const bool bPausedByCharacterCollision =
		!bReleasedRootMotionThisMontage &&
		(Ability->ShouldPauseRootMotionForCharacterCollision(Character) ||
			(SyncMoveComp && SyncMoveComp->IsAbilityRootMotionPausedByCharacterImpact()));

	FSyncAbilityMotionState DesiredState;
	DesiredState.bCanBlendMontage = bReleaseWasAlreadyReached;
	DesiredState.bShouldBlendLowerBody = bReleaseWasAlreadyReached && bHasMovementInput;
	DesiredState.bRootMotionEnabled = !bReleasedRootMotionThisMontage && !bPausedByCharacterCollision;
	DesiredState.bMovementInputSuppressed = !bReleaseWasAlreadyReached;

	if (SyncMoveComp)
	{
		SyncMoveComp->SetAbilityRootMotionSuppressed(!DesiredState.bRootMotionEnabled);
		SyncMoveComp->SetAbilityMovementInputSuppressed(DesiredState.bMovementInputSuppressed);
	}

	if (SyncMotion->GetAbilityMotionState() == DesiredState) return;

	SyncMotion->SetAbilityMotionState(DesiredState);
}

bool USyncAbilityMotionAnimInstance::GetAbilityPercentMontagePlayed(
	float& OutPercent,
	USyncAbilityMotionGameplayAbility*& OutAbility)
{
	OutPercent = 0.f;
	OutAbility = nullptr;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentSafe();
	if (!ASC) return false;

	UGameplayAbility* ActiveAbility = ASC->GetAnimatingAbility();
	if (!ActiveAbility) return false;

	UAnimMontage* CurrentMontage = ActiveAbility->GetCurrentMontage();
	if (!CurrentMontage) return false;

	USyncAbilityMotionGameplayAbility* Ability = Cast<USyncAbilityMotionGameplayAbility>(ActiveAbility);
	if (!Ability) return false;

	const float MontageLength = CurrentMontage->GetPlayLength();
	if (MontageLength <= 0.f) return false;

	const float CurrentPosition = Montage_GetPosition(CurrentMontage);
	OutPercent = (CurrentPosition / MontageLength) * 100.f;
	OutAbility = Ability;

	return true;
}

UAbilitySystemComponent* USyncAbilityMotionAnimInstance::GetAbilitySystemComponentSafe()
{
	if (!AbilitySystemComponent && Character)
	{
		if (IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(Character))
		{
			AbilitySystemComponent = AbilityOwner->GetAbilitySystemComponent();
		}

		if (!AbilitySystemComponent)
		{
			AbilitySystemComponent = Character->FindComponentByClass<UAbilitySystemComponent>();
		}
	}

	return AbilitySystemComponent;
}

USyncAbilityMotionComponent* USyncAbilityMotionAnimInstance::GetMotionComponentSafe()
{
	if (!MotionComponent && Character)
	{
		MotionComponent = Character->FindComponentByClass<USyncAbilityMotionComponent>();
	}

	return MotionComponent;
}
