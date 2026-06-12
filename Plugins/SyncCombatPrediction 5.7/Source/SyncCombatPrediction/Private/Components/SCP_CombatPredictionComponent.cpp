#include "Components/SCP_CombatPredictionComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BlueprintLibrary/SCP_CombatPredictionBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/SCP_ReactionData.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSyncCombatPrediction, Log, All);

namespace SyncCombatPrediction
{
	UAbilitySystemComponent* GetAbilitySystemComponent(AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}

		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			if (APlayerState* PlayerState = Pawn->GetPlayerState())
			{
				if (UAbilitySystemComponent* ASC =
					UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerState))
				{
					return ASC;
				}
			}
		}

		return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	}

	FGameplayTag ResolveDamageTag(const FGameplayTag& ConfiguredTag, const TCHAR* DefaultTagName)
	{
		if (ConfiguredTag.IsValid())
		{
			return ConfiguredTag;
		}

		return FGameplayTag::RequestGameplayTag(FName(DefaultTagName), false);
	}
}

USCP_CombatPredictionComponent::USCP_CombatPredictionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

FSCP_CombatPredictionContext USCP_CombatPredictionComponent::StartPredictionFromAbility(
	const UGameplayAbility* Ability)
{
	ActivePredictionContext =
		USCP_CombatPredictionBlueprintLibrary::MakePredictionContextFromAbility(Ability);

	bHasActivePrediction = ActivePredictionContext.IsValidForPrediction();
	ResetPredictionEvents();

	return ActivePredictionContext;
}

void USCP_CombatPredictionComponent::ClearActivePrediction()
{
	ActivePredictionContext = FSCP_CombatPredictionContext();
	bHasActivePrediction = false;
	ResetPredictionEvents();
	ProcessedTargets.Reset();
}

FSCP_CombatPredictionContext USCP_CombatPredictionComponent::BeginPredictionEvent()
{
	FSCP_CombatPredictionContext EventContext = ActivePredictionContext;
	EventContext.PredictionEventId = AllocatePredictionEventId();
	return EventContext;
}

bool USCP_CombatPredictionComponent::HasProcessedTarget(
	const FSCP_CombatPredictionContext& Context,
	AActor* TargetActor) const
{
	if (!TargetActor || !Context.IsValidForPrediction())
	{
		return false;
	}

	for (const FSCP_ProcessedPredictionTarget& Entry : ProcessedTargets)
	{
		if (Entry.TargetActor == TargetActor &&
			Entry.AbilityPredictionKey == Context.AbilityPredictionKey &&
			Entry.AbilitySpecHandle == Context.AbilitySpecHandle &&
			Entry.PredictionEventId == Context.PredictionEventId)
		{
			return true;
		}
	}

	return false;
}

bool USCP_CombatPredictionComponent::MarkTargetProcessed(
	const FSCP_CombatPredictionContext& Context,
	AActor* TargetActor)
{
	if (!TargetActor || !Context.IsValidForPrediction())
	{
		return false;
	}

	if (HasProcessedTarget(Context, TargetActor))
	{
		return false;
	}

	FSCP_ProcessedPredictionTarget Entry;
	Entry.TargetActor = TargetActor;
	Entry.AbilityPredictionKey = Context.AbilityPredictionKey;
	Entry.AbilitySpecHandle = Context.AbilitySpecHandle;
	Entry.PredictionEventId = Context.PredictionEventId;

	ProcessedTargets.Add(Entry);

	if (ProcessedTargets.Num() > MaxProcessedTargets)
	{
		ProcessedTargets.RemoveAt(
			0,
			ProcessedTargets.Num() - MaxProcessedTargets,
			EAllowShrinking::No);
	}

	return true;
}

bool USCP_CombatPredictionComponent::ReportHit(
	const FSCP_CombatPredictionContext& Context,
	const FHitResult& HitResult,
	FGameplayTag ReactionTag,
	const FSCP_GameplayEffectSettings& GameplayEffects)
{
	return ReportHitWithTransformSettings(
		Context,
		HitResult,
		ReactionTag,
		GameplayEffects,
		FSCP_HitTransformSettings());
}

bool USCP_CombatPredictionComponent::ReportHitWithTransformSettings(
	const FSCP_CombatPredictionContext& Context,
	const FHitResult& HitResult,
	FGameplayTag ReactionTag,
	const FSCP_GameplayEffectSettings& GameplayEffects,
	const FSCP_HitTransformSettings& TransformSettings)
{
	return ReportHitWithSettings(
		Context,
		HitResult,
		ReactionTag,
		GameplayEffects,
		TransformSettings,
		FSCP_HitDefenseSettings(),
		FSCP_HitDamageDefenseSettings());
}

