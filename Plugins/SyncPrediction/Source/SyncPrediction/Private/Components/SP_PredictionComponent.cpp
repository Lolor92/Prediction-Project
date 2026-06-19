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

	float PredictedReactionAge = 0.f;
	if (!ConsumePendingPredictedReaction(OwnerActor, ReactionTag, &PredictedReactionAge)) return;

	UAnimInstance* AnimInstance = BoundAnimInstance.Get();
	if (!AnimInstance) return;

	const float MontageLength = Montage->GetPlayLength();

	if (MontageLength <= 0.f) return;

	const float CorrectedMontagePosition = FMath::Clamp(PredictedReactionAge, 0.f,
		FMath::Max(0.f, MontageLength - KINDA_SMALL_NUMBER));

	UE_LOG(LogTemp, Warning,
		TEXT("SP Skipped predicted reaction replay Owner=%s Montage=%s Tag=%s Age=%.3f NewPosition=%.3f"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(Montage),
		*ReactionTag.ToString(),
		PredictedReactionAge,
		CorrectedMontagePosition);
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

	const float PlayedLength = AnimInstance->Montage_Play(Reaction.Montage, Reaction.PlayRate);
	if (PlayedLength <= 0.f)
	{
		return false;
	}

	if (USP_PredictionComponent* TargetPredictionComponent = TargetActor->FindComponentByClass<USP_PredictionComponent>())
	{
		TargetPredictionComponent->AddPendingPredictedReaction(TargetActor, ReactionTag);
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
