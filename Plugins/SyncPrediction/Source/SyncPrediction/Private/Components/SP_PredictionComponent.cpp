#include "Components/SP_PredictionComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"

USP_PredictionComponent::USP_PredictionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool USP_PredictionComponent::PlayPredictedReactionOnTargetProxy(AActor* TargetActor, const FSP_ReactionDataEntry& Reaction)
{
	if (!CanPlayPredictedReactionOnTargetProxy(TargetActor, Reaction))
	{
		return false;
	}

	ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
	if (!TargetCharacter)
	{
		return false;
	}

	USkeletalMeshComponent* Mesh = TargetCharacter->GetMesh();
	if (!Mesh)
	{
		return false;
	}

	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return false;
	}

	const float PlayedLength = AnimInstance->Montage_Play(Reaction.Montage, Reaction.PlayRate);
	if (PlayedLength <= 0.f)
	{
		return false;
	}

	if (Reaction.StartSection != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(Reaction.StartSection, Reaction.Montage);
	}

	if (UWorld* World = GetWorld())
	{
		LastReactionTimeByTarget.FindOrAdd(TargetActor) = World->GetTimeSeconds();
	}

	return true;
}

bool USP_PredictionComponent::CanPlayPredictedReactionOnTargetProxy(AActor* TargetActor,
	const FSP_ReactionDataEntry& Reaction) const
{
	if (!TargetActor || !Reaction.Montage)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	// This component is on the attacker.
	// Only the attacking client should predict target proxy reaction.
	if (OwnerActor->HasAuthority())
	{
		return false;
	}

	const APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return false;
	}

	// The target we are animating should be a proxy on this machine.
	if (TargetActor->HasAuthority())
	{
		return false;
	}

	const APawn* TargetPawn = Cast<APawn>(TargetActor);
	if (TargetPawn && TargetPawn->IsLocallyControlled())
	{
		return false;
	}

	const double Now = World->GetTimeSeconds();

	if (const double* LastTime = LastReactionTimeByTarget.Find(TargetActor))
	{
		if (Now - *LastTime < Reaction.MinReplayInterval)
		{
			return false;
		}
	}

	return true;
}