bool USCP_CombatPredictionComponent::ReportHitWithSettings(
	const FSCP_CombatPredictionContext& Context,
	const FHitResult& HitResult,
	FGameplayTag ReactionTag,
	const FSCP_GameplayEffectSettings& GameplayEffects,
	const FSCP_HitTransformSettings& TransformSettings,
	const FSCP_HitDefenseSettings& DefenseSettings,
	const FSCP_HitDamageDefenseSettings& DamageDefenseSettings)
{
	AActor* TargetActor = HitResult.GetActor();
	if (!MarkTargetProcessed(Context, TargetActor))
	{
		return false;
	}

	FSCP_PredictedHit Hit;
	Hit.PredictionContext = Context;
	Hit.InstigatorActor = GetOwner();
	Hit.TargetActor = TargetActor;
	Hit.HitResult = HitResult;
	Hit.ReactionTag = ReactionTag;
	Hit.GameplayEffects = GameplayEffects;
	Hit.TransformSettings = TransformSettings;
	Hit.DefenseSettings = DefenseSettings;
	Hit.DamageDefenseSettings = DamageDefenseSettings;
	Hit.bIsAuthority = Context.bIsAuthority;

	if (bLogReportedHits)
	{
		UE_LOG(
			LogSyncCombatPrediction,
			Log,
			TEXT("%s Hit Context={%s} Instigator={%s} Target={%s} Impact=%s"),
			Context.bIsAuthority ? TEXT("Authority") : TEXT("Predicted"),
			*Context.ToDebugString(),
			*BuildActorDebugString(Hit.InstigatorActor),
			*BuildActorDebugString(TargetActor),
			*HitResult.ImpactPoint.ToCompactString());
	}

	if (Context.bIsAuthority)
	{
		OnAuthorityHit.Broadcast(Hit);
		ApplyGameplayEffectsFromHit(Hit);
		ApplyReactionDataTargetEffectsFromHit(Hit);

		if (bAutoConfirmAuthorityReactions && ReactionData)
		{
			if (UAnimMontage* ReactionMontage = ReactionData->FindReactionMontage(ReactionTag))
			{
				ConfirmTargetReaction(Context, TargetActor, ReactionMontage, TransformSettings, DefenseSettings);
			}
		}
	}
	else
	{
		OnPredictedHit.Broadcast(Hit);

		if (bAutoPredictReactions)
		{
			PredictTargetReactionFromHit(Hit);
		}
	}

	return true;
}

bool USCP_CombatPredictionComponent::PredictTargetReactionFromHit(
	const FSCP_PredictedHit& Hit)
{
	UAnimMontage* ReactionMontage = ReactionData
		? ReactionData->FindReactionMontage(Hit.ReactionTag)
		: nullptr;

	if (!ReactionMontage)
	{
		return false;
	}

	return PredictTargetReaction(Hit, ReactionMontage);
}

bool USCP_CombatPredictionComponent::PredictTargetReaction(
	const FSCP_PredictedHit& Hit,
	UAnimMontage* ReactionMontage)
{
	if (!ReactionMontage || !Hit.TargetActor || !Hit.PredictionContext.IsValidForPrediction())
	{
		return false;
	}

	if (Hit.PredictionContext.bIsAuthority)
	{
		return false;
	}

	const bool bShouldApplyCleanReaction = ShouldApplyCleanHitReaction(
		Hit.InstigatorActor,
		Hit.TargetActor,
		Hit.DefenseSettings);

	ApplyHitTransformEffects(
		Hit.InstigatorActor,
		Hit.TargetActor,
		Hit.TransformSettings,
		Hit.DefenseSettings);

	if (!bShouldApplyCleanReaction)
	{
		ServerConfirmTargetReactionWithSettings(
			Hit.PredictionContext,
			Hit.TargetActor,
			ReactionMontage,
			Hit.TransformSettings,
			Hit.DefenseSettings);
		return false;
	}

	AddPendingPredictedReaction(Hit.PredictionContext, Hit.TargetActor);
	if (USCP_CombatPredictionComponent* TargetPredictionComponent =
		Hit.TargetActor->FindComponentByClass<USCP_CombatPredictionComponent>())
	{
		const float PredictedDuration =
			ReactionMontage->GetPlayLength() +
			ReactionMontage->GetDefaultBlendOutTime() +
			0.25f;
		TargetPredictionComponent->BeginPredictedTargetReaction(PredictedDuration);
	}

	const bool bPlayed = PlayReactionMontageOnActor(Hit.TargetActor, ReactionMontage, 0.f, true);

	ServerConfirmTargetReactionWithSettings(
		Hit.PredictionContext,
		Hit.TargetActor,
		ReactionMontage,
		Hit.TransformSettings,
		Hit.DefenseSettings);

	return bPlayed;
}

void USCP_CombatPredictionComponent::ServerConfirmTargetReaction_Implementation(
	FSCP_CombatPredictionContext Context,
	AActor* TargetActor,
	UAnimMontage* ReactionMontage)
{
	ConfirmTargetReaction(
		Context,
		TargetActor,
		ReactionMontage,
		FSCP_HitTransformSettings(),
		FSCP_HitDefenseSettings());
}

void USCP_CombatPredictionComponent::ServerConfirmTargetReactionWithTransform_Implementation(
	FSCP_CombatPredictionContext Context,
	AActor* TargetActor,
	UAnimMontage* ReactionMontage,
	FSCP_HitTransformSettings TransformSettings)
{
	ConfirmTargetReaction(
		Context,
		TargetActor,
		ReactionMontage,
		TransformSettings,
		FSCP_HitDefenseSettings());
}

void USCP_CombatPredictionComponent::ServerConfirmTargetReactionWithSettings_Implementation(
	FSCP_CombatPredictionContext Context,
	AActor* TargetActor,
	UAnimMontage* ReactionMontage,
	FSCP_HitTransformSettings TransformSettings,
	FSCP_HitDefenseSettings DefenseSettings)
{
	ConfirmTargetReaction(Context, TargetActor, ReactionMontage, TransformSettings, DefenseSettings);
}

