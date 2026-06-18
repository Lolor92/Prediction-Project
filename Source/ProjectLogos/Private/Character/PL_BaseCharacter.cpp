#include "Character/PL_BaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/PL_PlayerState.h"
#include "Tag/SyncCombatTags.h"

APL_BaseCharacter::APL_BaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	BodyCollisionPassthroughTag = TAG_State_Collision_IgnoreBody;
	BodyCollisionPassthroughChannels.Add(ECC_Pawn);
	
	// Collision defaults.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetGenerateOverlapEvents(true);
	
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	// Base movement tuning.
	GetCharacterMovement()->MaxWalkSpeed = 400.0f;
	GetCharacterMovement()->MaxCustomMovementSpeed = 400.0f;

	GetCharacterMovement()->MaxJumpApexAttemptsPerSimulation = 1;
	GetCharacterMovement()->JumpZVelocity = 850.f;
	GetCharacterMovement()->AirControl = 0.5f;
	GetCharacterMovement()->GravityScale = 2.5f;
}

UAbilitySystemComponent* APL_BaseCharacter::GetAbilitySystemComponent() const
{
	if (AbilitySystemComponent) return AbilitySystemComponent;

	// Fallback for actors that still own an ASC component directly.
	if (UAbilitySystemComponent* CharacterAbilitySystem = FindComponentByClass<UAbilitySystemComponent>())
	{
		return CharacterAbilitySystem;
	}

	APL_PlayerState* PL_PlayerState = GetPlayerState<APL_PlayerState>();
	return PL_PlayerState ? PL_PlayerState->GetAbilitySystemComponent() : nullptr;
}

UAttributeSet* APL_BaseCharacter::GetAttributeSet() const
{
	if (AttributeSet) return AttributeSet;

	const APL_PlayerState* PL_PlayerState = GetPlayerState<APL_PlayerState>();
	return PL_PlayerState ? PL_PlayerState->GetAttributeSet() : nullptr;
}

void APL_BaseCharacter::BindBodyCollisionPassthroughTag()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !BodyCollisionPassthroughTag.IsValid())
	{
		return;
	}

	if (BoundBodyCollisionASC.Get() == ASC)
	{
		OnBodyCollisionPassthroughTagChanged(
			BodyCollisionPassthroughTag,
			ASC->GetTagCount(BodyCollisionPassthroughTag)
		);
		return;
	}

	if (BoundBodyCollisionASC.IsValid() && BodyCollisionPassthroughDelegateHandle.IsValid())
	{
		BoundBodyCollisionASC->RegisterGameplayTagEvent(
			BodyCollisionPassthroughTag,
			EGameplayTagEventType::NewOrRemoved
		).Remove(BodyCollisionPassthroughDelegateHandle);

		BodyCollisionPassthroughDelegateHandle.Reset();
	}

	BoundBodyCollisionASC = ASC;

	BodyCollisionPassthroughDelegateHandle =
		ASC->RegisterGameplayTagEvent(
			BodyCollisionPassthroughTag,
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &APL_BaseCharacter::OnBodyCollisionPassthroughTagChanged);

	OnBodyCollisionPassthroughTagChanged(
		BodyCollisionPassthroughTag,
		ASC->GetTagCount(BodyCollisionPassthroughTag)
	);
}

void APL_BaseCharacter::OnBodyCollisionPassthroughTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	SetBodyCollisionPassthroughEnabled(NewCount > 0);
}

void APL_BaseCharacter::SetBodyCollisionPassthroughEnabled(bool bEnabled)
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule || BodyCollisionPassthroughChannels.IsEmpty())
	{
		return;
	}

	if (bEnabled == bBodyCollisionPassthroughActive)
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();

	if (bEnabled)
	{
		SavedCapsuleResponses.Reset();
		SavedMeshResponses.Reset();

		for (TEnumAsByte<ECollisionChannel> Channel : BodyCollisionPassthroughChannels)
		{
			const ECollisionChannel CollisionChannel = Channel.GetValue();

			SavedCapsuleResponses.Add(Capsule->GetCollisionResponseToChannel(CollisionChannel));
			Capsule->SetCollisionResponseToChannel(CollisionChannel, ECR_Ignore);

			if (MeshComponent)
			{
				SavedMeshResponses.Add(MeshComponent->GetCollisionResponseToChannel(CollisionChannel));
				MeshComponent->SetCollisionResponseToChannel(CollisionChannel, ECR_Ignore);
			}
		}

		Capsule->UpdateOverlaps();

		if (MeshComponent)
		{
			MeshComponent->UpdateOverlaps();
		}

		bBodyCollisionPassthroughActive = true;
		return;
	}

	for (int32 Index = 0; Index < BodyCollisionPassthroughChannels.Num(); ++Index)
	{
		const ECollisionChannel CollisionChannel = BodyCollisionPassthroughChannels[Index].GetValue();

		if (SavedCapsuleResponses.IsValidIndex(Index))
		{
			Capsule->SetCollisionResponseToChannel(
				CollisionChannel,
				SavedCapsuleResponses[Index].GetValue()
			);
		}

		if (MeshComponent && SavedMeshResponses.IsValidIndex(Index))
		{
			MeshComponent->SetCollisionResponseToChannel(
				CollisionChannel,
				SavedMeshResponses[Index].GetValue()
			);
		}
	}

	Capsule->UpdateOverlaps();

	if (MeshComponent)
	{
		MeshComponent->UpdateOverlaps();
	}

	SavedCapsuleResponses.Reset();
	SavedMeshResponses.Reset();

	bBodyCollisionPassthroughActive = false;
}

void APL_BaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetBodyCollisionPassthroughEnabled(false);

	if (BoundBodyCollisionASC.IsValid() && BodyCollisionPassthroughDelegateHandle.IsValid())
	{
		BoundBodyCollisionASC->RegisterGameplayTagEvent(
			BodyCollisionPassthroughTag,
			EGameplayTagEventType::NewOrRemoved
		).Remove(BodyCollisionPassthroughDelegateHandle);

		BodyCollisionPassthroughDelegateHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}
