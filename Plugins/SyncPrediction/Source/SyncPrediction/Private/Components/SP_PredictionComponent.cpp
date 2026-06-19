#include "Components/SP_PredictionComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

USP_PredictionComponent::USP_PredictionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USP_PredictionComponent::BeginPlay()
{
	Super::BeginPlay();
	BindToOwnerAnimInstance();
}

void USP_PredictionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundAnimInstance.IsValid())
	{
		BoundAnimInstance->OnMontageStarted.RemoveDynamic(this, &USP_PredictionComponent::HandleOwnerMontageStarted);
		BoundAnimInstance.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void USP_PredictionComponent::BindToOwnerAnimInstance()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	USkeletalMeshComponent* MeshComp = nullptr;

	if (const ACharacter* CharacterOwner = Cast<ACharacter>(OwnerActor))
	{
		MeshComp = CharacterOwner->GetMesh();
	}
	else
	{
		MeshComp = OwnerActor->FindComponentByClass<USkeletalMeshComponent>();
	}

	if (!MeshComp) return;

	UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	if (!AnimInstance) return;

	if (BoundAnimInstance.Get() == AnimInstance) return;

	if (BoundAnimInstance.IsValid())
	{
		BoundAnimInstance->OnMontageStarted.RemoveDynamic(this, &USP_PredictionComponent::HandleOwnerMontageStarted);
	}

	BoundAnimInstance = AnimInstance;
	AnimInstance->OnMontageStarted.AddDynamic(this, &USP_PredictionComponent::HandleOwnerMontageStarted);

	UE_LOG(LogTemp, Warning, TEXT("SP Bound montage start Owner=%s AnimInstance=%s"),
		*GetNameSafe(OwnerActor), *GetNameSafe(AnimInstance));
}