void USCP_CombatPredictionComponent::ConfirmTargetReaction(
	FSCP_CombatPredictionContext Context,
	AActor* TargetActor,
	UAnimMontage* ReactionMontage,
	const FSCP_HitTransformSettings& TransformSettings,
	const FSCP_HitDefenseSettings& DefenseSettings)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !TargetActor || !ReactionMontage)
	{
		return;
	}

	Context.bIsAuthority = true;
	Context.bIsLocallyControlled = false;

	ApplyHitTransformEffects(GetOwner(), TargetActor, TransformSettings, DefenseSettings);
	const bool bShouldApplyCleanReaction = ShouldApplyCleanHitReaction(GetOwner(), TargetActor, DefenseSettings);
	if (bShouldApplyCleanReaction)
	{
		PlayReactionMontageOnActor(TargetActor, ReactionMontage, 0.f, true);
	}

	if (USCP_CombatPredictionComponent* TargetPredictionComponent =
		TargetActor->FindComponentByClass<USCP_CombatPredictionComponent>())
	{
		const float ToleranceDuration =
			ReactionMontage->GetPlayLength() +
			ReactionMontage->GetDefaultBlendOutTime() +
			0.05f;

		TargetPredictionComponent->BeginTargetReactionMovementTolerance(ToleranceDuration);
		TargetPredictionComponent->ClientPlayOwnerTargetReactionWithTransform(
			GetOwner(),
			bShouldApplyCleanReaction ? ReactionMontage : nullptr,
			TransformSettings,
			DefenseSettings);
	}

	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	const float ServerStartTime = GameState ? GameState->GetServerWorldTimeSeconds() : 0.f;

	MulticastPlayConfirmedTargetReactionWithTransform(
		Context,
		GetOwner(),
		TargetActor,
		bShouldApplyCleanReaction ? ReactionMontage : nullptr,
		ServerStartTime,
		TransformSettings,
		DefenseSettings);
}

void USCP_CombatPredictionComponent::MulticastPlayConfirmedTargetReaction_Implementation(
	FSCP_CombatPredictionContext Context,
	AActor* TargetActor,
	UAnimMontage* ReactionMontage,
	float ServerStartTime)
{
	MulticastPlayConfirmedTargetReactionWithTransform_Implementation(
		Context,
		GetOwner(),
		TargetActor,
		ReactionMontage,
		ServerStartTime,
		FSCP_HitTransformSettings(),
		FSCP_HitDefenseSettings());
}

void USCP_CombatPredictionComponent::MulticastPlayConfirmedTargetReactionWithTransform_Implementation(
	FSCP_CombatPredictionContext Context,
	AActor* InstigatorActor,
	AActor* TargetActor,
	UAnimMontage* ReactionMontage,
	float ServerStartTime,
	FSCP_HitTransformSettings TransformSettings,
	FSCP_HitDefenseSettings DefenseSettings)
{
	if (!TargetActor)
	{
		return;
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return;
	}

	if (ConsumePendingPredictedReaction(Context, TargetActor))
	{
		return;
	}

	const APawn* TargetPawn = Cast<APawn>(TargetActor);
	if (TargetPawn && TargetPawn->IsLocallyControlled())
	{
		return;
	}

	ApplyHitTransformEffects(InstigatorActor, TargetActor, TransformSettings, DefenseSettings);

	if (ReactionMontage)
	{
		const UWorld* World = GetWorld();
		const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
		const float CurrentServerTime = GameState ? GameState->GetServerWorldTimeSeconds() : ServerStartTime;
		const float StartPosition = FMath::Clamp(
			CurrentServerTime - ServerStartTime,
			0.f,
			ReactionMontage->GetPlayLength());

		PlayReactionMontageOnActor(TargetActor, ReactionMontage, StartPosition, false);
	}
}

void USCP_CombatPredictionComponent::ClientPlayOwnerTargetReaction_Implementation(
	UAnimMontage* ReactionMontage)
{
	ClientPlayOwnerTargetReactionWithTransform_Implementation(
		nullptr,
		ReactionMontage,
		FSCP_HitTransformSettings(),
		FSCP_HitDefenseSettings());
}

void USCP_CombatPredictionComponent::ClientPlayOwnerTargetReactionWithTransform_Implementation(
	AActor* InstigatorActor,
	UAnimMontage* ReactionMontage,
	FSCP_HitTransformSettings TransformSettings,
	FSCP_HitDefenseSettings DefenseSettings)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return;
	}

	ApplyHitTransformEffects(InstigatorActor, GetOwner(), TransformSettings, DefenseSettings);
	if (ReactionMontage)
	{
		PlayReactionMontageOnActor(GetOwner(), ReactionMontage, 0.f, true);
	}
}

int32 USCP_CombatPredictionComponent::AllocatePredictionEventId()
{
	const int32 EventId = NextPredictionEventId;
	NextPredictionEventId = NextPredictionEventId >= MaxPredictionEventId
		? 0
		: NextPredictionEventId + 1;
	return EventId;
}

void USCP_CombatPredictionComponent::ResetPredictionEvents()
{
	NextPredictionEventId = 0;
}

FString USCP_CombatPredictionComponent::BuildActorDebugString(const AActor* Actor) const
{
	if (!Actor)
	{
		return TEXT("None");
	}

	const APawn* Pawn = Cast<APawn>(Actor);
	const APlayerState* PlayerState = Pawn ? Pawn->GetPlayerState() : nullptr;
	const FString PlayerIdString = PlayerState
		? FString::FromInt(PlayerState->GetPlayerId())
		: TEXT("None");

	const FString LocalString = Pawn
		? (Pawn->IsLocallyControlled() ? TEXT("true") : TEXT("false"))
		: TEXT("n/a");

	return FString::Printf(
		TEXT("Name=%s PlayerId=%s Local=%s Authority=%s NetMode=%d"),
		*Actor->GetName(),
		*PlayerIdString,
		*LocalString,
		Actor->HasAuthority() ? TEXT("true") : TEXT("false"),
		static_cast<int32>(Actor->GetNetMode()));
}

