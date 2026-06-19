#include "Abilities/SP_GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

USP_GameplayAbility::USP_GameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void USP_GameplayAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Character)
	{
		return;
	}

	CachedCharacter = Character;
	bRootMotionStoppedByContact = false;

	if (USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
		{
			PreviousRootMotionMode = AnimInstance->RootMotionMode.GetValue();
		}
	}

	StartRootMotionContactCheck();
}

void USP_GameplayAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	StopRootMotionContactCheck();
	RestoreRootMotionMode();

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void USP_GameplayAbility::StartRootMotionContactCheck()
{
	if (!bStopRootMotionOnPawnContact)
	{
		return;
	}

	ACharacter* Character = CachedCharacter.Get();
	if (!Character)
	{
		return;
	}

	UWorld* World = Character->GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		RootMotionContactTimerHandle,
		this,
		&USP_GameplayAbility::CheckRootMotionContact,
		RootMotionContactCheckInterval,
		true);

	// Also check immediately, so starting already touching a proxy capsule stops root motion right away.
	CheckRootMotionContact();
}

void USP_GameplayAbility::StopRootMotionContactCheck()
{
	if (ACharacter* Character = CachedCharacter.Get())
	{
		if (UWorld* World = Character->GetWorld())
		{
			World->GetTimerManager().ClearTimer(RootMotionContactTimerHandle);
		}
	}
}

void USP_GameplayAbility::CheckRootMotionContact()
{
	if (bRootMotionStoppedByContact)
	{
		return;
	}

	ACharacter* Character = CachedCharacter.Get();
	if (!Character)
	{
		return;
	}

	AActor* BlockingActor = nullptr;
	float ContactAngle = 0.f;

	if (ShouldStopRootMotionForContact(Character, BlockingActor, ContactAngle))
	{
		StopRootMotionFromContact(BlockingActor, ContactAngle);
	}
}

bool USP_GameplayAbility::ShouldStopRootMotionForContact(
	ACharacter* Character,
	AActor*& OutBlockingActor,
	float& OutContactAngle) const
{
	OutBlockingActor = nullptr;
	OutContactAngle = 0.f;

	if (!Character)
	{
		return false;
	}

	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	if (!Capsule)
	{
		return false;
	}

	UWorld* World = Character->GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector CharacterLocation = Character->GetActorLocation();

	const FVector Forward2D =
		FVector(Character->GetActorForwardVector().X, Character->GetActorForwardVector().Y, 0.f)
		.GetSafeNormal();

	if (Forward2D.IsNearlyZero())
	{
		return false;
	}

	const float Radius = Capsule->GetScaledCapsuleRadius() + RootMotionContactCapsuleInflation;
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SP_RootMotionContact), false);
	QueryParams.AddIgnoredActor(Character);

	TArray<FOverlapResult> Overlaps;

	const bool bHasOverlap = World->OverlapMultiByObjectType(
		Overlaps,
		CharacterLocation,
		FQuat::Identity,
		ObjectParams,
		Shape,
		QueryParams);

	if (!bHasOverlap)
	{
		return false;
	}

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OtherActor = Overlap.GetActor();
		if (!OtherActor || OtherActor == Character)
		{
			continue;
		}

		const APawn* OtherPawn = Cast<APawn>(OtherActor);
		if (!OtherPawn)
		{
			continue;
		}

		FVector ToOther = OtherActor->GetActorLocation() - CharacterLocation;
		ToOther.Z = 0.f;

		if (ToOther.IsNearlyZero())
		{
			// Already deeply touching. Treat as valid contact.
			OutBlockingActor = OtherActor;
			OutContactAngle = 0.f;
			return true;
		}

		ToOther.Normalize();

		const float Dot = FMath::Clamp(FVector::DotProduct(Forward2D, ToOther), -1.f, 1.f);
		const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));

		if (AngleDegrees <= RootMotionStopContactAngleDegrees)
		{
			OutBlockingActor = OtherActor;
			OutContactAngle = AngleDegrees;
			return true;
		}
	}

	return false;
}

void USP_GameplayAbility::StopRootMotionFromContact(
	AActor* BlockingActor,
	float ContactAngle)
{
	ACharacter* Character = CachedCharacter.Get();
	if (!Character)
	{
		return;
	}

	if (USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
		{
			AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
		}
	}

	if (bClearVelocityWhenRootMotionStops)
	{
		if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}
	}

	bRootMotionStoppedByContact = true;

	UE_LOG(LogTemp, Warning,
		TEXT("SP Ability stopped root motion from contact Character=%s BlockingActor=%s Angle=%.2f NetMode=%d Role=%d Local=%d Auth=%d"),
		*GetNameSafe(Character),
		*GetNameSafe(BlockingActor),
		ContactAngle,
		Character->GetWorld() ? static_cast<int32>(Character->GetWorld()->GetNetMode()) : -1,
		static_cast<int32>(Character->GetLocalRole()),
		Character->IsLocallyControlled(),
		Character->HasAuthority());
}

void USP_GameplayAbility::RestoreRootMotionMode()
{
	ACharacter* Character = CachedCharacter.Get();
	if (!Character)
	{
		return;
	}

	if (USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
		{
			AnimInstance->SetRootMotionMode(PreviousRootMotionMode);
		}
	}
}
