#include "Components/SP_PredictionComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

USP_PredictionComponent::USP_PredictionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool USP_PredictionComponent::PlayPredictedReactionOnTargetProxy(AActor* TargetActor, FGameplayTag ReactionTag)
{
	if (!ReactionData || !ReactionTag.IsValid())
	{
		return false;
	}

	FSP_ReactionDataEntry Reaction;
	if (!ReactionData->FindReaction(ReactionTag, Reaction))
	{
		return false;
	}

	if (!CanPlayPredictedReactionOnTargetProxy(TargetActor, Reaction))
	{
		return false;
	}

	const FSP_ReactionPredictionContext Context = MakeReactionPredictionContext();

	AddPendingPredictedReaction(Context, TargetActor, ReactionTag);

	const float StartPosition = GetReactionStartPosition(Reaction);

	const bool bPlayed = PlayReactionMontageOnActor(TargetActor, Reaction, StartPosition,
		true);

	if (!bPlayed)
	{
		ConsumePendingPredictedReaction(Context, TargetActor, ReactionTag);
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		LastReactionTimeByTarget.FindOrAdd(TargetActor) = World->GetTimeSeconds();
	}

	ServerConfirmPredictedReaction(Context, TargetActor, ReactionTag);

	return true;
}

bool USP_PredictionComponent::ApplyReactionEffectsToTarget(AActor* TargetActor, FGameplayTag ReactionTag)
{
	if (!TargetActor || !ReactionData || !ReactionTag.IsValid()) return false;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority()) return false;

	FSP_ReactionDataEntry Reaction;
	if (!ReactionData->FindReaction(ReactionTag, Reaction)) return false;

	if (Reaction.TargetEffects.IsEmpty()) return false;

	UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	if (!SourceASC || !TargetASC) return false;

	bool bAppliedAnyEffect = false;

	for (const TSubclassOf<UGameplayEffect>& EffectClass : Reaction.TargetEffects)
	{
		if (!EffectClass) continue;

		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		ContextHandle.AddSourceObject(this);

		const FGameplayEffectSpecHandle SpecHandle =
			SourceASC->MakeOutgoingSpec(EffectClass, 1.f, ContextHandle);

		if (!SpecHandle.IsValid()) continue;

		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		bAppliedAnyEffect = true;
	}

	return bAppliedAnyEffect;
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

FSP_ReactionPredictionContext USP_PredictionComponent::MakeReactionPredictionContext()
{
	FSP_ReactionPredictionContext Context;
	Context.PredictionId = NextPredictionId;

	NextPredictionId = (NextPredictionId + 1) % MaxPredictionId;
	if (NextPredictionId == INDEX_NONE)
	{
		NextPredictionId = 0;
	}

	return Context;
}

void USP_PredictionComponent::RemoveExpiredPendingPredictedReactions()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		PendingPredictedReactions.Reset();
		return;
	}

	const double Now = World->GetTimeSeconds();

	for (int32 Index = PendingPredictedReactions.Num() - 1; Index >= 0; --Index)
	{
		const FSP_PendingPredictedReaction& Entry = PendingPredictedReactions[Index];

		const bool bExpired = Now - Entry.TimeSeconds > PendingPredictedReactionTimeout;
		const bool bInvalid =
			!Entry.TargetActor.IsValid() ||
			!Entry.ReactionTag.IsValid() ||
			Entry.PredictionId == INDEX_NONE;

		if (bExpired || bInvalid)
		{
			PendingPredictedReactions.RemoveAtSwap(Index);
		}
	}
}