void USCP_CombatPredictionComponent::AddPendingPredictedReaction(
	const FSCP_CombatPredictionContext& Context,
	AActor* TargetActor)
{
	if (!TargetActor || !Context.IsValidForPrediction())
	{
		return;
	}

	PendingPredictedReactions.RemoveAll(
		[](const FSCP_PendingPredictedReaction& Entry)
		{
			return !Entry.TargetActor;
		});

	for (const FSCP_PendingPredictedReaction& Entry : PendingPredictedReactions)
	{
		if (DoesReactionKeyMatch(Entry, Context, TargetActor))
		{
			return;
		}
	}

	FSCP_PendingPredictedReaction Entry;
	Entry.TargetActor = TargetActor;
	Entry.AbilityPredictionKey = Context.AbilityPredictionKey;
	Entry.AbilitySpecHandle = Context.AbilitySpecHandle;
	Entry.PredictionEventId = Context.PredictionEventId;

	PendingPredictedReactions.Add(Entry);

	if (PendingPredictedReactions.Num() > MaxPendingReactions)
	{
		PendingPredictedReactions.RemoveAt(
			0,
			PendingPredictedReactions.Num() - MaxPendingReactions,
			EAllowShrinking::No);
	}
}

bool USCP_CombatPredictionComponent::ConsumePendingPredictedReaction(
	const FSCP_CombatPredictionContext& Context,
	AActor* TargetActor)
{
	if (!TargetActor || !Context.IsValidForPrediction())
	{
		return false;
	}

	for (int32 Index = 0; Index < PendingPredictedReactions.Num(); ++Index)
	{
		if (DoesReactionKeyMatch(PendingPredictedReactions[Index], Context, TargetActor))
		{
			PendingPredictedReactions.RemoveAtSwap(Index);
			return true;
		}
	}

	return false;
}

bool USCP_CombatPredictionComponent::DoesReactionKeyMatch(
	const FSCP_PendingPredictedReaction& Entry,
	const FSCP_CombatPredictionContext& Context,
	const AActor* TargetActor) const
{
	return Entry.TargetActor == TargetActor &&
		Entry.AbilityPredictionKey == Context.AbilityPredictionKey &&
		Entry.AbilitySpecHandle == Context.AbilitySpecHandle &&
		Entry.PredictionEventId == Context.PredictionEventId;
}

bool USCP_CombatPredictionComponent::PlayReactionMontageOnActor(
	AActor* TargetActor,
	UAnimMontage* ReactionMontage,
	float StartPosition,
	bool bForceRestart) const
{
	if (!TargetActor || !ReactionMontage)
	{
		return false;
	}

	USkeletalMeshComponent* MeshComponent =
		TargetActor->FindComponentByClass<USkeletalMeshComponent>();
	if (!MeshComponent)
	{
		return false;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!AnimInstance)
	{
		return false;
	}

	const bool bWasPlaying = AnimInstance->Montage_IsPlaying(ReactionMontage);
	if (bWasPlaying && !bForceRestart)
	{
		return true;
	}

	if (bWasPlaying && bForceRestart)
	{
		AnimInstance->Montage_Stop(0.f, ReactionMontage);
	}

	if (bLogReportedHits)
	{
		UE_LOG(
			LogSyncCombatPrediction,
			Log,
			TEXT("PlayReaction Target={%s} Montage=%s Start=%.3f ForceRestart=%s WasPlaying=%s"),
			*BuildActorDebugString(TargetActor),
			*GetNameSafe(ReactionMontage),
			StartPosition,
			bForceRestart ? TEXT("true") : TEXT("false"),
			bWasPlaying ? TEXT("true") : TEXT("false"));
	}

	return AnimInstance->Montage_Play(
		ReactionMontage,
		1.f,
		EMontagePlayReturnType::MontageLength,
		StartPosition) > 0.f;
}

void USCP_CombatPredictionComponent::ApplyGameplayEffectsFromHit(
	const FSCP_PredictedHit& Hit)
{
	if (!Hit.bIsAuthority || !Hit.TargetActor || !Hit.InstigatorActor)
	{
		return;
	}

	if (!Hit.GameplayEffects.HasAnyEffects())
	{
		return;
	}

	UAbilitySystemComponent* SourceASC =
		SyncCombatPrediction::GetAbilitySystemComponent(Hit.InstigatorActor);
	if (!SourceASC)
	{
		return;
	}

	const UGameplayAbility* AnimatingAbility = SourceASC->GetAnimatingAbility();
	const float SourceLevel = AnimatingAbility ? AnimatingAbility->GetAbilityLevel() : 1.f;

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	EffectContext.AddInstigator(Hit.InstigatorActor, GetOwner());
	EffectContext.AddHitResult(Hit.HitResult);
	EffectContext.AddOrigin(Hit.HitResult.ImpactPoint);

	ApplyEffectClassesToActor(
		Hit.InstigatorActor,
		Hit.GameplayEffects.ActivationSelfEffects,
		SourceLevel,
		EffectContext);

	const bool bShouldApplyCleanReaction = ShouldApplyCleanHitReaction(
		Hit.InstigatorActor,
		Hit.TargetActor,
		Hit.DefenseSettings);
	if (bShouldApplyCleanReaction)
	{
		ApplyEffectClassesToActor(
			Hit.TargetActor,
			Hit.GameplayEffects.TargetEffects,
			SourceLevel,
			EffectContext);
	}

	if (ShouldApplyDamageEffects(
		Hit.InstigatorActor,
		Hit.TargetActor,
		Hit.DefenseSettings,
		Hit.DamageDefenseSettings))
	{
		ApplyEffectClassesToActor(
			Hit.TargetActor,
			Hit.GameplayEffects.DamageEffects,
			SourceLevel,
			EffectContext,
			&Hit.GameplayEffects);
	}
}

