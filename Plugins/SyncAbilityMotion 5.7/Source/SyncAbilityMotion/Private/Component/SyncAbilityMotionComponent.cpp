#include "Component/SyncAbilityMotionComponent.h"

#include "AnimInstance/SyncAbilityMotionAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Movement/SyncAbilityMotionCharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

USyncAbilityMotionComponent::USyncAbilityMotionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USyncAbilityMotionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(USyncAbilityMotionComponent, AbilityMotionState, COND_SkipOwner);
}

void USyncAbilityMotionComponent::SetAbilityMotionState(const FSyncAbilityMotionState& NewState)
{
	if (AbilityMotionState == NewState)
	{
		return;
	}

	AbilityMotionState = NewState;
	ApplyAbilityMotionState(NewState);

	AActor* OwnerActor = GetOwner();
	if (OwnerActor && OwnerActor->HasAuthority())
	{
		// The release-to-lower-body state is timing-sensitive.
		// Do not wait for the actor's normal replication interval.
		OwnerActor->ForceNetUpdate();

		// Visual fast path for simulated proxies.
		// The replicated AbilityMotionState remains the reliable eventual backup.
		MulticastApplyAbilityMotionState(NewState);
	}
}

void USyncAbilityMotionComponent::ResetAbilityMotionState()
{
	if (ACharacter* Character = GetOwnerCharacter())
	{
		if (USyncAbilityMotionCharacterMovementComponent* MoveComp =
			Cast<USyncAbilityMotionCharacterMovementComponent>(Character->GetCharacterMovement()))
		{
			MoveComp->SetAbilityRootMotionPausedByCharacterImpact(false);
			MoveComp->SetAbilityRootMotionSuppressed(false);
			MoveComp->SetAbilityMovementInputSuppressed(false);
		}
	}

	FSyncAbilityMotionState DefaultState;
	DefaultState.bCanBlendMontage = false;
	DefaultState.bShouldBlendLowerBody = false;
	DefaultState.bRootMotionEnabled = true;
	DefaultState.bMovementInputSuppressed = false;

	SetAbilityMotionState(DefaultState);
}

void USyncAbilityMotionComponent::ServerSetAbilityMotionState_Implementation(const FSyncAbilityMotionState& NewState)
{
	SetAbilityMotionState(NewState);
}

void USyncAbilityMotionComponent::MulticastApplyAbilityMotionState_Implementation(
	const FSyncAbilityMotionState& NewState)
{
	ACharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		return;
	}

	// Server already applied it.
	if (Character->HasAuthority())
	{
		return;
	}

	// Owning client predicts this locally. Do not stomp it.
	if (Character->IsLocallyControlled())
	{
		return;
	}

	AbilityMotionState = NewState;
	ApplyAbilityMotionState(NewState);
}

void USyncAbilityMotionComponent::OnRep_AbilityMotionState()
{
	const ACharacter* Character = GetOwnerCharacter();
	if (Character && Character->IsLocallyControlled())
	{
		return;
	}

	ApplyAbilityMotionState(AbilityMotionState);
}

void USyncAbilityMotionComponent::ApplyAbilityMotionState(const FSyncAbilityMotionState& NewState)
{
	ACharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = Character->GetMesh();
	if (!MeshComp)
	{
		return;
	}

	USyncAbilityMotionAnimInstance* AnimInstance = Cast<USyncAbilityMotionAnimInstance>(MeshComp->GetAnimInstance());
	if (!AnimInstance)
	{
		return;
	}

	AnimInstance->bCanBlendMontage = NewState.bCanBlendMontage;
	AnimInstance->bShouldBlendLowerBody = NewState.bShouldBlendLowerBody;
	AnimInstance->bRootMotionEnabled = NewState.bRootMotionEnabled;

	AnimInstance->SetRootMotionMode(
		NewState.bRootMotionEnabled
			? ERootMotionMode::RootMotionFromMontagesOnly
			: ERootMotionMode::IgnoreRootMotion);

	USyncAbilityMotionCharacterMovementComponent* MoveComp =
		Cast<USyncAbilityMotionCharacterMovementComponent>(Character->GetCharacterMovement());

	const bool bRootMotionSuppressed = !NewState.bRootMotionEnabled;
	const bool bPausedByCharacterImpact = bRootMotionSuppressed && NewState.bMovementInputSuppressed;

	if (MoveComp)
	{
		// If the replicated/multicast ability state says movement is released,
		// also clear any leftover reaction movement lock on simulated proxies.
		// Escape clears this on owner/server during activation, but simulated proxies
		// do not run that activation path.
		if (!NewState.bMovementInputSuppressed)
		{
			MoveComp->EndReactionMovementInputLock();
		}

		MoveComp->SetAbilityRootMotionPausedByCharacterImpact(bPausedByCharacterImpact);
		MoveComp->SetAbilityRootMotionSuppressed(bRootMotionSuppressed);
		MoveComp->SetAbilityMovementInputSuppressed(NewState.bMovementInputSuppressed);
		MoveComp->RefreshAbilityRootMotionMode();
	}
}

ACharacter* USyncAbilityMotionComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}
