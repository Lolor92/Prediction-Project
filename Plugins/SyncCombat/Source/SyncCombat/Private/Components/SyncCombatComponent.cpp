// Copyright ProjectLogos

#include "Components/SyncCombatComponent.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Runtime/SyncCombatHitWindowRuntime.h"
#include "Runtime/SyncCombatTagReactionRuntime.h"
#include "Utilities/SyncCombatFunctionLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "TimerManager.h"

USyncCombatComponent::USyncCombatComponent(FVTableHelper& Helper)
	: Super(Helper)
	, HitWindowRuntime(*this)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	SetComponentTickEnabled(false);

	TagReactionRuntime = new FSyncCombatTagReactionRuntime(*this);
}

USyncCombatComponent::USyncCombatComponent()
	: HitWindowRuntime(*this)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	SetComponentTickEnabled(false);

	TagReactionRuntime = new FSyncCombatTagReactionRuntime(*this);
}

USyncCombatComponent::~USyncCombatComponent()
{
	delete TagReactionRuntime;
	TagReactionRuntime = nullptr;
}

void USyncCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoInitializeCombat)
	{
		TryAutoInitializeCombat();
	}
}

void USyncCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearAutoInitializeRetry();
	DeinitializeCombat();

	Super::EndPlay(EndPlayReason);
}

void USyncCombatComponent::InitializeCombat(
	AActor* InOwnerActor,
	UAbilitySystemComponent* InAbilitySystemComponent)
{
	if (!InOwnerActor || !InAbilitySystemComponent) return;
	if (OwningActor == InOwnerActor && AbilitySystemComponent == InAbilitySystemComponent) return;

	if (AbilitySystemComponent && AbilitySystemComponent != InAbilitySystemComponent) DeinitializeCombat();

	OwningActor = InOwnerActor;
	AbilitySystemComponent = InAbilitySystemComponent;

	GrantDefaultAbilities();
	BindCrowdControlTagEvent();
	BindMovementModeChanged();
	if (TagReactionRuntime) TagReactionRuntime->Initialize(OwningActor, AbilitySystemComponent, TagReactionData, AnimBoolBindings);
	ClearAutoInitializeRetry();
}

void USyncCombatComponent::DeinitializeCombat()
{
	if (TagReactionRuntime) TagReactionRuntime->Deinitialize();
	HitWindowRuntime.Deinitialize();

	ClearCrowdControlTagEvent();
	ClearMovementModeChanged();
	RemoveGameplayEffect(AirborneEffectHandle);
	ClearDefaultAbilities();

	OwningActor = nullptr;
	AbilitySystemComponent = nullptr;
	bAirborneEffectApplied = false;
}

void USyncCombatComponent::HandleMovementModeChanged(EMovementMode NewMovementMode)
{
	if (!OwningActor || !AbilitySystemComponent) return;

	const bool bIsAirborne = NewMovementMode == MOVE_Falling;
	if (bIsAirborne)
	{
		if (!bAirborneEffectApplied && AirborneEffectClass)
		{
			AirborneEffectHandle = ApplyEffectToSelf(AirborneEffectClass, 1.f);
			bAirborneEffectApplied = AirborneEffectHandle.IsValid();
		}

		return;
	}

	if (bAirborneEffectApplied)
	{
		RemoveGameplayEffect(AirborneEffectHandle);
		bAirborneEffectApplied = false;
	}
}

void USyncCombatComponent::BindMovementModeChanged()
{
	if (bMovementModeChangedBound) return;

	ACharacter* OwningCharacter = Cast<ACharacter>(OwningActor);
	if (!OwningCharacter) return;

	OwningCharacter->MovementModeChangedDelegate.AddDynamic(this, &ThisClass::OnCharacterMovementModeChanged);
	bMovementModeChangedBound = true;

	if (const UCharacterMovementComponent* MoveComp = OwningCharacter->GetCharacterMovement())
	{
		HandleMovementModeChanged(MoveComp->MovementMode);
	}
}

void USyncCombatComponent::ClearMovementModeChanged()
{
	if (!bMovementModeChangedBound) return;

	if (ACharacter* OwningCharacter = Cast<ACharacter>(OwningActor))
	{
		OwningCharacter->MovementModeChangedDelegate.RemoveDynamic(
			this,
			&ThisClass::OnCharacterMovementModeChanged);
	}

	bMovementModeChangedBound = false;
}