void USCP_CombatPredictionComponent::ApplyReactionDataTargetEffectsFromHit(
	const FSCP_PredictedHit& Hit)
{
	if (!Hit.bIsAuthority || !Hit.TargetActor || !Hit.InstigatorActor || !ReactionData)
	{
		return;
	}

	if (!ShouldApplyCleanHitReaction(
		Hit.InstigatorActor,
		Hit.TargetActor,
		Hit.DefenseSettings))
	{
		return;
	}

	const FSCP_ReactionMontageEntry* Reaction = ReactionData->FindReaction(Hit.ReactionTag);
	if (!Reaction || Reaction->TargetEffects.IsEmpty())
	{
		return;
	}

	UAbilitySystemComponent* SourceASC =
		SyncCombatPrediction::GetAbilitySystemComponent(Hit.InstigatorActor);
	if (!SourceASC)
	{
		return;
	}

	const UGameplayAbility* AnimatingAbility = SourceASC->GetAnimatingAbility();
	const float SourceLevel = AnimatingAbility ? AnimatingAbility->GetAbilityLevel() : 1.f;

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	EffectContext.AddInstigator(Hit.InstigatorActor, GetOwner());
	EffectContext.AddHitResult(Hit.HitResult);
	EffectContext.AddOrigin(Hit.HitResult.ImpactPoint);

	ApplyEffectClassesToActor(
		Hit.TargetActor,
		Reaction->TargetEffects,
		SourceLevel,
		EffectContext);
}

void USCP_CombatPredictionComponent::ApplyEffectClassesToActor(
	AActor* TargetActor,
	const TArray<TSubclassOf<UGameplayEffect>>& EffectClasses,
	float Level,
	const FGameplayEffectContextHandle& EffectContext,
	const FSCP_GameplayEffectSettings* DamageSettings) const
{
	if (!TargetActor || EffectClasses.IsEmpty())
	{
		return;
	}

	UAbilitySystemComponent* SourceASC =
		SyncCombatPrediction::GetAbilitySystemComponent(GetOwner());
	UAbilitySystemComponent* TargetASC =
		SyncCombatPrediction::GetAbilitySystemComponent(TargetActor);
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectClasses)
	{
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectSpecHandle SpecHandle =
			SourceASC->MakeOutgoingSpec(EffectClass, Level, EffectContext);
		if (!SpecHandle.IsValid())
		{
			continue;
		}

		if (DamageSettings)
		{
			const FGameplayTag PhysicalDamageTag =
				SyncCombatPrediction::ResolveDamageTag(
					DamageSettings->PhysicalDamageSetByCallerTag,
					TEXT("Damage.Type.Physical"));
			const FGameplayTag MagicalDamageTag =
				SyncCombatPrediction::ResolveDamageTag(
					DamageSettings->MagicalDamageSetByCallerTag,
					TEXT("Damage.Type.Magical"));

			if (PhysicalDamageTag.IsValid())
			{
				UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
					SpecHandle,
					PhysicalDamageTag,
					DamageSettings->PhysicalAttackPercent);
			}

			if (MagicalDamageTag.IsValid())
			{
				UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
					SpecHandle,
					MagicalDamageTag,
					DamageSettings->MagicalAttackPercent);
			}
		}

		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void USCP_CombatPredictionComponent::ApplyHitTransformEffects(
	AActor* InstigatorActor,
	AActor* TargetActor,
	const FSCP_HitTransformSettings& TransformSettings,
	const FSCP_HitDefenseSettings& DefenseSettings) const
{
	if (!InstigatorActor || !TargetActor)
	{
		return;
	}

	ApplyHitRotation(InstigatorActor, TargetActor, TransformSettings.RotationSettings, DefenseSettings);
	ApplyHitMovement(InstigatorActor, TargetActor, TransformSettings.MovementSettings, DefenseSettings);
}

void USCP_CombatPredictionComponent::ApplyHitMovement(
	AActor* InstigatorActor,
	AActor* TargetActor,
	const FSCP_HitMovementSettings& MovementSettings,
	const FSCP_HitDefenseSettings& DefenseSettings) const
{
	if (MovementSettings.MoveDirection == ESCP_HitMoveDirection::None)
	{
		return;
	}

	if (MovementSettings.MoveDirection != ESCP_HitMoveDirection::KeepCurrentDistance
		&& MovementSettings.MoveDistance <= 0.f)
	{
		return;
	}

	if (!DoesTransformTimingMatch(MovementSettings.TriggerTiming, ESCP_HitTransformTriggerTiming::OnHit))
	{
		return;
	}

	const bool bWasBlocked = IsAttackBlocked(InstigatorActor, TargetActor, DefenseSettings);
	if (IsAttackDodged(TargetActor, DefenseSettings) || HasRequiredSuperArmor(TargetActor, DefenseSettings))
	{
		return;
	}

	if (bWasBlocked && !DefenseSettings.BlockSettings.bAllowMovementWhenBlocked)
	{
		return;
	}

	AActor* const ReferenceActor = ResolveTransformReferenceActor(
		MovementSettings.ReferenceActorSource,
		InstigatorActor,
		TargetActor);
	if (!ReferenceActor)
	{
		return;
	}

	auto ApplyToRecipient = [this, &MovementSettings, ReferenceActor](AActor* RecipientActor)
	{
		if (!RecipientActor || RecipientActor == ReferenceActor)
		{
			return;
		}

		ApplyMovementToActor(RecipientActor, ReferenceActor, MovementSettings);
	};

	switch (MovementSettings.Recipient)
	{
	case ESCP_HitTransformRecipient::Instigator:
		ApplyToRecipient(InstigatorActor);
		break;

	case ESCP_HitTransformRecipient::Target:
		ApplyToRecipient(TargetActor);
		break;

	case ESCP_HitTransformRecipient::Both:
		ApplyToRecipient(InstigatorActor);
		if (TargetActor != InstigatorActor)
		{
			ApplyToRecipient(TargetActor);
		}
		break;

	default:
		break;
	}
}