void USP_PredictionComponent::HandleOwnerMontageStarted(UAnimMontage* Montage)
{
	AActor* OwnerActor = GetOwner();

	UE_LOG(LogTemp, Warning,
		TEXT("SP MontageStarted Owner=%s Montage=%s LocalRole=%d RemoteRole=%d HasAuthority=%d"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(Montage),
		OwnerActor ? static_cast<int32>(OwnerActor->GetLocalRole()) : -1,
		OwnerActor ? static_cast<int32>(OwnerActor->GetRemoteRole()) : -1,
		OwnerActor ? OwnerActor->HasAuthority() : false);

	if (!OwnerActor || !Montage) return;

	// Only fix cosmetic replay on simulated proxies.
	// Server and owning client should keep normal authoritative behavior.
	if (OwnerActor->GetLocalRole() != ROLE_SimulatedProxy) return;

	const FGameplayTag ReactionTag = FindReactionTagForMontage(Montage);
	if (!ReactionTag.IsValid()) return;

	FSP_ActivePredictedReaction* ActiveReaction =
	FindActivePredictedReaction(OwnerActor, Montage, ReactionTag);

	if (!ActiveReaction) return;

	UAnimInstance* AnimInstance = BoundAnimInstance.Get();
	if (!AnimInstance) return;

	if (ActiveReaction->bIgnoreNextMontageStarted)
	{
		ActiveReaction->bIgnoreNextMontageStarted = false;

		UE_LOG(LogTemp, Warning,
			TEXT("SP Ignore local predicted MontageStarted Owner=%s Montage=%s Tag=%s"),
			*GetNameSafe(OwnerActor),
			*GetNameSafe(Montage),
			*ReactionTag.ToString());

		return;
	}

	const float LocalCosmeticPosition =
		GetActivePredictedReactionMontagePosition(*ActiveReaction);

	const float MontageLength = Montage->GetPlayLength();

	if (MontageLength <= 0.f) return;

	if (LocalCosmeticPosition >= MontageLength - KINDA_SMALL_NUMBER)
	{
		AnimInstance->Montage_Stop(0.f, Montage);

		RemoveActivePredictedReaction(OwnerActor, Montage, ReactionTag);

		UE_LOG(LogTemp, Warning,
			TEXT("SP Blocked late replicated reaction restart Owner=%s Montage=%s Tag=%s LocalCosmeticPosition=%.3f"),
			*GetNameSafe(OwnerActor),
			*GetNameSafe(Montage),
			*ReactionTag.ToString(),
			LocalCosmeticPosition);

		return;
	}

	// The server montage already tried to restart this visual.
	// Restore the LOCAL predicted cosmetic timeline, not the server timeline.
	AnimInstance->Montage_SetPosition(Montage, LocalCosmeticPosition);

	UE_LOG(LogTemp, Warning,
		TEXT("SP Ignored replicated reaction restart Owner=%s Montage=%s Tag=%s LocalCosmeticPosition=%.3f"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(Montage),
		*ReactionTag.ToString(),
		LocalCosmeticPosition);
}

FGameplayTag USP_PredictionComponent::FindReactionTagForMontage(const UAnimMontage* Montage) const
{
	if (!Montage || !ReactionData)
	{
		return FGameplayTag();
	}

	for (const FSP_ReactionDataEntry& ReactionEntry : ReactionData->Reactions)
	{
		if (ReactionEntry.Montage == Montage)
		{
			return ReactionEntry.ReactionTag;
		}
	}

	return FGameplayTag();
}

void USP_PredictionComponent::AddActivePredictedReaction(AActor* TargetActor, UAnimMontage* Montage,
	FGameplayTag ReactionTag, float MontageStartPositionSeconds)
{
	if (!TargetActor || !Montage || !ReactionTag.IsValid()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	RemoveExpiredActivePredictedReactions();

	RemoveActivePredictedReaction(TargetActor, Montage, ReactionTag);

	FSP_ActivePredictedReaction& ActiveReaction = ActivePredictedReactions.AddDefaulted_GetRef();

	ActiveReaction.TargetActor = TargetActor;
	ActiveReaction.Montage = Montage;
	ActiveReaction.ReactionTag = ReactionTag;
	ActiveReaction.StartTimeSeconds = World->GetTimeSeconds();
	ActiveReaction.MontageStartPositionSeconds = MontageStartPositionSeconds;
	ActiveReaction.bIgnoreNextMontageStarted = true;

	UE_LOG(LogTemp, Warning,
		TEXT("SP AddActivePredictedReaction Target=%s Montage=%s Tag=%s Time=%.3f StartPos=%.3f"),
		*GetNameSafe(TargetActor),
		*GetNameSafe(Montage),
		*ReactionTag.ToString(),
		ActiveReaction.StartTimeSeconds,
		MontageStartPositionSeconds);
}

void USP_PredictionComponent::RemoveActivePredictedReaction(AActor* TargetActor, UAnimMontage* Montage,
	FGameplayTag ReactionTag)
{
	ActivePredictedReactions.RemoveAll(
		[TargetActor, Montage, ReactionTag](const FSP_ActivePredictedReaction& ActiveReaction)
		{
			return ActiveReaction.TargetActor.Get() == TargetActor &&
				ActiveReaction.Montage == Montage &&
				ActiveReaction.ReactionTag == ReactionTag;
		});
}

FSP_ActivePredictedReaction* USP_PredictionComponent::FindActivePredictedReaction(AActor* TargetActor,
	UAnimMontage* Montage, FGameplayTag ReactionTag)
{
	if (!TargetActor || !Montage || !ReactionTag.IsValid()) return nullptr;

	RemoveExpiredActivePredictedReactions();

	for (FSP_ActivePredictedReaction& ActiveReaction : ActivePredictedReactions)
	{
		if (ActiveReaction.TargetActor.Get() == TargetActor &&
			ActiveReaction.Montage == Montage &&
			ActiveReaction.ReactionTag == ReactionTag)
		{
			return &ActiveReaction;
		}
	}

	return nullptr;
}

void USP_PredictionComponent::RemoveExpiredActivePredictedReactions()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		ActivePredictedReactions.Reset();
		return;
	}

	const float Now = World->GetTimeSeconds();

	for (int32 Index = ActivePredictedReactions.Num() - 1; Index >= 0; --Index)
	{
		const FSP_ActivePredictedReaction& ActiveReaction = ActivePredictedReactions[Index];

		const bool bExpired =
			(Now - ActiveReaction.StartTimeSeconds) > ActivePredictedReactionTimeout;

		const bool bInvalid =
			!ActiveReaction.TargetActor.IsValid() ||
			!ActiveReaction.Montage ||
			!ActiveReaction.ReactionTag.IsValid();

		if (bExpired || bInvalid)
		{
			ActivePredictedReactions.RemoveAtSwap(Index);
		}
	}
}

float USP_PredictionComponent::GetActivePredictedReactionMontagePosition(
	const FSP_ActivePredictedReaction& ActiveReaction) const
{
	const UWorld* World = GetWorld();
	if (!World || !ActiveReaction.Montage)
	{
		return 0.f;
	}

	const float LocalElapsed =
		World->GetTimeSeconds() - ActiveReaction.StartTimeSeconds;

	const float MontageLength = ActiveReaction.Montage->GetPlayLength();

	return FMath::Clamp(
		ActiveReaction.MontageStartPositionSeconds + LocalElapsed,
		0.f,
		FMath::Max(0.f, MontageLength - KINDA_SMALL_NUMBER));
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

	float MontageStartPositionSeconds = 0.f;

	if (Reaction.StartSection != NAME_None)
	{
		const int32 SectionIndex = Reaction.Montage->GetSectionIndex(Reaction.StartSection);

		if (SectionIndex != INDEX_NONE)
		{
			float SectionStartTime = 0.f;
			float SectionEndTime = 0.f;
			Reaction.Montage->GetSectionStartAndEndTime(SectionIndex, SectionStartTime, SectionEndTime);

			MontageStartPositionSeconds = SectionStartTime;
		}
	}

	USP_PredictionComponent* TargetPredictionComponent =
		TargetActor->FindComponentByClass<USP_PredictionComponent>();

	if (TargetPredictionComponent)
	{
		TargetPredictionComponent->AddActivePredictedReaction(TargetActor, Reaction.Montage, ReactionTag,
			MontageStartPositionSeconds);
	}

	const float PlayedLength = AnimInstance->Montage_Play(Reaction.Montage, Reaction.PlayRate);

	if (PlayedLength <= 0.f)
	{
		if (TargetPredictionComponent)
		{
			TargetPredictionComponent->RemoveActivePredictedReaction(TargetActor, Reaction.Montage, ReactionTag);
		}

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

void USP_PredictionComponent::AddPendingPredictedReaction(AActor* TargetActor, FGameplayTag ReactionTag)
{
	if (!TargetActor || !ReactionTag.IsValid()) return;

	UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;

	PendingPredictedReactions.RemoveAll([this, Now](const FSP_PendingPredictedReaction& Entry)
	{
		return !Entry.TargetActor.IsValid() || Now - Entry.TimeSeconds > PendingPredictedReactionTimeout;
	});

	FSP_PendingPredictedReaction& Entry = PendingPredictedReactions.AddDefaulted_GetRef();
	Entry.TargetActor = TargetActor;
	Entry.ReactionTag = ReactionTag;
	Entry.TimeSeconds = Now;

	UE_LOG(LogTemp, Warning, TEXT("SP AddPendingPredictedReaction Target=%s Tag=%s Time=%.3f"),
		*GetNameSafe(TargetActor), *ReactionTag.ToString(), Now);
}

bool USP_PredictionComponent::ConsumePendingPredictedReaction(AActor* TargetActor, FGameplayTag ReactionTag, float* OutAgeSeconds)
{
	if (!TargetActor || !ReactionTag.IsValid()) return false;

	const UWorld* World = GetWorld();
	if (!World) return false;

	const float CurrentTime = World->GetTimeSeconds();

	for (int32 Index = PendingPredictedReactions.Num() - 1; Index >= 0; --Index)
	{
		const FSP_PendingPredictedReaction& PendingReaction = PendingPredictedReactions[Index];

		const bool bExpired = (CurrentTime - PendingReaction.TimeSeconds) > PendingPredictedReactionTimeout;

		const bool bInvalid =
			!PendingReaction.TargetActor.IsValid() ||
			!PendingReaction.ReactionTag.IsValid();

		if (bExpired || bInvalid)
		{
			PendingPredictedReactions.RemoveAtSwap(Index);
			continue;
		}

		if (PendingReaction.TargetActor.Get() == TargetActor && PendingReaction.ReactionTag == ReactionTag)
		{
			if (OutAgeSeconds)
			{
				*OutAgeSeconds = CurrentTime - PendingReaction.TimeSeconds;
			}

			UE_LOG(LogTemp, Warning,
				TEXT("SP ConsumePendingPredictedReaction SUCCESS Target=%s Tag=%s Age=%.3f"),
				*GetNameSafe(TargetActor),
				*ReactionTag.ToString(),
				CurrentTime - PendingReaction.TimeSeconds);

			PendingPredictedReactions.RemoveAtSwap(Index);
			return true;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("SP ConsumePendingPredictedReaction FAILED Target=%s Tag=%s"),
		*GetNameSafe(TargetActor),
		*ReactionTag.ToString());

	return false;
}