void USP_PredictionComponent::AddPendingPredictedReaction(const FSP_ReactionPredictionContext& Context,
	AActor* TargetActor, FGameplayTag ReactionTag)
{
	if (!Context.IsValid() || !TargetActor || !ReactionTag.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	RemoveExpiredPendingPredictedReactions();

	FSP_PendingPredictedReaction& Entry =
		PendingPredictedReactions.AddDefaulted_GetRef();

	Entry.TargetActor = TargetActor;
	Entry.ReactionTag = ReactionTag;
	Entry.PredictionId = Context.PredictionId;
	Entry.TimeSeconds = World->GetTimeSeconds();

	if (PendingPredictedReactions.Num() > MaxPendingPredictedReactions)
	{
		PendingPredictedReactions.RemoveAt(
			0,
			PendingPredictedReactions.Num() - MaxPendingPredictedReactions,
			EAllowShrinking::No);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("SP AddPendingPredictedReaction Target=%s Tag=%s PredictionId=%d Time=%.3f"),
		*GetNameSafe(TargetActor),
		*ReactionTag.ToString(),
		Context.PredictionId,
		Entry.TimeSeconds);
}

bool USP_PredictionComponent::ConsumePendingPredictedReaction(const FSP_ReactionPredictionContext& Context, AActor* TargetActor,
	FGameplayTag ReactionTag)
{
	if (!Context.IsValid() || !TargetActor || !ReactionTag.IsValid())
	{
		return false;
	}

	RemoveExpiredPendingPredictedReactions();

	for (int32 Index = PendingPredictedReactions.Num() - 1; Index >= 0; --Index)
	{
		const FSP_PendingPredictedReaction& Entry = PendingPredictedReactions[Index];

		if (Entry.TargetActor.Get() == TargetActor &&
			Entry.ReactionTag == ReactionTag &&
			Entry.PredictionId == Context.PredictionId)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("SP ConsumePendingPredictedReaction SUCCESS Target=%s Tag=%s PredictionId=%d"),
				*GetNameSafe(TargetActor),
				*ReactionTag.ToString(),
				Context.PredictionId);

			PendingPredictedReactions.RemoveAtSwap(Index);
			return true;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("SP ConsumePendingPredictedReaction FAILED Target=%s Tag=%s PredictionId=%d"),
		*GetNameSafe(TargetActor),
		*ReactionTag.ToString(),
		Context.PredictionId);

	return false;
}

float USP_PredictionComponent::GetReactionStartPosition(
	const FSP_ReactionDataEntry& Reaction) const
{
	if (!Reaction.Montage || Reaction.StartSection == NAME_None)
	{
		return 0.f;
	}

	const int32 SectionIndex = Reaction.Montage->GetSectionIndex(Reaction.StartSection);
	if (SectionIndex == INDEX_NONE)
	{
		return 0.f;
	}

	float SectionStartTime = 0.f;
	float SectionEndTime = 0.f;

	Reaction.Montage->GetSectionStartAndEndTime(SectionIndex, SectionStartTime, SectionEndTime);

	return SectionStartTime;
}

float USP_PredictionComponent::GetServerWorldTimeSecondsSafe() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	const AGameStateBase* GameState = World->GetGameState();
	return GameState ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}