void USCP_CombatPredictionComponent::ApplyHitRotation(
	AActor* InstigatorActor,
	AActor* TargetActor,
	const FSCP_HitRotationSettings& RotationSettings,
	const FSCP_HitDefenseSettings& DefenseSettings) const
{
	if (RotationSettings.RotationDirection == ESCP_HitRotationDirection::None)
	{
		return;
	}

	if (!DoesTransformTimingMatch(RotationSettings.TriggerTiming, ESCP_HitTransformTriggerTiming::OnHit))
	{
		return;
	}

	const bool bWasBlocked = IsAttackBlocked(InstigatorActor, TargetActor, DefenseSettings);
	if (IsAttackDodged(TargetActor, DefenseSettings) || HasRequiredSuperArmor(TargetActor, DefenseSettings))
	{
		return;
	}

	if (bWasBlocked && !DefenseSettings.BlockSettings.bAllowRotationWhenBlocked)
	{
		return;
	}

	AActor* const ReferenceActor = ResolveTransformReferenceActor(
		RotationSettings.ReferenceActorSource,
		InstigatorActor,
		TargetActor);
	if (!ReferenceActor)
	{
		return;
	}

	auto ApplyToRecipient = [this, &RotationSettings, ReferenceActor](AActor* RecipientActor)
	{
		if (!RecipientActor || RecipientActor == ReferenceActor)
		{
			return;
		}

		ApplyRotationToActor(RecipientActor, ReferenceActor, RotationSettings);
	};

	switch (RotationSettings.Recipient)
	{
	case ESCP_HitTransformRecipient::Instigator:
		ApplyToRecipient(InstigatorActor);
		break;

	case ESCP_HitTransformRecipient::Target:
		ApplyToRecipient(TargetActor);
		break;

	case ESCP_HitTransformRecipient::Both:
		ApplyToRecipient(InstigatorActor);
		if (TargetActor != InstigatorActor)
		{
			ApplyToRecipient(TargetActor);
		}
		break;

	default:
		break;
	}
}

void USCP_CombatPredictionComponent::ApplyMovementToActor(
	AActor* RecipientActor,
	AActor* ReferenceActor,
	const FSCP_HitMovementSettings& MovementSettings) const
{
	FVector NewLocation;
	if (!CalculateMovementTargetLocation(RecipientActor, ReferenceActor, MovementSettings, NewLocation))
	{
		return;
	}

	RecipientActor->SetActorLocation(
		NewLocation,
		MovementSettings.bSweep,
		nullptr,
		SCPToTeleportType(MovementSettings.TeleportType));
}

void USCP_CombatPredictionComponent::ApplyRotationToActor(
	AActor* RecipientActor,
	AActor* ReferenceActor,
	const FSCP_HitRotationSettings& RotationSettings) const
{
	FRotator DesiredRotation;
	if (!CalculateRotationTargetRotation(RecipientActor, ReferenceActor, RotationSettings, DesiredRotation))
	{
		return;
	}

	RecipientActor->SetActorRotation(
		DesiredRotation,
		SCPToTeleportType(RotationSettings.TeleportType));
}

bool USCP_CombatPredictionComponent::CalculateMovementTargetLocation(
	AActor* RecipientActor,
	AActor* ReferenceActor,
	const FSCP_HitMovementSettings& MovementSettings,
	FVector& OutLocation) const
{
	if (!RecipientActor || !ReferenceActor)
	{
		return false;
	}

	const FVector ReferenceLocation = ReferenceActor->GetActorLocation();
	const FVector RecipientLocation = RecipientActor->GetActorLocation();

	FVector ReferenceForward = ReferenceActor->GetActorForwardVector();
	ReferenceForward.Z = 0.f;
	ReferenceForward = ReferenceForward.GetSafeNormal();
	if (ReferenceForward.IsNearlyZero())
	{
		return false;
	}

	FVector ReferenceRight = ReferenceActor->GetActorRightVector();
	ReferenceRight.Z = 0.f;
	ReferenceRight = ReferenceRight.GetSafeNormal();
	if (ReferenceRight.IsNearlyZero())
	{
		ReferenceRight = FVector::CrossProduct(FVector::UpVector, ReferenceForward).GetSafeNormal();
	}

	const FVector RelativeLocation = RecipientLocation - ReferenceLocation;
	const float CurrentForwardProjection = FVector::DotProduct(RelativeLocation, ReferenceForward);
	const float CurrentLateralProjection = FVector::DotProduct(RelativeLocation, ReferenceRight);

	float TargetForwardProjection = CurrentForwardProjection;
	float TargetLateralProjection = CurrentLateralProjection;

	switch (MovementSettings.MoveDirection)
	{
	case ESCP_HitMoveDirection::KeepCurrentDistance:
		break;

	case ESCP_HitMoveDirection::MoveCloser:
		TargetForwardProjection -= MovementSettings.MoveDistance;
		break;

	case ESCP_HitMoveDirection::MoveAway:
		TargetForwardProjection += MovementSettings.MoveDistance;
		break;

	case ESCP_HitMoveDirection::SnapToDistance:
		TargetForwardProjection = MovementSettings.MoveDistance;
		break;

	case ESCP_HitMoveDirection::None:
	default:
		return false;
	}

	switch (MovementSettings.LateralOffsetMode)
	{
	case ESCP_HitLateralOffsetMode::KeepCurrent:
		break;

	case ESCP_HitLateralOffsetMode::AddOffset:
		TargetLateralProjection += MovementSettings.LateralOffset;
		break;

	case ESCP_HitLateralOffsetMode::SnapToOffset:
		TargetLateralProjection = MovementSettings.LateralOffset;
		break;

	default:
		break;
	}

	if (FMath::IsNearlyEqual(TargetForwardProjection, CurrentForwardProjection)
		&& FMath::IsNearlyEqual(TargetLateralProjection, CurrentLateralProjection))
	{
		return false;
	}

	FVector NewLocation = ReferenceLocation
		+ (ReferenceForward * TargetForwardProjection)
		+ (ReferenceRight * TargetLateralProjection);
	NewLocation.Z = RecipientLocation.Z;

	OutLocation = NewLocation;
	return true;
}

