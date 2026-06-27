#include "Character/PL_BaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Movement/SP_CharacterMovementComponent.h"
#include "Player/PL_PlayerState.h"

static int32 PL_GetWorldPIEInstanceId(const UWorld* World)
{
	if (!World)
	{
		return INDEX_NONE;
	}

	if (const UPackage* Package = World->GetPackage())
	{
		const int32 PIEInstance = Package->GetPIEInstanceID();
		if (PIEInstance != INDEX_NONE)
		{
			return PIEInstance;
		}
	}

	if (GEngine)
	{
		if (const FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(World))
		{
			return WorldContext->PIEInstance;
		}
	}

	return INDEX_NONE;
}

APL_BaseCharacter::APL_BaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USP_CharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
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

void APL_BaseCharacter::OnRep_ReplicatedMovement()
{
	const FVector BeforeLocation = GetActorLocation();

	Super::OnRep_ReplicatedMovement();

	const FVector AfterLocation = GetActorLocation();

	UE_LOG(LogTemp, Warning,
		TEXT("SP OnRep_ReplicatedMovement PIE=%d Actor=%s Before=%s After=%s Delta=%s Role=%d Local=%d Auth=%d"),
		PL_GetWorldPIEInstanceId(GetWorld()),
		*GetNameSafe(this),
		*BeforeLocation.ToString(),
		*AfterLocation.ToString(),
		*(AfterLocation - BeforeLocation).ToString(),
		static_cast<int32>(GetLocalRole()),
		IsLocallyControlled(),
		HasAuthority());
}

void APL_BaseCharacter::PostNetReceiveLocationAndRotation()
{
	const FVector BeforeLocation = GetActorLocation();

	Super::PostNetReceiveLocationAndRotation();

	const FVector AfterLocation = GetActorLocation();

	UE_LOG(LogTemp, Warning,
		TEXT("SP PostNetReceiveLocationAndRotation PIE=%d Actor=%s Before=%s After=%s Delta=%s Role=%d Local=%d Auth=%d"),
		PL_GetWorldPIEInstanceId(GetWorld()),
		*GetNameSafe(this),
		*BeforeLocation.ToString(),
		*AfterLocation.ToString(),
		*(AfterLocation - BeforeLocation).ToString(),
		static_cast<int32>(GetLocalRole()),
		IsLocallyControlled(),
		HasAuthority());
}

void APL_BaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}