bool USP_PredictionComponent::PlayReactionMontageOnActor(AActor* TargetActor, const FSP_ReactionDataEntry& Reaction,
	float StartPosition, bool bForceRestart) const
{
	if (!TargetActor || !Reaction.Montage)
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

	if (!bForceRestart && AnimInstance->Montage_IsPlaying(Reaction.Montage))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("SP Confirmed reaction already playing, not restarting Target=%s Montage=%s"),
			*GetNameSafe(TargetActor),
			*GetNameSafe(Reaction.Montage));

		return true;
	}

	const FVector BeforeLocation = TargetActor->GetActorLocation();

	const float PlayedLength = AnimInstance->Montage_Play(
		Reaction.Montage,
		Reaction.PlayRate);

	const FVector AfterLocation = TargetActor->GetActorLocation();

	UE_LOG(LogTemp, Warning,
		TEXT("SP MontagePlay Location Target=%s Before=%s After=%s Delta=%s Role=%d"),
		*GetNameSafe(TargetActor),
		*BeforeLocation.ToString(),
		*AfterLocation.ToString(),
		*(AfterLocation - BeforeLocation).ToString(),
		TargetActor ? static_cast<int32>(TargetActor->GetLocalRole()) : -1);

	if (PlayedLength <= 0.f)
	{
		return false;
	}

	if (UWorld* World = TargetActor->GetWorld())
	{
		TWeakObjectPtr<AActor> WeakTargetActor(TargetActor);
		TWeakObjectPtr<ACharacter> WeakTargetCharacter(TargetCharacter);
		TSharedRef<int32> TickCount = MakeShared<int32>(0);
		TSharedRef<FTimerHandle> TimerHandle = MakeShared<FTimerHandle>();

		World->GetTimerManager().SetTimer(
			*TimerHandle,
			[World, WeakTargetActor, WeakTargetCharacter, TickCount, TimerHandle]()
			{
				AActor* TimerTargetActor = WeakTargetActor.Get();
				ACharacter* TimerTargetCharacter = WeakTargetCharacter.Get();

				if (!TimerTargetActor || !TimerTargetCharacter)
				{
					World->GetTimerManager().ClearTimer(*TimerHandle);
					return;
				}

				const UCharacterMovementComponent* MovementComponent =
					TimerTargetCharacter->GetCharacterMovement();

				const FString VelocityString = MovementComponent
					? MovementComponent->Velocity.ToString()
					: TEXT("None");

				UE_LOG(LogTemp, Warning,
					TEXT("SP ReactionLocation Target=%s Loc=%s Velocity=%s Role=%d"),
					*GetNameSafe(TimerTargetActor),
					*TimerTargetActor->GetActorLocation().ToString(),
					*VelocityString,
					static_cast<int32>(TimerTargetActor->GetLocalRole()));

				++(*TickCount);

				if (*TickCount >= 20)
				{
					World->GetTimerManager().ClearTimer(*TimerHandle);
				}
			},
			0.05f,
			true);
	}

	const float MontageLength = Reaction.Montage->GetPlayLength();
	const float ClampedStartPosition = FMath::Clamp(
		StartPosition,
		0.f,
		FMath::Max(0.f, MontageLength - KINDA_SMALL_NUMBER));

	if (ClampedStartPosition > KINDA_SMALL_NUMBER)
	{
		AnimInstance->Montage_SetPosition(
			Reaction.Montage,
			ClampedStartPosition);
	}
	else if (Reaction.StartSection != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(
			Reaction.StartSection,
			Reaction.Montage);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("SP PlayReactionMontage Target=%s Montage=%s Start=%.3f ForceRestart=%d"),
		*GetNameSafe(TargetActor),
		*GetNameSafe(Reaction.Montage),
		ClampedStartPosition,
		bForceRestart);

	return true;
}

void USP_PredictionComponent::ServerConfirmPredictedReaction_Implementation(FSP_ReactionPredictionContext Context,
	AActor* TargetActor, FGameplayTag ReactionTag)
{
	AActor* OwnerActor = GetOwner();

	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (!Context.IsValid() || !TargetActor || !ReactionData || !ReactionTag.IsValid())
	{
		return;
	}

	FSP_ReactionDataEntry Reaction;
	if (!ReactionData->FindReaction(ReactionTag, Reaction))
	{
		return;
	}

	const float StartPosition = GetReactionStartPosition(Reaction);

	const bool bPlayedOnServer = PlayReactionMontageOnActor(TargetActor, Reaction, StartPosition,
		true);

	UE_LOG(LogTemp, Warning,
		TEXT("SP ServerConfirmPredictedReaction Owner=%s Target=%s Tag=%s Montage=%s PlayedOnServer=%d PredictionId=%d"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(TargetActor),
		*ReactionTag.ToString(),
		*GetNameSafe(Reaction.Montage),
		bPlayedOnServer,
		Context.PredictionId);

	if (USP_PredictionComponent* TargetPredictionComponent =
		TargetActor->FindComponentByClass<USP_PredictionComponent>())
	{
		TargetPredictionComponent->ClientPlayOwnerConfirmedReaction(Context, TargetActor, OwnerActor, ReactionTag);
	}

	const float ServerStartTime = GetServerWorldTimeSecondsSafe();

	MulticastPlayConfirmedReaction(Context, TargetActor, ReactionTag, ServerStartTime);
}

void USP_PredictionComponent::MulticastPlayConfirmedReaction_Implementation(FSP_ReactionPredictionContext Context,
	AActor* TargetActor, FGameplayTag ReactionTag, float ServerStartTime)
{
	AActor* OwnerActor = GetOwner();

	if (!OwnerActor || OwnerActor->HasAuthority())
	{
		return;
	}

	if (!TargetActor || !ReactionData || !ReactionTag.IsValid())
	{
		return;
	}

	if (ConsumePendingPredictedReaction(Context, TargetActor, ReactionTag))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("SP Multicast confirmed reaction consumed predicted local replay Owner=%s Target=%s Tag=%s PredictionId=%d"),
			*GetNameSafe(OwnerActor),
			*GetNameSafe(TargetActor),
			*ReactionTag.ToString(),
			Context.PredictionId);

		return;
	}

	const APawn* TargetPawn = Cast<APawn>(TargetActor);
	if (TargetPawn && TargetPawn->IsLocallyControlled())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("SP Multicast confirmed reaction skipped locally controlled target Target=%s Tag=%s PredictionId=%d"),
			*GetNameSafe(TargetActor),
			*ReactionTag.ToString(),
			Context.PredictionId);

		return;
	}

	FSP_ReactionDataEntry Reaction;
	if (!ReactionData->FindReaction(ReactionTag, Reaction))
	{
		return;
	}

	const float ServerElapsed = FMath::Max(
		0.f,
		GetServerWorldTimeSecondsSafe() - ServerStartTime);

	const float StartPosition = GetReactionStartPosition(Reaction) + ServerElapsed;

	const bool bPlayed = PlayReactionMontageOnActor(
		TargetActor,
		Reaction,
		StartPosition,
		false);

	UE_LOG(LogTemp, Warning,
		TEXT("SP Multicast confirmed reaction played Owner=%s Target=%s Tag=%s Start=%.3f Played=%d PredictionId=%d"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(TargetActor),
		*ReactionTag.ToString(),
		StartPosition,
		bPlayed,
		Context.PredictionId);
}