bool USCP_CombatPredictionComponent::CalculateRotationTargetRotation(
	AActor* RecipientActor,
	AActor* ReferenceActor,
	const FSCP_HitRotationSettings& RotationSettings,
	FRotator& OutRotation) const
{
	if (!RecipientActor || !ReferenceActor)
	{
		return false;
	}

	const FVector ReferenceLocation = ReferenceActor->GetActorLocation();
	const FVector RecipientLocation = RecipientActor->GetActorLocation();
	FRotator DesiredRotation = RecipientActor->GetActorRotation();

	switch (RotationSettings.RotationDirection)
	{
	case ESCP_HitRotationDirection::FaceReferenceActor:
		{
			FVector ToReference = ReferenceLocation - RecipientLocation;
			ToReference.Z = 0.f;
			if (const FVector FacingDirection = ToReference.GetSafeNormal(); !FacingDirection.IsNearlyZero())
			{
				DesiredRotation = FacingDirection.Rotation();
			}
			break;
		}

	case ESCP_HitRotationDirection::FaceAwayFromReference:
		{
			FVector AwayFromReference = RecipientLocation - ReferenceLocation;
			AwayFromReference.Z = 0.f;
			if (const FVector FacingDirection = AwayFromReference.GetSafeNormal(); !FacingDirection.IsNearlyZero())
			{
				DesiredRotation = FacingDirection.Rotation();
			}
			break;
		}

	case ESCP_HitRotationDirection::FaceOppositeReferenceForward:
		{
			FVector OppositeDirection = -ReferenceActor->GetActorForwardVector();
			OppositeDirection.Z = 0.f;
			if (const FVector FacingDirection = OppositeDirection.GetSafeNormal(); !FacingDirection.IsNearlyZero())
			{
				DesiredRotation = FacingDirection.Rotation();
			}
			break;
		}

	case ESCP_HitRotationDirection::FaceDirection:
		DesiredRotation = RotationSettings.DirectionToFace;
		break;

	case ESCP_HitRotationDirection::None:
	default:
		return false;
	}

	OutRotation = DesiredRotation;
	return true;
}

bool USCP_CombatPredictionComponent::DoesTransformTimingMatch(
	ESCP_HitTransformTriggerTiming ConfiguredTiming,
	ESCP_HitTransformTriggerTiming InvocationTiming)
{
	return ConfiguredTiming == ESCP_HitTransformTriggerTiming::Both
		|| ConfiguredTiming == InvocationTiming;
}

bool USCP_CombatPredictionComponent::TransformRecipientIncludesTarget(
	ESCP_HitTransformRecipient Recipient)
{
	return Recipient == ESCP_HitTransformRecipient::Target
		|| Recipient == ESCP_HitTransformRecipient::Both;
}

AActor* USCP_CombatPredictionComponent::ResolveTransformReferenceActor(
	ESCP_HitTransformReferenceActorSource ReferenceSource,
	AActor* InstigatorActor,
	AActor* TargetActor)
{
	switch (ReferenceSource)
	{
	case ESCP_HitTransformReferenceActorSource::Instigator:
		return InstigatorActor;

	case ESCP_HitTransformReferenceActorSource::Target:
		return TargetActor;

	default:
		return nullptr;
	}
}

bool USCP_CombatPredictionComponent::IsAttackBlocked(
	AActor* InstigatorActor,
	AActor* TargetActor,
	const FSCP_HitDefenseSettings& DefenseSettings) const
{
	if (!DefenseSettings.BlockSettings.bBlockable || !InstigatorActor || !TargetActor)
	{
		return false;
	}

	if (!BlockingTag.IsValid())
	{
		return false;
	}

	const UAbilitySystemComponent* TargetASC =
		SyncCombatPrediction::GetAbilitySystemComponent(TargetActor);
	if (!TargetASC || !TargetASC->HasMatchingGameplayTag(BlockingTag))
	{
		return false;
	}

	return IsWithinBlockAngle(
		TargetActor,
		InstigatorActor,
		DefenseSettings.BlockSettings.BlockAngleDegrees);
}

bool USCP_CombatPredictionComponent::IsAttackDodged(
	AActor* TargetActor,
	const FSCP_HitDefenseSettings& DefenseSettings) const
{
	if (!DefenseSettings.DodgeSettings.bDodgeable || !TargetActor)
	{
		return false;
	}

	if (!DodgingTag.IsValid())
	{
		return false;
	}

	const UAbilitySystemComponent* TargetASC =
		SyncCombatPrediction::GetAbilitySystemComponent(TargetActor);
	return TargetASC && TargetASC->HasMatchingGameplayTag(DodgingTag);
}

bool USCP_CombatPredictionComponent::HasRequiredSuperArmor(
	AActor* TargetActor,
	const FSCP_HitDefenseSettings& DefenseSettings) const
{
	if (!TargetActor || DefenseSettings.RequiredSuperArmor == ESCP_HitSuperArmorLevel::None)
	{
		return false;
	}

	return GetTargetSuperArmorLevel(TargetActor) >= DefenseSettings.RequiredSuperArmor;
}

ESCP_HitSuperArmorLevel USCP_CombatPredictionComponent::GetTargetSuperArmorLevel(AActor* TargetActor) const
{
	const UAbilitySystemComponent* TargetASC =
		SyncCombatPrediction::GetAbilitySystemComponent(TargetActor);
	if (!TargetASC)
	{
		return ESCP_HitSuperArmorLevel::None;
	}

	if (SuperArmorTag3.IsValid() && TargetASC->HasMatchingGameplayTag(SuperArmorTag3))
	{
		return ESCP_HitSuperArmorLevel::SuperArmor3;
	}

	if (SuperArmorTag2.IsValid() && TargetASC->HasMatchingGameplayTag(SuperArmorTag2))
	{
		return ESCP_HitSuperArmorLevel::SuperArmor2;
	}

	if (SuperArmorTag1.IsValid() && TargetASC->HasMatchingGameplayTag(SuperArmorTag1))
	{
		return ESCP_HitSuperArmorLevel::SuperArmor1;
	}

	return ESCP_HitSuperArmorLevel::None;
}

