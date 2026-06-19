#include "Abilities/SP_GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

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
	bMovementInputBlockedByContact = false;
	bContactEventBound = false;

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
	RestoreMovementInputFromContact();

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

	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	if (!Capsule)
	{
		return;
	}

	if (!bContactEventBound)
	{
		Capsule->OnComponentHit.AddDynamic(
			this,
			&USP_GameplayAbility::HandleCapsuleHit);

		bContactEventBound = true;

		UE_LOG(LogTemp, Warning,
			TEXT("SP Ability bound capsule contact event Character=%s"),
			*GetNameSafe(Character));
	}

	// Important: handles the case where the ability starts while already touching.
	CheckInitialRootMotionContact();
}

void USP_GameplayAbility::StopRootMotionContactCheck()
{
	ACharacter* Character = CachedCharacter.Get();
	if (!Character)
	{
		bContactEventBound = false;
		return;
	}

	if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
	{
		if (bContactEventBound)
		{
			Capsule->OnComponentHit.RemoveDynamic(
				this,
				&USP_GameplayAbility::HandleCapsuleHit);
		}
	}

	bContactEventBound = false;
}

void USP_GameplayAbility::CheckInitialRootMotionContact()
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

void USP_GameplayAbility::HandleCapsuleHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (bRootMotionStoppedByContact)
	{
		return;
	}

	ACharacter* Character = CachedCharacter.Get();
	if (!Character || !OtherActor || OtherActor == Character)
	{
		return;
	}

	float ContactAngle = 0.f;
	if (!IsValidRootMotionStopContact(Character, OtherActor, ContactAngle))
	{
		return;
	}

	StopRootMotionFromContact(OtherActor, ContactAngle);
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

		if (IsValidRootMotionStopContact(Character, OtherActor, OutContactAngle))
		{
			OutBlockingActor = OtherActor;
			return true;
		}
	}

	return false;
}

bool USP_GameplayAbility::IsValidRootMotionStopContact(
	ACharacter* Character,
	AActor* OtherActor,
	float& OutContactAngle) const
{
	OutContactAngle = 0.f;

	if (!Character || !OtherActor || OtherActor == Character)
	{
		return false;
	}

	const APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (!OtherPawn)
	{
		return false;
	}

	FVector Forward2D = Character->GetActorForwardVector();
	Forward2D.Z = 0.f;
	Forward2D.Normalize();

	if (Forward2D.IsNearlyZero())
	{
		return false;
	}

	FVector ToOther = OtherActor->GetActorLocation() - Character->GetActorLocation();
	ToOther.Z = 0.f;

	if (ToOther.IsNearlyZero())
	{
		OutContactAngle = 0.f;
		return true;
	}

	ToOther.Normalize();

	const float Dot = FMath::Clamp(FVector::DotProduct(Forward2D, ToOther), -1.f, 1.f);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));

	OutContactAngle = AngleDegrees;

	return AngleDegrees <= RootMotionStopContactAngleDegrees;
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

	if (bBlockMovementInputWhenRootMotionStops)
	{
		BlockMovementInputFromContact();
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

	if (Character->HasAuthority())
	{
		MulticastStopRootMotionFromContact(Character);
	}
}

void USP_GameplayAbility::MulticastStopRootMotionFromContact_Implementation(AActor* AvatarActor)
{
	ACharacter* Character = Cast<ACharacter>(AvatarActor);
	if (!Character)
	{
		return;
	}

	// Server already did it in StopRootMotionFromContact.
	if (Character->HasAuthority())
	{
		return;
	}

	// Owning client already stopped immediately through local prediction.
	if (Character->IsLocallyControlled())
	{
		return;
	}

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;

	if (AnimInstance)
	{
		AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}

	if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->Velocity = FVector::ZeroVector;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("SP Ability multicast stopped simulated root motion Character=%s NetMode=%d Role=%d Local=%d Auth=%d"),
		*GetNameSafe(Character),
		Character->GetWorld() ? static_cast<int32>(Character->GetWorld()->GetNetMode()) : -1,
		static_cast<int32>(Character->GetLocalRole()),
		Character->IsLocallyControlled(),
		Character->HasAuthority());
}

void USP_GameplayAbility::BlockMovementInputFromContact()
{
	if (bMovementInputBlockedByContact)
	{
		return;
	}

	ACharacter* Character = CachedCharacter.Get();
	if (!Character)
	{
		return;
	}

	AController* Controller = Character->GetController();
	if (!Controller)
	{
		return;
	}

	Controller->SetIgnoreMoveInput(true);
	bMovementInputBlockedByContact = true;

	if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->Velocity = FVector::ZeroVector;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("SP Ability blocked movement input from root motion contact Character=%s NetMode=%d Role=%d Local=%d Auth=%d"),
		*GetNameSafe(Character),
		Character->GetWorld() ? static_cast<int32>(Character->GetWorld()->GetNetMode()) : -1,
		static_cast<int32>(Character->GetLocalRole()),
		Character->IsLocallyControlled(),
		Character->HasAuthority());
}

void USP_GameplayAbility::RestoreMovementInputFromContact()
{
	if (!bMovementInputBlockedByContact)
	{
		return;
	}

	if (!bRestoreMovementInputWhenAbilityEnds)
	{
		return;
	}

	ACharacter* Character = CachedCharacter.Get();
	if (!Character)
	{
		bMovementInputBlockedByContact = false;
		return;
	}

	if (AController* Controller = Character->GetController())
	{
		Controller->SetIgnoreMoveInput(false);
	}

	bMovementInputBlockedByContact = false;

	UE_LOG(LogTemp, Warning,
		TEXT("SP Ability restored movement input after root motion contact Character=%s NetMode=%d Role=%d Local=%d Auth=%d"),
		*GetNameSafe(Character),
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