void USyncCombatComponent::OnCharacterMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode,
	uint8 PreviousCustomMode)
{
	(void)PrevMovementMode;
	(void)PreviousCustomMode;

	if (!Character) return;

	if (const UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
	{
		HandleMovementModeChanged(MoveComp->MovementMode);
	}
}

void USyncCombatComponent::ClientApplyHitWindowTransformPrediction_Implementation(
	const bool bApplyRotation,
	const FRotator Rotation,
	const ESyncCombatHitWindowTeleportType RotationTeleportType,
	const bool bApplyLocation,
	const FVector Location,
	const bool bSweepLocation,
	const ESyncCombatHitWindowTeleportType LocationTeleportType)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || OwnerActor->HasAuthority())
	{
		return;
	}

	if (bApplyRotation)
	{
		OwnerActor->SetActorRotation(Rotation, ToTeleportType(RotationTeleportType));
	}

	if (bApplyLocation)
	{
		OwnerActor->SetActorLocation(Location, bSweepLocation, nullptr, ToTeleportType(LocationTeleportType));
	}
}

bool USyncCombatComponent::IsBlockingActive() const
{
	return AbilitySystemComponent
		&& BlockingTag.IsValid()
		&& AbilitySystemComponent->HasMatchingGameplayTag(BlockingTag);
}

bool USyncCombatComponent::IsParryingActive() const
{
	return AbilitySystemComponent
		&& ParryingTag.IsValid()
		&& AbilitySystemComponent->HasMatchingGameplayTag(ParryingTag);
}

bool USyncCombatComponent::IsCrowdControlActive() const
{
	return AbilitySystemComponent
		&& CrowdControlTag.IsValid()
		&& AbilitySystemComponent->HasMatchingGameplayTag(CrowdControlTag);
}

ESyncCombatHitWindowSuperArmorLevel USyncCombatComponent::GetCurrentSuperArmorLevel() const
{
	if (!AbilitySystemComponent)
	{
		return ESyncCombatHitWindowSuperArmorLevel::None;
	}

	if (SuperArmorTag3.IsValid() && AbilitySystemComponent->HasMatchingGameplayTag(SuperArmorTag3))
	{
		return ESyncCombatHitWindowSuperArmorLevel::SuperArmor3;
	}

	if (SuperArmorTag2.IsValid() && AbilitySystemComponent->HasMatchingGameplayTag(SuperArmorTag2))
	{
		return ESyncCombatHitWindowSuperArmorLevel::SuperArmor2;
	}

	if (SuperArmorTag1.IsValid() && AbilitySystemComponent->HasMatchingGameplayTag(SuperArmorTag1))
	{
		return ESyncCombatHitWindowSuperArmorLevel::SuperArmor1;
	}

	return ESyncCombatHitWindowSuperArmorLevel::None;
}

void USyncCombatComponent::SetLastCombatReferenceActor(AActor* InActor)
{
	HitWindowRuntime.SetLastCombatReferenceActor(InActor);
}

const FSyncCombatTagReactionBinding* USyncCombatComponent::FindTagReactionBindingForTriggerTag(const FGameplayTag& TriggerTag) const
{
	if (!TagReactionData || !TriggerTag.IsValid()) return nullptr;

	for (const FSyncCombatTagReactionBinding& Reaction : TagReactionData->Reactions)
	{
		if (!Reaction.TriggerTag.IsValid()) continue;
		if (!TriggerTag.MatchesTag(Reaction.TriggerTag)) continue;

		return &Reaction;
	}

	return nullptr;
}

bool USyncCombatComponent::FindReactionAbilityTag(const FGameplayTag& TriggerTag, FGameplayTag& OutAbilityTag) const
{
	OutAbilityTag = FGameplayTag();

	if (!TagReactionData || !TriggerTag.IsValid()) return false;

	for (const FSyncCombatTagReactionBinding& Reaction : TagReactionData->Reactions)
	{
		if (!Reaction.TriggerTag.IsValid()) continue;
		if (!TriggerTag.MatchesTag(Reaction.TriggerTag)) continue;
		if (!Reaction.Ability.AbilityTag.IsValid()) continue;

		OutAbilityTag = Reaction.Ability.AbilityTag;
		return true;
	}

	return false;
}

bool USyncCombatComponent::HasSuperArmorAtOrAbove(const ESyncCombatHitWindowSuperArmorLevel RequiredSuperArmor) const
{
	if (RequiredSuperArmor == ESyncCombatHitWindowSuperArmorLevel::None)
	{
		return false;
	}

	return GetCurrentSuperArmorLevel() >= RequiredSuperArmor;
}

void USyncCombatComponent::GrantDefaultAbilities()
{
	if (!AbilitySystemComponent || !OwningActor || !OwningActor->HasAuthority()) return;

	for (const USyncCombatAbilitySet* AbilitySet : DefaultAbilitySets)
	{
		if (!AbilitySet) continue;

		AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, &DefaultAbilityHandles, OwningActor);
	}
}