bool USCP_CombatPredictionComponent::ShouldApplyCleanHitReaction(
	AActor* InstigatorActor,
	AActor* TargetActor,
	const FSCP_HitDefenseSettings& DefenseSettings) const
{
	return !IsAttackBlocked(InstigatorActor, TargetActor, DefenseSettings)
		&& !IsAttackDodged(TargetActor, DefenseSettings)
		&& !HasRequiredSuperArmor(TargetActor, DefenseSettings);
}

bool USCP_CombatPredictionComponent::ShouldApplyDamageEffects(
	AActor* InstigatorActor,
	AActor* TargetActor,
	const FSCP_HitDefenseSettings& DefenseSettings,
	const FSCP_HitDamageDefenseSettings& DamageDefenseSettings) const
{
	const bool bWasBlocked = IsAttackBlocked(InstigatorActor, TargetActor, DefenseSettings);
	const bool bWasDodged = IsAttackDodged(TargetActor, DefenseSettings);
	const ESCP_HitSuperArmorLevel TargetSuperArmorLevel = GetTargetSuperArmorLevel(TargetActor);

	if (bWasBlocked && !DamageDefenseSettings.bApplyDamageWhenBlocked)
	{
		return false;
	}

	if (bWasDodged && !DamageDefenseSettings.bApplyDamageWhenDodged)
	{
		return false;
	}

	if (TargetSuperArmorLevel > DamageDefenseSettings.MaxSuperArmorLevelThatTakesDamage)
	{
		return false;
	}

	return true;
}

bool USCP_CombatPredictionComponent::IsWithinBlockAngle(
	const AActor* DefenderActor,
	const AActor* AttackerActor,
	float BlockAngleDegrees)
{
	if (!DefenderActor || !AttackerActor)
	{
		return false;
	}

	const FVector ToAttacker =
		(AttackerActor->GetActorLocation() - DefenderActor->GetActorLocation()).GetSafeNormal();
	const FVector DefenderForward = DefenderActor->GetActorForwardVector().GetSafeNormal();
	const float Dot = FVector::DotProduct(DefenderForward, ToAttacker);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
	return AngleDegrees < BlockAngleDegrees;
}

void USCP_CombatPredictionComponent::BeginPredictedTargetReaction(float Duration)
{
	if (GetOwnerRole() != ROLE_SimulatedProxy)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	++LocalPredictedTargetReactionCount;

	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			EndPredictedTargetReaction();
		}),
		FMath::Max(Duration, 0.01f),
		false);
}

void USCP_CombatPredictionComponent::EndPredictedTargetReaction()
{
	if (LocalPredictedTargetReactionCount <= 0)
	{
		return;
	}

	--LocalPredictedTargetReactionCount;
}

void USCP_CombatPredictionComponent::BeginTargetReactionMovementTolerance(float Duration)
{
	BeginMovementCorrectionTolerance(Duration, false);
}

void USCP_CombatPredictionComponent::BeginMovementCorrectionTolerance(
	float Duration,
	bool bAcceptClientAuthoritativePosition)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !World)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent =
		OwnerActor->FindComponentByClass<UCharacterMovementComponent>();
	if (!MovementComponent)
	{
		return;
	}

	if (MovementCorrectionToleranceCount == 0)
	{
		bSavedIgnoreClientMovementErrorChecksAndCorrection =
			MovementComponent->bIgnoreClientMovementErrorChecksAndCorrection;
		bSavedServerAcceptClientAuthoritativePosition =
			MovementComponent->bServerAcceptClientAuthoritativePosition;
	}

	++MovementCorrectionToleranceCount;
	if (bAcceptClientAuthoritativePosition)
	{
		++MovementCorrectionAcceptClientAuthoritativeCount;
	}

	MovementComponent->bIgnoreClientMovementErrorChecksAndCorrection = true;
	MovementComponent->bServerAcceptClientAuthoritativePosition =
		MovementCorrectionAcceptClientAuthoritativeCount > 0;

	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, bAcceptClientAuthoritativePosition]()
		{
			EndMovementCorrectionTolerance(bAcceptClientAuthoritativePosition);
		}),
		FMath::Max(Duration, 0.01f),
		false);
}

void USCP_CombatPredictionComponent::EndMovementCorrectionTolerance(
	bool bAcceptClientAuthoritativePosition)
{
	if (MovementCorrectionToleranceCount <= 0)
	{
		return;
	}

	--MovementCorrectionToleranceCount;
	if (bAcceptClientAuthoritativePosition && MovementCorrectionAcceptClientAuthoritativeCount > 0)
	{
		--MovementCorrectionAcceptClientAuthoritativeCount;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent =
		OwnerActor->FindComponentByClass<UCharacterMovementComponent>();
	if (!MovementComponent)
	{
		return;
	}

	if (MovementCorrectionToleranceCount > 0)
	{
		MovementComponent->bIgnoreClientMovementErrorChecksAndCorrection = true;
		MovementComponent->bServerAcceptClientAuthoritativePosition =
			MovementCorrectionAcceptClientAuthoritativeCount > 0;
		return;
	}

	MovementCorrectionAcceptClientAuthoritativeCount = 0;

	MovementComponent->bIgnoreClientMovementErrorChecksAndCorrection =
		bSavedIgnoreClientMovementErrorChecksAndCorrection;
	MovementComponent->bServerAcceptClientAuthoritativePosition =
		bSavedServerAcceptClientAuthoritativePosition;

	OwnerActor->ForceNetUpdate();
}