void USP_PredictionComponent::ClientPlayOwnerConfirmedReaction_Implementation(FSP_ReactionPredictionContext Context,
	AActor* ExpectedTargetActor, AActor* InstigatorActor, FGameplayTag ReactionTag)
{
	AActor* OwnerActor = GetOwner();

	if (!OwnerActor || OwnerActor->HasAuthority())
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("SP ClientOwnerReaction skipped non locally controlled owner Owner=%s Instigator=%s PredictionId=%d"),
			*GetNameSafe(OwnerActor),
			*GetNameSafe(InstigatorActor),
			Context.PredictionId);

		return;
	}

	if (OwnerActor == InstigatorActor)
	{
		UE_LOG(LogTemp, Error,
			TEXT("SP ClientOwnerReaction blocked because OwnerActor == InstigatorActor Owner=%s PredictionId=%d"),
			*GetNameSafe(OwnerActor),
			Context.PredictionId);

		return;
	}

	if (OwnerActor != ExpectedTargetActor)
	{
		UE_LOG(LogTemp, Error,
			TEXT("SP ClientOwnerReaction WRONG OWNER ComponentOwner=%s ExpectedTarget=%s Instigator=%s PredictionId=%d"),
			*GetNameSafe(OwnerActor),
			*GetNameSafe(ExpectedTargetActor),
			*GetNameSafe(InstigatorActor),
			Context.PredictionId);

		return;
	}

	if (!ReactionData || !ReactionTag.IsValid())
	{
		return;
	}

	if (ConsumePendingPredictedReaction(Context, OwnerActor, ReactionTag))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("SP Client owner confirmed reaction consumed predicted local replay Owner=%s Tag=%s PredictionId=%d"),
			*GetNameSafe(OwnerActor),
			*ReactionTag.ToString(),
			Context.PredictionId);

		return;
	}

	FSP_ReactionDataEntry Reaction;
	if (!ReactionData->FindReaction(ReactionTag, Reaction))
	{
		return;
	}

	const float StartPosition = GetReactionStartPosition(Reaction);

	const bool bPlayed = PlayReactionMontageOnActor(OwnerActor, Reaction, StartPosition,
		true);

	UE_LOG(LogTemp, Warning,
		TEXT("SP Client owner confirmed reaction Owner=%s Instigator=%s Tag=%s Played=%d PredictionId=%d"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(InstigatorActor),
		*ReactionTag.ToString(),
		bPlayed,
		Context.PredictionId);
}