void USyncCombatComponent::ClearDefaultAbilities()
{
	if (!AbilitySystemComponent || !OwningActor || !OwningActor->HasAuthority()) return;

	DefaultAbilityHandles.TakeFromAbilitySystem(AbilitySystemComponent);
}

void USyncCombatComponent::BindCrowdControlTagEvent()
{
	if (!AbilitySystemComponent || !CrowdControlTag.IsValid() || CrowdControlTagDelegateHandle.IsValid()) return;

	CrowdControlTagDelegateHandle = AbilitySystemComponent
		->RegisterGameplayTagEvent(CrowdControlTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::OnCrowdControlTagChanged);

	BoundCrowdControlTag = CrowdControlTag;
}

void USyncCombatComponent::ClearCrowdControlTagEvent()
{
	if (AbilitySystemComponent && BoundCrowdControlTag.IsValid() && CrowdControlTagDelegateHandle.IsValid())
	{
		AbilitySystemComponent
			->RegisterGameplayTagEvent(BoundCrowdControlTag, EGameplayTagEventType::NewOrRemoved)
			.Remove(CrowdControlTagDelegateHandle);
	}

	CrowdControlTagDelegateHandle.Reset();
	BoundCrowdControlTag = FGameplayTag();
}

void USyncCombatComponent::OnCrowdControlTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	APawn* OwningPawn = Cast<APawn>(OwningActor);
	if (!OwningPawn || !OwningPawn->IsLocallyControlled()) return;

	if (AController* ActorController = OwningPawn->GetController())
	{
		ActorController->SetIgnoreMoveInput(NewCount > 0);
	}

	if (NewCount > 0)
	{
		if (const ACharacter* OwningCharacter = Cast<ACharacter>(OwningPawn))
		{
			if (UCharacterMovementComponent* MoveComp = OwningCharacter->GetCharacterMovement())
			{
				MoveComp->bUseControllerDesiredRotation = false;
				MoveComp->bOrientRotationToMovement = false;
			}
		}
	}
}

FActiveGameplayEffectHandle USyncCombatComponent::ApplyEffectToSelf(const TSubclassOf<UGameplayEffect>& GameplayEffectClass,
	float Level) const
{
	if (!GameplayEffectClass || !AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(GetOwner());

	const FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(GameplayEffectClass, Level, ContextHandle);

	return SpecHandle.IsValid()
		? AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get())
		: FActiveGameplayEffectHandle();
}

void USyncCombatComponent::RemoveGameplayEffect(FActiveGameplayEffectHandle& EffectHandle)
{
	if (!AbilitySystemComponent || !EffectHandle.IsValid()) return;

	AbilitySystemComponent->RemoveActiveGameplayEffect(EffectHandle);
	EffectHandle.Invalidate();
}

void USyncCombatComponent::TryAutoInitializeCombat()
{
	if (!bAutoInitializeCombat || AbilitySystemComponent) return;

	AActor* OwnerActor = GetOwner();
	UAbilitySystemComponent* ResolvedAbilitySystemComponent =
		USyncCombatFunctionLibrary::GetAbilitySystemComponent(OwnerActor);

	if (ResolvedAbilitySystemComponent)
	{
		InitializeCombat(OwnerActor, ResolvedAbilitySystemComponent);
		return;
	}

	ScheduleAutoInitializeRetry();
}

void USyncCombatComponent::ScheduleAutoInitializeRetry()
{
	if (!bAutoInitializeCombat || AutoInitializeRetryTimerHandle.IsValid()) return;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			AutoInitializeRetryTimerHandle,
			this,
			&ThisClass::TryAutoInitializeCombat,
			AutoInitializeRetryInterval,
			true);
	}
}

void USyncCombatComponent::ClearAutoInitializeRetry()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoInitializeRetryTimerHandle);
	}

	AutoInitializeRetryTimerHandle.Invalidate();
}

void USyncCombatComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HitWindowRuntime.Tick();
}

bool USyncCombatComponent::BeginHitDetectionWindow(const UAnimNotifyState* NotifyState,
	USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEvent* NotifyEvent,
	const float TotalDuration,
	FName TraceSocketName, const FSyncCombatHitWindowSettings& HitWindowSettings)
{
	const bool bStarted = HitWindowRuntime.BeginHitDetectionWindow(
		NotifyState,
		MeshComp,
		Animation,
		NotifyEvent,
		TotalDuration,
		TraceSocketName,
		HitWindowSettings);

	if (bStarted)
	{
		SetComponentTickEnabled(true);
	}

	return bStarted;
}

void USyncCombatComponent::EndHitDetectionWindow(
	const UAnimNotifyState* NotifyState,
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation)
{
	HitWindowRuntime.EndHitDetectionWindow(NotifyState, MeshComp, Animation);
}
