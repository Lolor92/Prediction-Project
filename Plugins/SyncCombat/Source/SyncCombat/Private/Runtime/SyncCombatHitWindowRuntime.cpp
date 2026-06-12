// Copyright ProjectLogos

#include "Runtime/SyncCombatHitWindowRuntime.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SyncCombatComponent.h"
#include "Utilities/SyncCombatFunctionLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Tag/SyncCombatTags.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPLCombatHitDetectionRuntime, Log, All);

namespace
{
constexpr int32 PLMaxScopedHitWindowCacheEntries = 256;

bool PLSyncCombatTransformRecipientIncludesTarget(const ESyncCombatHitWindowTransformRecipient Recipient)
{
	return Recipient == ESyncCombatHitWindowTransformRecipient::Target
		|| Recipient == ESyncCombatHitWindowTransformRecipient::Both;
}

FString SyncCombatHitWindowEffectListToString(const TArray<FSyncCombatHitWindowGameplayEffect>& GameplayEffects)
{
	if (GameplayEffects.IsEmpty())
	{
		return TEXT("None");
	}

	TArray<FString> EffectNames;
	EffectNames.Reserve(GameplayEffects.Num());

	for (const FSyncCombatHitWindowGameplayEffect& Effect : GameplayEffects)
	{
		EffectNames.Add(FString::Printf(
			TEXT("%s@%.2f"),
			*GetNameSafe(Effect.GameplayEffectClass),
			Effect.EffectLevel));
	}

	return FString::Join(EffectNames, TEXT(", "));
}

FString PLGameplayTagsToString(const FGameplayTagContainer& Tags)
{
	return Tags.IsEmpty() ? TEXT("None") : Tags.ToStringSimple();
}

FString PLGetOwnedTagsString(const UAbilitySystemComponent* AbilitySystemComponent)
{
	FGameplayTagContainer OwnedTags;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->GetOwnedGameplayTags(OwnedTags);
	}

	return PLGameplayTagsToString(OwnedTags);
}

void PLGetActiveMontageDebugData(const USkeletalMeshComponent* MeshComp, const UAnimMontage*& OutActiveMontage,
	float& OutActiveMontagePosition)
{
	OutActiveMontage = nullptr;
	OutActiveMontagePosition = -1.f;

	const UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	OutActiveMontage = AnimInstance->GetCurrentActiveMontage();
	if (OutActiveMontage)
	{
		OutActiveMontagePosition = AnimInstance->Montage_GetPosition(OutActiveMontage);
	}
}

}

FSyncCombatHitWindowRuntime::FSyncCombatHitWindowRuntime(USyncCombatComponent& InCombatComponent)
	: CombatComponent(InCombatComponent)
{
}

void FSyncCombatHitWindowRuntime::Deinitialize()
{
	ActiveHitDetectionWindows.Reset();
	ActiveHitDetectionWindowCounts.Reset();
	ScopedHitActors.Reset();
	LastCombatReferenceActor.Reset();
	ResetActiveHitDebugWindow();
}

void FSyncCombatHitWindowRuntime::Tick()
{
	if (!bHitDebugWindowActive) return;
	DebugSweepActiveHitWindow();
}

void FSyncCombatHitWindowRuntime::SetLastCombatReferenceActor(AActor* InActor)
{
	LastCombatReferenceActor = InActor && InActor != CombatComponent.GetOwner() ? InActor : nullptr;
}

bool FSyncCombatHitWindowRuntime::IsWithinBlockAngle(const AActor* DefenderActor, const AActor* AttackerActor,
	const float BlockAngleDegrees)
{
	if (!DefenderActor || !AttackerActor) return false;

	const FVector ToAttacker = (AttackerActor->GetActorLocation() - DefenderActor->GetActorLocation()).GetSafeNormal();
	const FVector DefenderForward = DefenderActor->GetActorForwardVector().GetSafeNormal();
	const float Dot = FVector::DotProduct(DefenderForward, ToAttacker);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
	return AngleDegrees < BlockAngleDegrees;
}

bool FSyncCombatHitWindowRuntime::DoesTransformTimingMatch(const ESyncCombatHitWindowTransformTriggerTiming ConfiguredTiming,
	const ESyncCombatHitWindowTransformTriggerTiming InvocationTiming)
{
	return ConfiguredTiming == ESyncCombatHitWindowTransformTriggerTiming::Both || ConfiguredTiming == InvocationTiming;
}

USyncCombatComponent* FSyncCombatHitWindowRuntime::FindCombatComponent(AActor* Actor)
{
	if (!Actor) return nullptr;

	return Actor->FindComponentByClass<USyncCombatComponent>();
}

bool FSyncCombatHitWindowRuntime::BuildScopedHitWindowKey(UAnimSequenceBase* Animation,
	const FAnimNotifyEvent* NotifyEvent, FSyncCombatScopedHitWindowKey& OutKey) const
{
	if (!CombatComponent.AbilitySystemComponent
		|| !CombatComponent.GetOwner()
		|| !Animation
		|| !NotifyEvent)
	{
		return false;
	}

	const UGameplayAbility* AnimatingAbility = CombatComponent.AbilitySystemComponent->GetAnimatingAbility();
	if (!AnimatingAbility)
	{
		return false;
	}

	const FPredictionKey& PredictionKey =
		AnimatingAbility->GetCurrentActivationInfo().GetActivationPredictionKey();

	OutKey.SourceActorKey = FObjectKey(CombatComponent.GetOwner());
	OutKey.AbilityKey = FObjectKey(AnimatingAbility);
	OutKey.AnimationKey = FObjectKey(Animation);
	OutKey.NotifyEvent = NotifyEvent;
	OutKey.ActivationPredictionKey = PredictionKey.Current;
	OutKey.ActivationPredictionBase = PredictionKey.Base;
	return true;
}

void FSyncCombatHitWindowRuntime::RunHitDebugQuery(const FTransform& StartTransform, const FTransform& EndTransform,
	const bool bDrawDebug)
{
	UWorld* World = CombatComponent.GetWorld();
	if (!World) return;

	const FVector StartLocation = StartTransform.GetLocation();
	const FVector EndLocation = EndTransform.GetLocation();
	const FQuat StartRotation = StartTransform.GetRotation();
	const FQuat EndRotation = EndTransform.GetRotation();
	const FQuat SweepRotation = EndTransform.GetRotation();

	const FSyncCombatHitWindowShapeSettings& ShapeSettings = ActiveHitWindowSettings.ShapeSettings;
	const float SphereRadius = FMath::Max(0.f, ShapeSettings.SphereRadius);
	const float CapsuleRadius = FMath::Max(0.f, ShapeSettings.CapsuleRadius);
	const float CapsuleHalfHeight = FMath::Max(CapsuleRadius, ShapeSettings.CapsuleHalfHeight);
	const FVector BoxHalfExtent = ShapeSettings.BoxHalfExtent.ComponentMax(FVector::ZeroVector);

	FCollisionShape CollisionShape;
	switch (ShapeSettings.ShapeType)
	{
	case ESyncCombatHitDetectionShapeType::Capsule:
		CollisionShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
		break;

	case ESyncCombatHitDetectionShapeType::Box:
		CollisionShape = FCollisionShape::MakeBox(BoxHalfExtent);
		break;

	case ESyncCombatHitDetectionShapeType::Sphere:
	default:
		CollisionShape = FCollisionShape::MakeSphere(SphereRadius);
		break;
	}

	if (bDrawDebug)
	{
		switch (ShapeSettings.ShapeType)
		{
		case ESyncCombatHitDetectionShapeType::Capsule:
			DrawDebugCapsule(World, EndLocation, CapsuleHalfHeight, CapsuleRadius, SweepRotation,
				FColor::Blue, false, 0.1f, 0, 1.5f);
			break;

		case ESyncCombatHitDetectionShapeType::Box:
			DrawDebugBox(World, EndLocation, BoxHalfExtent, SweepRotation, FColor::Blue,
				false, 0.1f, 0, 1.5f);
			break;

		case ESyncCombatHitDetectionShapeType::Sphere:
		default:
			DrawDebugSphere(World, EndLocation, SphereRadius, 12, FColor::Blue,
				false, 0.1f, 0, 1.5f);
			break;
		}

		DrawDebugLine(World, StartLocation, EndLocation, FColor::Cyan, false, 0.1f, 0, 1.0f);

		DrawDebugDirectionalArrow(World, EndLocation, EndLocation + (EndTransform.GetUnitAxis(EAxis::X) * 30.f),
			12.f, FColor::Green, false, 0.1f, 0, 1.0f);
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SyncCombatHitDebugSweep), false, CombatComponent.GetOwner());
	QueryParams.AddIgnoredActor(CombatComponent.GetOwner());

	const double RotationDeltaDegrees = FMath::RadiansToDegrees(StartRotation.AngularDistance(EndRotation));
	const double LargestShapeExtent = FMath::Max(
		FMath::Max(static_cast<double>(SphereRadius), static_cast<double>(CapsuleHalfHeight)),
		BoxHalfExtent.GetMax());

	const double DistanceStepSize = FMath::Max(20.0, LargestShapeExtent * 0.75);
	const int32 DistanceSteps = FMath::Clamp(
		FMath::CeilToInt(FVector::Distance(StartLocation, EndLocation) / DistanceStepSize),
		1,
		6);

	const int32 RotationSteps = FMath::Clamp(
		FMath::CeilToInt(RotationDeltaDegrees / 15.f),
		1,
		6);

	const int32 NumSubsteps = FMath::Clamp(FMath::Max(DistanceSteps, RotationSteps), 1, 6);

	for (int32 StepIndex = 1; StepIndex <= NumSubsteps; ++StepIndex)
	{
		const float PrevAlpha = static_cast<float>(StepIndex - 1) / static_cast<float>(NumSubsteps);
		const float CurrAlpha = static_cast<float>(StepIndex) / static_cast<float>(NumSubsteps);

		const FVector SegmentStartLocation = FMath::Lerp(StartLocation, EndLocation, PrevAlpha);
		const FVector SegmentEndLocation = FMath::Lerp(StartLocation, EndLocation, CurrAlpha);
		const FQuat SegmentRotation = FQuat::Slerp(StartRotation, EndRotation, CurrAlpha).GetNormalized();

		TArray<FHitResult> HitResults;
		World->SweepMultiByObjectType(HitResults, SegmentStartLocation, SegmentEndLocation, SegmentRotation,
			ObjectQueryParams, CollisionShape, QueryParams);

		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor || HitActor == CombatComponent.GetOwner()) continue;

			const FObjectKey HitActorKey(HitActor);
			TSet<FObjectKey>* ScopedHitActorSet = bActiveScopedHitWindowKeyValid
				? &ScopedHitActors.FindOrAdd(ActiveScopedHitWindowKey)
				: nullptr;
			const bool bAlreadyHit = ScopedHitActorSet
				? ScopedHitActorSet->Contains(HitActorKey)
				: HitActorsThisWindow.Contains(HitActorKey);

			if (bAlreadyHit)
			{
				if (CombatComponent.GetOwner() && CombatComponent.GetOwner()->HasAuthority())
				{
					UE_LOG(LogPLCombatHitDetectionRuntime, VeryVerbose,
						TEXT("HitWindowDuplicateSkippedV4 Source=%s Target=%s Step=%d/%d Depth=%d HitActors=%d ScopedHitActors=%d Scoped=%d"),
						*GetNameSafe(CombatComponent.GetOwner()),
						*GetNameSafe(HitActor),
						StepIndex,
						NumSubsteps,
						ActiveHitDebugWindowDepth,
						HitActorsThisWindow.Num(),
						ScopedHitActorSet ? ScopedHitActorSet->Num() : 0,
						bActiveScopedHitWindowKeyValid);
				}
				continue;
			}

			if (CombatComponent.GetOwner() && CombatComponent.GetOwner()->HasAuthority())
			{
				UE_LOG(LogPLCombatHitDetectionRuntime, Display,
					TEXT("HitWindowHitAccepted Source=%s Target=%s Step=%d/%d Depth=%d HitActorsBefore=%d Impact=%s"),
					*GetNameSafe(CombatComponent.GetOwner()),
					*GetNameSafe(HitActor),
					StepIndex,
					NumSubsteps,
					ActiveHitDebugWindowDepth,
					HitActorsThisWindow.Num(),
					*Hit.ImpactPoint.ToCompactString());
			}

			HitActorsThisWindow.Add(HitActorKey);
			if (ScopedHitActorSet)
			{
				ScopedHitActorSet->Add(HitActorKey);

				if (ScopedHitActors.Num() > PLMaxScopedHitWindowCacheEntries)
				{
					for (auto It = ScopedHitActors.CreateIterator(); It; ++It)
					{
						if (!(It.Key() == ActiveScopedHitWindowKey))
						{
							It.RemoveCurrent();
							break;
						}
					}
				}
			}
			TryApplyHitGameplayEffects(HitActor, Hit);
		}
	}
}

void FSyncCombatHitWindowRuntime::DebugSweepActiveHitWindow()
{
	if (!CombatComponent.GetOwner() || !ActiveHitDebugMesh)
	{
		ResetActiveHitDebugWindow();
		return;
	}

	const FTransform CurrentTransform = GetHitTraceWorldTransform(
		ActiveHitDebugMesh,
		ActiveHitDebugSocketName,
		ActiveHitWindowSettings.ShapeSettings);

	if (!bHasPreviousHitDebugLocation)
	{
		PreviousHitDebugTransform = CurrentTransform;
		bHasPreviousHitDebugLocation = true;
	}

	RunHitDebugQuery(
		PreviousHitDebugTransform,
		CurrentTransform,
		ActiveHitWindowSettings.DebugSettings.bDrawDebugTrace);
	PreviousHitDebugTransform = CurrentTransform;
}

void FSyncCombatHitWindowRuntime::ResetActiveHitDebugWindow()
{
	ActiveHitDebugMesh = nullptr;
	ActiveHitDebugSocketName = NAME_None;
	PreviousHitDebugTransform = FTransform::Identity;
	bHitDebugWindowActive = false;
	bHasPreviousHitDebugLocation = false;
	HitActorsThisWindow.Reset();
	bActiveScopedHitWindowKeyValid = false;
	ActiveHitDebugWindowDepth = 0;
	ActiveHitWindowSettings = FSyncCombatHitWindowSettings();

	CombatComponent.SetComponentTickEnabled(false);
}

FTransform FSyncCombatHitWindowRuntime::GetHitTraceWorldTransform(USkeletalMeshComponent* MeshComp, const FName SocketName,
	const FSyncCombatHitWindowShapeSettings& HitShapeSettings) const
{
	if (!MeshComp) return FTransform::Identity;

	const FTransform BaseTransform =
		(!SocketName.IsNone() && MeshComp->DoesSocketExist(SocketName))
			? MeshComp->GetSocketTransform(SocketName)
			: MeshComp->GetComponentTransform();

	return FTransform(
		BaseTransform.TransformRotation(HitShapeSettings.LocalRotation.Quaternion()),
		BaseTransform.TransformPosition(HitShapeSettings.LocalOffset),
		FVector::OneVector);
}

void FSyncCombatHitWindowRuntime::TryApplyHitGameplayEffects(AActor* HitActor, const FHitResult& HitResult)
{
	if (!CombatComponent.AbilitySystemComponent || !HitActor || HitActor == CombatComponent.GetOwner())
	{
		return;
	}

	const bool bIsAuthority = CombatComponent.GetOwner() && CombatComponent.GetOwner()->HasAuthority();

	if (!bIsAuthority)
	{
		return;
	}

	const bool bWasBlocked = IsAttackBlocked(HitActor);
	const bool bWasParried = bWasBlocked && IsAttackParried(HitActor);
	const bool bWasDodged = IsAttackDodged(HitActor);
	const bool bHasSuperArmor = HasRequiredSuperArmor(HitActor);
	const ESyncCombatHitWindowSuperArmorLevel TargetSuperArmorLevel = GetTargetSuperArmorLevel(HitActor);

	const bool bHasDefenseOutcome = bWasBlocked || bWasParried || bWasDodged || bHasSuperArmor;
	const bool bWillApplyDamage = ShouldApplyDamageGameplayEffects(
		bWasBlocked,
		bWasParried,
		bWasDodged,
		TargetSuperArmorLevel);
	const bool bWillApplyCleanHitReactions = !bHasDefenseOutcome;
	TMap<FGameplayTag, int32> PreviousReactionTagCounts;
	CaptureReactionTagCounts(HitActor, PreviousReactionTagCounts);

	UE_LOG(LogPLCombatHitDetectionRuntime, VeryVerbose,
		TEXT("HitWindowResult Source=%s Target=%s Depth=%d HitActors=%d Blocked=%d Parried=%d Dodged=%d SuperArmor=%d TargetSuperArmorLevel=%d WillApplyDamage=%d WillApplyCleanHitReactions=%d ReactionEffects=[%s] DamageEffects=[%s]"),
		*GetNameSafe(CombatComponent.GetOwner()),
		*GetNameSafe(HitActor),
		ActiveHitDebugWindowDepth,
		HitActorsThisWindow.Num(),
		bWasBlocked,
		bWasParried,
		bWasDodged,
		bHasSuperArmor,
		static_cast<int32>(TargetSuperArmorLevel),
		bWillApplyDamage,
		bWillApplyCleanHitReactions,
		*SyncCombatHitWindowEffectListToString(ActiveHitWindowSettings.GameplayEffectsToApply),
		*SyncCombatHitWindowEffectListToString(ActiveHitWindowSettings.DamageSettings.DamageGameplayEffectsToApply));

	ApplyHitWindowTransformEffects(HitActor, bWasBlocked, bWasDodged, bHasSuperArmor);

	if (bWillApplyDamage)
	{
		ApplyHitWindowGameplayEffectListToTarget(
			HitActor,
			HitResult,
			ActiveHitWindowSettings.DamageSettings.DamageGameplayEffectsToApply
		);
	}

	if (bHasDefenseOutcome)
	{
		if (bWasParried)
		{
			LastCombatReferenceActor = HitActor;

			if (USyncCombatComponent* TargetCombatComponent = FindCombatComponent(HitActor))
			{
				TargetCombatComponent->SetLastCombatReferenceActor(CombatComponent.GetOwner());
			}
		}

		ApplyDefenseGameplayEffects(
			HitActor,
			HitResult,
			bWasBlocked,
			bWasParried,
			bWasDodged,
			bHasSuperArmor
		);

		SendHitReactionVisualPrediction(
			HitActor,
			bWasBlocked,
			bWasDodged,
			bHasSuperArmor,
			FGameplayTagContainer(),
			FGameplayTagContainer());

		return;
	}

	// Reaction effects only happen when the hit actually lands cleanly.
	// Put stagger/knockdown/pushback trigger effects here, not damage.
	ApplyHitWindowGameplayEffectListToTarget(
		HitActor,
		HitResult,
		ActiveHitWindowSettings.GameplayEffectsToApply
	);

	FGameplayTagContainer ReactionTriggerTags;
	FGameplayTagContainer ReactionAbilityTags;
	BuildReactionPredictionTags(HitActor, PreviousReactionTagCounts, ReactionTriggerTags, ReactionAbilityTags);
	SendHitReactionVisualPrediction(
		HitActor,
		bWasBlocked,
		bWasDodged,
		bHasSuperArmor,
		ReactionTriggerTags,
		ReactionAbilityTags);

	// Delayed reaction effects also only happen on a clean hit.
	// Block / parry / dodge / super armor prevent these because the function returned above.
	ScheduleDelayedHitWindowGameplayEffects(
		HitActor,
		HitResult,
		ActiveHitWindowSettings.DelayedGameplayEffectsToApply
	);

	ExecuteHitWindowGameplayCues(HitActor, &HitResult, ESyncCombatHitWindowCueTriggerTiming::OnHit);
}

void FSyncCombatHitWindowRuntime::ApplyHitWindowGameplayEffectListToTarget(
	AActor* HitActor,
	const FHitResult& HitResult,
	const TArray<FSyncCombatHitWindowGameplayEffect>& GameplayEffects
) const
{
	if (!CombatComponent.AbilitySystemComponent || !HitActor || HitActor == CombatComponent.GetOwner())
	{
		return;
	}

	if (GameplayEffects.IsEmpty())
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = USyncCombatFunctionLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = CombatComponent.AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(&CombatComponent);
	ContextHandle.AddHitResult(HitResult);

	if (AActor* OwnerActor = CombatComponent.GetOwner())
	{
		ContextHandle.AddInstigator(OwnerActor, OwnerActor);
	}

	for (const FSyncCombatHitWindowGameplayEffect& GameplayEffectToApply : GameplayEffects)
	{
		if (!GameplayEffectToApply.GameplayEffectClass)
		{
			continue;
		}

		const FGameplayEffectSpecHandle SpecHandle = CombatComponent.AbilitySystemComponent->MakeOutgoingSpec(
			GameplayEffectToApply.GameplayEffectClass,
			GameplayEffectToApply.EffectLevel,
			ContextHandle
		);

		if (!SpecHandle.IsValid())
		{
			UE_LOG(LogPLCombatHitDetectionRuntime, Warning,
				TEXT("ApplyHitWindowGESpecInvalid Source=%s Target=%s Effect=%s Level=%.2f"),
				*GetNameSafe(CombatComponent.GetOwner()),
				*GetNameSafe(HitActor),
				*GetNameSafe(GameplayEffectToApply.GameplayEffectClass),
				GameplayEffectToApply.EffectLevel);
			continue;
		}

		UE_LOG(LogPLCombatHitDetectionRuntime, Display,
			TEXT("ApplyHitWindowGEStart Source=%s Target=%s Effect=%s Level=%.2f TargetTagsBefore=[%s] Depth=%d HitActors=%d"),
			*GetNameSafe(CombatComponent.GetOwner()),
			*GetNameSafe(HitActor),
			*GetNameSafe(GameplayEffectToApply.GameplayEffectClass),
			GameplayEffectToApply.EffectLevel,
			*PLGetOwnedTagsString(TargetASC),
			ActiveHitDebugWindowDepth,
			HitActorsThisWindow.Num());

		const FActiveGameplayEffectHandle AppliedHandle = CombatComponent.AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(
			*SpecHandle.Data.Get(),
			TargetASC
		);

		UE_LOG(LogPLCombatHitDetectionRuntime, VeryVerbose,
			TEXT("ApplyHitWindowGEEnd Source=%s Target=%s Effect=%s Level=%.2f AppliedHandleValid=%d TargetTagsAfter=[%s]"),
			*GetNameSafe(CombatComponent.GetOwner()),
			*GetNameSafe(HitActor),
			*GetNameSafe(GameplayEffectToApply.GameplayEffectClass),
			GameplayEffectToApply.EffectLevel,
			AppliedHandle.IsValid(),
			*PLGetOwnedTagsString(TargetASC));
	}
}

void FSyncCombatHitWindowRuntime::ScheduleDelayedHitWindowGameplayEffects(
	AActor* HitActor,
	const FHitResult& HitResult,
	const TArray<FSyncCombatHitWindowDelayedGameplayEffect>& DelayedGameplayEffects
) const
{
	if (!CombatComponent.AbilitySystemComponent || !HitActor || HitActor == CombatComponent.GetOwner())
	{
		return;
	}

	if (DelayedGameplayEffects.IsEmpty())
	{
		return;
	}

	UWorld* World = CombatComponent.GetWorld();
	if (!World)
	{
		return;
	}

	TWeakObjectPtr<UAbilitySystemComponent> SourceASC = CombatComponent.AbilitySystemComponent;
	TWeakObjectPtr<AActor> SourceActor = CombatComponent.GetOwner();
	TWeakObjectPtr<AActor> TargetActor = HitActor;

	for (const FSyncCombatHitWindowDelayedGameplayEffect& DelayedEffect : DelayedGameplayEffects)
	{
		if (!DelayedEffect.GameplayEffectClass)
		{
			continue;
		}

		const float DelaySeconds = FMath::Max(0.f, DelayedEffect.DelaySeconds);

		UE_LOG(LogPLCombatHitDetectionRuntime, VeryVerbose,
			TEXT("ScheduleDelayedHitWindowGE Source=%s Target=%s Effect=%s Level=%.2f Delay=%.3f"),
			*GetNameSafe(SourceActor.Get()),
			*GetNameSafe(TargetActor.Get()),
			*GetNameSafe(DelayedEffect.GameplayEffectClass),
			DelayedEffect.EffectLevel,
			DelaySeconds);

		FTimerDelegate TimerDelegate;
		TimerDelegate.BindStatic(
			&FSyncCombatHitWindowRuntime::ApplyDelayedGameplayEffectToTarget,
			SourceASC,
			SourceActor,
			TargetActor,
			HitResult,
			DelayedEffect.GameplayEffectClass,
			DelayedEffect.EffectLevel
		);

		if (DelaySeconds <= 0.f)
		{
			World->GetTimerManager().SetTimerForNextTick(TimerDelegate);
			continue;
		}

		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(
			TimerHandle,
			TimerDelegate,
			DelaySeconds,
			false
		);
	}
}

void FSyncCombatHitWindowRuntime::ApplyDelayedGameplayEffectToTarget(
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC,
	TWeakObjectPtr<AActor> SourceActor,
	TWeakObjectPtr<AActor> TargetActor,
	const FHitResult HitResult,
	TSubclassOf<UGameplayEffect> GameplayEffectClass,
	const float EffectLevel
)
{
	if (!SourceASC.IsValid() || !SourceActor.IsValid() || !TargetActor.IsValid() || !GameplayEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC =
		USyncCombatFunctionLibrary::GetAbilitySystemComponent(TargetActor.Get());

	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddHitResult(HitResult);
	ContextHandle.AddInstigator(SourceActor.Get(), SourceActor.Get());

	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
		GameplayEffectClass,
		EffectLevel,
		ContextHandle
	);

	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogPLCombatHitDetectionRuntime, Warning,
			TEXT("ApplyDelayedHitWindowGESpecInvalid Source=%s Target=%s Effect=%s Level=%.2f"),
			*GetNameSafe(SourceActor.Get()),
			*GetNameSafe(TargetActor.Get()),
			*GetNameSafe(GameplayEffectClass),
			EffectLevel);
		return;
	}

	UE_LOG(LogPLCombatHitDetectionRuntime, VeryVerbose,
		TEXT("ApplyDelayedHitWindowGEStart Source=%s Target=%s Effect=%s Level=%.2f TargetTagsBefore=[%s]"),
		*GetNameSafe(SourceActor.Get()),
		*GetNameSafe(TargetActor.Get()),
		*GetNameSafe(GameplayEffectClass),
		EffectLevel,
		*PLGetOwnedTagsString(TargetASC));

	const FActiveGameplayEffectHandle AppliedHandle = SourceASC->ApplyGameplayEffectSpecToTarget(
		*SpecHandle.Data.Get(),
		TargetASC
	);

	UE_LOG(LogPLCombatHitDetectionRuntime, VeryVerbose,
		TEXT("ApplyDelayedHitWindowGEEnd Source=%s Target=%s Effect=%s Level=%.2f AppliedHandleValid=%d TargetTagsAfter=[%s]"),
		*GetNameSafe(SourceActor.Get()),
		*GetNameSafe(TargetActor.Get()),
		*GetNameSafe(GameplayEffectClass),
		EffectLevel,
		AppliedHandle.IsValid(),
		*PLGetOwnedTagsString(TargetASC));
}

bool FSyncCombatHitWindowRuntime::ShouldApplyDamageGameplayEffects(
	const bool bWasBlocked,
	const bool bWasParried,
	const bool bWasDodged,
	const ESyncCombatHitWindowSuperArmorLevel TargetSuperArmorLevel
) const
{
	const FSyncCombatHitWindowDamageSettings& DamageSettings = ActiveHitWindowSettings.DamageSettings;

	if (DamageSettings.DamageGameplayEffectsToApply.IsEmpty())
	{
		return false;
	}

	if (bWasParried && !DamageSettings.bApplyDamageWhenParried)
	{
		return false;
	}

	if (bWasBlocked && !DamageSettings.bApplyDamageWhenBlocked)
	{
		return false;
	}

	if (bWasDodged && !DamageSettings.bApplyDamageWhenDodged)
	{
		return false;
	}

	if (TargetSuperArmorLevel > DamageSettings.MaxSuperArmorLevelThatTakesDamage)
	{
		return false;
	}

	return true;
}

void FSyncCombatHitWindowRuntime::ApplyHitWindowTransformEffects(AActor* HitActor, const bool bWasBlocked,
	const bool bWasDodged, const bool bHasSuperArmor) const
{
	if (!HitActor || HitActor == CombatComponent.GetOwner()) return;
	ApplyHitWindowRotation(HitActor, ESyncCombatHitWindowTransformTriggerTiming::OnHit, bWasBlocked, bWasDodged, bHasSuperArmor);
	ApplyHitWindowMovement(HitActor, ESyncCombatHitWindowTransformTriggerTiming::OnHit, bWasBlocked, bWasDodged, bHasSuperArmor);
}

void FSyncCombatHitWindowRuntime::ApplyActivationTransformEffects() const
{
	ApplyHitWindowRotation(nullptr, ESyncCombatHitWindowTransformTriggerTiming::OnActivation, false, false, false);
	ApplyHitWindowMovement(nullptr, ESyncCombatHitWindowTransformTriggerTiming::OnActivation, false, false, false);
}

void FSyncCombatHitWindowRuntime::ApplyHitWindowMovement(AActor* HitActor,
	const ESyncCombatHitWindowTransformTriggerTiming InvocationTiming, const bool bWasBlocked,
	const bool bWasDodged, const bool bHasSuperArmor) const
{
	const FSyncCombatHitWindowMovementSettings& MovementSettings = ActiveHitWindowSettings.MovementSettings;
	if (!CanApplyHitWindowMovement(InvocationTiming, bWasBlocked, bWasDodged, bHasSuperArmor)) return;

	AActor* const OwnerActor = CombatComponent.GetOwner();
	AActor* const TargetActor = HitActor ? HitActor : LastCombatReferenceActor.Get();
	AActor* const ReferenceActor = ResolveTransformReferenceActor(MovementSettings.ReferenceActorSource,
		HitActor, InvocationTiming);

	auto ApplyToRecipient = [this, &MovementSettings, ReferenceActor](AActor* RecipientActor)
	{
		if (!RecipientActor || !ReferenceActor || RecipientActor == ReferenceActor) return;
		ApplyMovementToActor(RecipientActor, ReferenceActor, MovementSettings);
	};

	switch (MovementSettings.Recipient)
	{
	case ESyncCombatHitWindowTransformRecipient::Instigator:
		ApplyToRecipient(OwnerActor);
		break;

	case ESyncCombatHitWindowTransformRecipient::Target:
		ApplyToRecipient(TargetActor);
		break;

	case ESyncCombatHitWindowTransformRecipient::Both:
		ApplyToRecipient(OwnerActor);
		if (TargetActor && TargetActor != OwnerActor) ApplyToRecipient(TargetActor);
		break;

	default:
		return;
	}
}

bool FSyncCombatHitWindowRuntime::CanApplyHitWindowMovement(
	const ESyncCombatHitWindowTransformTriggerTiming InvocationTiming,
	const bool bWasBlocked,
	const bool bWasDodged,
	const bool bHasSuperArmor) const
{
	const FSyncCombatHitWindowMovementSettings& MovementSettings = ActiveHitWindowSettings.MovementSettings;
	if (MovementSettings.MoveDirection == ESyncCombatHitWindowMoveDirection::None)
	{
		return false;
	}

	if (MovementSettings.MoveDirection != ESyncCombatHitWindowMoveDirection::KeepCurrentDistance
		&& MovementSettings.MoveDistance <= 0.f)
	{
		return false;
	}

	if (!DoesTransformTimingMatch(MovementSettings.TriggerTiming, InvocationTiming))
	{
		return false;
	}

	if (InvocationTiming == ESyncCombatHitWindowTransformTriggerTiming::OnHit)
	{
		if (bWasDodged || bHasSuperArmor) return false;
		if (bWasBlocked && !ActiveHitWindowSettings.DefenseSettings.BlockSettings.bAllowMovementWhenBlocked) return false;
	}

	return true;
}

void FSyncCombatHitWindowRuntime::ApplyMovementToActor(AActor* RecipientActor, AActor* ReferenceActor,
	const FSyncCombatHitWindowMovementSettings& MovementSettings) const
{
	FVector NewLocation;
	if (!CalculateMovementTargetLocation(RecipientActor, ReferenceActor, MovementSettings, NewLocation)) return;

	RecipientActor->SetActorLocation(NewLocation, MovementSettings.bSweep, nullptr,
		ToTeleportType(MovementSettings.TeleportType));
}

bool FSyncCombatHitWindowRuntime::CalculateMovementTargetLocation(AActor* RecipientActor, AActor* ReferenceActor,
	const FSyncCombatHitWindowMovementSettings& MovementSettings, FVector& OutLocation) const
{
	if (!RecipientActor || !ReferenceActor) return false;

	const FVector ReferenceLocation = ReferenceActor->GetActorLocation();
	const FVector RecipientLocation = RecipientActor->GetActorLocation();

	FVector ReferenceForward = ReferenceActor->GetActorForwardVector();
	ReferenceForward.Z = 0.f;
	ReferenceForward = ReferenceForward.GetSafeNormal();

	if (ReferenceForward.IsNearlyZero()) return false;

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
	case ESyncCombatHitWindowMoveDirection::KeepCurrentDistance:
		break;

	case ESyncCombatHitWindowMoveDirection::MoveCloser:
		TargetForwardProjection -= MovementSettings.MoveDistance;
		break;

	case ESyncCombatHitWindowMoveDirection::MoveAway:
		TargetForwardProjection += MovementSettings.MoveDistance;
		break;

	case ESyncCombatHitWindowMoveDirection::SnapToDistance:
		TargetForwardProjection = MovementSettings.MoveDistance;
		break;

	case ESyncCombatHitWindowMoveDirection::None:
	default:
		return false;
	}

	switch (MovementSettings.LateralOffsetMode)
	{
	case ESyncCombatHitWindowLateralOffsetMode::KeepCurrent:
		break;

	case ESyncCombatHitWindowLateralOffsetMode::AddOffset:
		TargetLateralProjection += MovementSettings.LateralOffset;
		break;

	case ESyncCombatHitWindowLateralOffsetMode::SnapToOffset:
		TargetLateralProjection = MovementSettings.LateralOffset;
		break;

	default:
		break;
	}

	if (FMath::IsNearlyEqual(TargetForwardProjection, CurrentForwardProjection)
		&& FMath::IsNearlyEqual(TargetLateralProjection, CurrentLateralProjection))
		return false;

	FVector NewLocation = ReferenceLocation
		+ (ReferenceForward * TargetForwardProjection)
		+ (ReferenceRight * TargetLateralProjection);
	NewLocation.Z = RecipientLocation.Z;

	OutLocation = NewLocation;
	return true;
}

void FSyncCombatHitWindowRuntime::ApplyHitWindowRotation(AActor* HitActor,
	const ESyncCombatHitWindowTransformTriggerTiming InvocationTiming, const bool bWasBlocked,
	const bool bWasDodged, const bool bHasSuperArmor) const
{
	const FSyncCombatHitWindowRotationSettings& RotationSettings = ActiveHitWindowSettings.RotationSettings;
	if (!CanApplyHitWindowRotation(InvocationTiming, bWasBlocked, bWasDodged, bHasSuperArmor)) return;

	AActor* const OwnerActor = CombatComponent.GetOwner();
	AActor* const TargetActor = HitActor ? HitActor : LastCombatReferenceActor.Get();
	AActor* const ReferenceActor = ResolveTransformReferenceActor(RotationSettings.ReferenceActorSource,
		HitActor, InvocationTiming);

	auto ApplyToRecipient = [this, &RotationSettings, ReferenceActor](AActor* RecipientActor)
	{
		if (!RecipientActor || !ReferenceActor || RecipientActor == ReferenceActor) return;
		ApplyRotationToActor(RecipientActor, ReferenceActor, RotationSettings);
	};

	switch (RotationSettings.Recipient)
	{
	case ESyncCombatHitWindowTransformRecipient::Instigator:
		ApplyToRecipient(OwnerActor);
		break;

	case ESyncCombatHitWindowTransformRecipient::Target:
		ApplyToRecipient(TargetActor);
		break;

	case ESyncCombatHitWindowTransformRecipient::Both:
		ApplyToRecipient(OwnerActor);
		if (TargetActor && TargetActor != OwnerActor) ApplyToRecipient(TargetActor);
		break;

	default:
		return;
	}
}

bool FSyncCombatHitWindowRuntime::CanApplyHitWindowRotation(
	const ESyncCombatHitWindowTransformTriggerTiming InvocationTiming,
	const bool bWasBlocked,
	const bool bWasDodged,
	const bool bHasSuperArmor) const
{
	const FSyncCombatHitWindowRotationSettings& RotationSettings = ActiveHitWindowSettings.RotationSettings;
	if (RotationSettings.RotationDirection == ESyncCombatHitWindowRotationDirection::None)
	{
		return false;
	}

	if (!DoesTransformTimingMatch(RotationSettings.TriggerTiming, InvocationTiming))
	{
		return false;
	}

	if (InvocationTiming == ESyncCombatHitWindowTransformTriggerTiming::OnHit)
	{
		if (bWasDodged || bHasSuperArmor) return false;
		if (bWasBlocked && !ActiveHitWindowSettings.DefenseSettings.BlockSettings.bAllowRotationWhenBlocked) return false;
	}

	return true;
}

void FSyncCombatHitWindowRuntime::ApplyRotationToActor(AActor* RecipientActor, AActor* ReferenceActor,
	const FSyncCombatHitWindowRotationSettings& RotationSettings) const
{
	FRotator DesiredRotation;
	if (!CalculateRotationTargetRotation(RecipientActor, ReferenceActor, RotationSettings, DesiredRotation)) return;

	RecipientActor->SetActorRotation(DesiredRotation, ToTeleportType(RotationSettings.TeleportType));
}

bool FSyncCombatHitWindowRuntime::CalculateRotationTargetRotation(AActor* RecipientActor, AActor* ReferenceActor,
	const FSyncCombatHitWindowRotationSettings& RotationSettings, FRotator& OutRotation) const
{
	if (!RecipientActor || !ReferenceActor) return false;

	const FVector ReferenceLocation = ReferenceActor->GetActorLocation();
	const FVector RecipientLocation = RecipientActor->GetActorLocation();
	FRotator DesiredRotation = RecipientActor->GetActorRotation();

	switch (RotationSettings.RotationDirection)
	{
	case ESyncCombatHitWindowRotationDirection::FaceToFace:
		{
			FVector ToReference = ReferenceLocation - RecipientLocation;
			ToReference.Z = 0.f;
			if (const FVector FacingDirection = ToReference.GetSafeNormal(); !FacingDirection.IsNearlyZero())
			{
				DesiredRotation = FacingDirection.Rotation();
			}
			break;
		}

	case ESyncCombatHitWindowRotationDirection::FaceAway:
		{
			FVector AwayFromReference = RecipientLocation - ReferenceLocation;
			AwayFromReference.Z = 0.f;
			if (const FVector FacingDirection = AwayFromReference.GetSafeNormal(); !FacingDirection.IsNearlyZero())
			{
				DesiredRotation = FacingDirection.Rotation();
			}
			break;
		}

	case ESyncCombatHitWindowRotationDirection::FaceOppositeInstigatorForward:
		{
			FVector OppositeDirection = -ReferenceActor->GetActorForwardVector();
			OppositeDirection.Z = 0.f;
			if (const FVector FacingDirection = OppositeDirection.GetSafeNormal(); !FacingDirection.IsNearlyZero())
			{
				DesiredRotation = FacingDirection.Rotation();
			}
			break;
		}

	case ESyncCombatHitWindowRotationDirection::FaceDirection:
		DesiredRotation = RotationSettings.DirectionToFace;
		break;

	case ESyncCombatHitWindowRotationDirection::None:
	default:
		return false;
	}

	OutRotation = DesiredRotation;
	return true;
}

void FSyncCombatHitWindowRuntime::CaptureReactionTagCounts(AActor* HitActor,
	TMap<FGameplayTag, int32>& OutTagCounts) const
{
	OutTagCounts.Reset();

	const USyncCombatComponent* TargetCombatComponent = FindCombatComponent(HitActor);
	const UAbilitySystemComponent* TargetASC = USyncCombatFunctionLibrary::GetAbilitySystemComponent(HitActor);
	const USyncCombatTagReactionData* ReactionData = TargetCombatComponent
		? TargetCombatComponent->GetTagReactionData()
		: nullptr;
	if (!TargetASC || !ReactionData)
	{
		return;
	}

	for (const FSyncCombatTagReactionBinding& Reaction : ReactionData->Reactions)
	{
		if (!Reaction.TriggerTag.IsValid()) continue;
		OutTagCounts.FindOrAdd(Reaction.TriggerTag) = TargetASC->GetGameplayTagCount(Reaction.TriggerTag);
	}
}

void FSyncCombatHitWindowRuntime::BuildReactionPredictionTags(AActor* HitActor,
	const TMap<FGameplayTag, int32>& PreviousTagCounts, FGameplayTagContainer& OutTriggerTags,
	FGameplayTagContainer& OutAbilityTags) const
{
	OutTriggerTags.Reset();
	OutAbilityTags.Reset();

	const USyncCombatComponent* TargetCombatComponent = FindCombatComponent(HitActor);
	const UAbilitySystemComponent* TargetASC = USyncCombatFunctionLibrary::GetAbilitySystemComponent(HitActor);
	const USyncCombatTagReactionData* ReactionData = TargetCombatComponent
		? TargetCombatComponent->GetTagReactionData()
		: nullptr;
	if (!TargetASC || !ReactionData)
	{
		return;
	}

	for (const FSyncCombatTagReactionBinding& Reaction : ReactionData->Reactions)
	{
		if (!Reaction.TriggerTag.IsValid()) continue;

		const int32 PreviousCount = PreviousTagCounts.FindRef(Reaction.TriggerTag);
		const int32 CurrentCount = TargetASC->GetGameplayTagCount(Reaction.TriggerTag);
		if (CurrentCount <= PreviousCount)
		{
			continue;
		}

		if (Reaction.Policy != ESyncCombatTagReactionPolicy::OnAdd
			&& Reaction.Policy != ESyncCombatTagReactionPolicy::Both)
		{
			continue;
		}

		OutTriggerTags.AddTag(Reaction.TriggerTag);
		if (Reaction.Ability.AbilityTag.IsValid())
		{
			OutAbilityTags.AddTag(Reaction.Ability.AbilityTag);
		}
	}
}

void FSyncCombatHitWindowRuntime::SendHitReactionVisualPrediction(AActor* HitActor, const bool bWasBlocked,
	const bool bWasDodged, const bool bHasSuperArmor, const FGameplayTagContainer& ReactionTriggerTags,
	const FGameplayTagContainer& ReactionAbilityTags) const
{
	AActor* const OwnerActor = CombatComponent.GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !HitActor || HitActor == OwnerActor)
	{
		return;
	}

	USyncCombatComponent* TargetCombatComponent = FindCombatComponent(HitActor);
	if (!TargetCombatComponent)
	{
		return;
	}

	bool bApplyRotation = false;
	FRotator PredictedRotation = FRotator::ZeroRotator;
	ESyncCombatHitWindowTeleportType RotationTeleportType = ESyncCombatHitWindowTeleportType::None;

	const FSyncCombatHitWindowRotationSettings& RotationSettings = ActiveHitWindowSettings.RotationSettings;
	if (PLSyncCombatTransformRecipientIncludesTarget(RotationSettings.Recipient)
		&& CanApplyHitWindowRotation(ESyncCombatHitWindowTransformTriggerTiming::OnHit, bWasBlocked, bWasDodged, bHasSuperArmor))
	{
		AActor* const ReferenceActor = ResolveTransformReferenceActor(
			RotationSettings.ReferenceActorSource,
			HitActor,
			ESyncCombatHitWindowTransformTriggerTiming::OnHit);

		if (ReferenceActor && ReferenceActor != HitActor
			&& CalculateRotationTargetRotation(HitActor, ReferenceActor, RotationSettings, PredictedRotation))
		{
			bApplyRotation = true;
			RotationTeleportType = RotationSettings.TeleportType;
		}
	}

	bool bApplyLocation = false;
	FVector PredictedLocation = FVector::ZeroVector;
	bool bSweepLocation = false;
	ESyncCombatHitWindowTeleportType LocationTeleportType = ESyncCombatHitWindowTeleportType::None;

	const FSyncCombatHitWindowMovementSettings& MovementSettings = ActiveHitWindowSettings.MovementSettings;
	if (PLSyncCombatTransformRecipientIncludesTarget(MovementSettings.Recipient)
		&& CanApplyHitWindowMovement(ESyncCombatHitWindowTransformTriggerTiming::OnHit, bWasBlocked, bWasDodged, bHasSuperArmor))
	{
		AActor* const ReferenceActor = ResolveTransformReferenceActor(
			MovementSettings.ReferenceActorSource,
			HitActor,
			ESyncCombatHitWindowTransformTriggerTiming::OnHit);

		if (ReferenceActor && ReferenceActor != HitActor
			&& CalculateMovementTargetLocation(HitActor, ReferenceActor, MovementSettings, PredictedLocation))
		{
			bApplyLocation = true;
			bSweepLocation = MovementSettings.bSweep;
			LocationTeleportType = MovementSettings.TeleportType;
		}
	}

	if (!bApplyRotation && !bApplyLocation && ReactionTriggerTags.IsEmpty() && ReactionAbilityTags.IsEmpty())
	{
		return;
	}

	CombatComponent.MulticastApplyHitReactionVisualPrediction(
		HitActor,
		bApplyRotation,
		PredictedRotation,
		RotationTeleportType,
		bApplyLocation,
		PredictedLocation,
		bSweepLocation,
		LocationTeleportType,
		ReactionTriggerTags,
		ReactionAbilityTags);
}

AActor* FSyncCombatHitWindowRuntime::ResolveTransformReferenceActor(
	const ESyncCombatHitWindowReferenceActorSource ReferenceSource, AActor* HitActor,
	const ESyncCombatHitWindowTransformTriggerTiming InvocationTiming) const
{
	switch (ReferenceSource)
	{
	case ESyncCombatHitWindowReferenceActorSource::Instigator:
		return CombatComponent.GetOwner();

	case ESyncCombatHitWindowReferenceActorSource::Target:
		return HitActor ? HitActor : (InvocationTiming == ESyncCombatHitWindowTransformTriggerTiming::OnActivation
			? LastCombatReferenceActor.Get()
			: nullptr);

	case ESyncCombatHitWindowReferenceActorSource::LastCombatReferenceActor:
		return LastCombatReferenceActor.Get();

	default:
		return nullptr;
	}
}

bool FSyncCombatHitWindowRuntime::IsAttackBlocked(AActor* HitActor) const
{
	const FSyncCombatHitWindowBlockSettings& BlockSettings = ActiveHitWindowSettings.DefenseSettings.BlockSettings;
	if (!BlockSettings.bBlockable || !HitActor) return false;
	if (!CombatComponent.BlockingTag.IsValid()) return false;

	const UAbilitySystemComponent* TargetASC = USyncCombatFunctionLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC || !TargetASC->HasMatchingGameplayTag(CombatComponent.BlockingTag))
	{
		return false;
	}

	return IsWithinBlockAngle(HitActor, CombatComponent.GetOwner(), BlockSettings.BlockAngleDegrees);
}

bool FSyncCombatHitWindowRuntime::IsAttackParried(AActor* HitActor) const
{
	if (!HitActor) return false;

	const USyncCombatComponent* TargetCombatComponent = FindCombatComponent(HitActor);
	return TargetCombatComponent && TargetCombatComponent->IsParryingActive();
}

bool FSyncCombatHitWindowRuntime::IsAttackDodged(AActor* HitActor) const
{
	if (!ActiveHitWindowSettings.DefenseSettings.DodgeSettings.bDodgeable || !HitActor) return false;
	if (!CombatComponent.DodgingTag.IsValid()) return false;

	const UAbilitySystemComponent* TargetASC = USyncCombatFunctionLibrary::GetAbilitySystemComponent(HitActor);
	return TargetASC && TargetASC->HasMatchingGameplayTag(CombatComponent.DodgingTag);
}

bool FSyncCombatHitWindowRuntime::HasRequiredSuperArmor(AActor* HitActor) const
{
	const ESyncCombatHitWindowSuperArmorLevel RequiredSuperArmor = ActiveHitWindowSettings.DefenseSettings.RequiredSuperArmor;
	if (RequiredSuperArmor == ESyncCombatHitWindowSuperArmorLevel::None || !HitActor) return false;

	const USyncCombatComponent* TargetCombatComponent = FindCombatComponent(HitActor);
	return TargetCombatComponent && TargetCombatComponent->HasSuperArmorAtOrAbove(RequiredSuperArmor);
}

ESyncCombatHitWindowSuperArmorLevel FSyncCombatHitWindowRuntime::GetTargetSuperArmorLevel(AActor* HitActor) const
{
	if (const USyncCombatComponent* TargetCombatComponent = FindCombatComponent(HitActor))
	{
		return TargetCombatComponent->GetCurrentSuperArmorLevel();
	}

	return ESyncCombatHitWindowSuperArmorLevel::None;
}

void FSyncCombatHitWindowRuntime::ApplyDefenseGameplayEffects(AActor* HitActor, const FHitResult& HitResult,
	const bool bWasBlocked, const bool bWasParried, const bool bWasDodged, const bool bHasSuperArmor) const
{
	if (bWasBlocked)
	{
		ApplyGameplayEffectToActor(CombatComponent.GetOwner(), CombatComponent.AttackerBlockedEffectClass, 1.f, &HitResult);
		ApplyGameplayEffectToActor(HitActor, CombatComponent.DefenderBlockedEffectClass, 1.f, &HitResult);
	}

	if (bWasParried)
	{
		ApplyGameplayEffectToActor(CombatComponent.GetOwner(), CombatComponent.AttackerParriedEffectClass, 1.f, &HitResult);
		ApplyGameplayEffectToActor(HitActor, CombatComponent.DefenderParrySuccessEffectClass, 1.f, &HitResult);
	}

	if (bWasDodged)
	{
		ApplyGameplayEffectToActor(CombatComponent.GetOwner(), CombatComponent.AttackerDodgedEffectClass, 1.f, &HitResult);
		ApplyGameplayEffectToActor(HitActor, CombatComponent.DefenderDodgedEffectClass, 1.f, &HitResult);
	}

	if (bHasSuperArmor)
	{
		ApplyGameplayEffectToActor(CombatComponent.GetOwner(), CombatComponent.AttackerSuperArmoredEffectClass, 1.f, &HitResult);
		ApplyGameplayEffectToActor(HitActor, CombatComponent.DefenderSuperArmoredEffectClass, 1.f, &HitResult);
	}
}

void FSyncCombatHitWindowRuntime::ApplyGameplayEffectToActor(AActor* RecipientActor,
	const TSubclassOf<UGameplayEffect>& GameplayEffectClass, const float EffectLevel, const FHitResult* HitResult) const
{
	if (!RecipientActor || !GameplayEffectClass) return;

	UAbilitySystemComponent* RecipientASC = USyncCombatFunctionLibrary::GetAbilitySystemComponent(RecipientActor);
	if (!RecipientASC) return;

	FGameplayEffectContextHandle ContextHandle = RecipientASC->MakeEffectContext();
	ContextHandle.AddSourceObject(CombatComponent.GetOwner()
		? static_cast<const UObject*>(CombatComponent.GetOwner())
		: static_cast<const UObject*>(&CombatComponent));

	if (AActor* OwnerActor = CombatComponent.GetOwner())
	{
		ContextHandle.AddInstigator(OwnerActor, OwnerActor);
	}

	if (HitResult)
	{
		ContextHandle.AddHitResult(*HitResult);
	}

	const FGameplayEffectSpecHandle SpecHandle =
		RecipientASC->MakeOutgoingSpec(GameplayEffectClass, EffectLevel, ContextHandle);

	if (!SpecHandle.IsValid()) return;

	UE_LOG(LogPLCombatHitDetectionRuntime, VeryVerbose,
		TEXT("ApplyDefenseOrReactionGEStart Source=%s Recipient=%s Effect=%s Level=%.2f RecipientTagsBefore=[%s] Depth=%d HitActors=%d"),
		*GetNameSafe(CombatComponent.GetOwner()),
		*GetNameSafe(RecipientActor),
		*GetNameSafe(GameplayEffectClass),
		EffectLevel,
		*PLGetOwnedTagsString(RecipientASC),
		ActiveHitDebugWindowDepth,
		HitActorsThisWindow.Num());

	const FActiveGameplayEffectHandle AppliedHandle = RecipientASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	UE_LOG(LogPLCombatHitDetectionRuntime, VeryVerbose,
		TEXT("ApplyDefenseOrReactionGEEnd Source=%s Recipient=%s Effect=%s Level=%.2f AppliedHandleValid=%d RecipientTagsAfter=[%s]"),
		*GetNameSafe(CombatComponent.GetOwner()),
		*GetNameSafe(RecipientActor),
		*GetNameSafe(GameplayEffectClass),
		EffectLevel,
		AppliedHandle.IsValid(),
		*PLGetOwnedTagsString(RecipientASC));
}

void FSyncCombatHitWindowRuntime::ExecuteHitWindowGameplayCues(AActor* HitActor, const FHitResult* HitResult,
	const ESyncCombatHitWindowCueTriggerTiming TriggerTiming) const
{
	if (!CombatComponent.AbilitySystemComponent || ActiveHitWindowSettings.GameplayCuesToExecute.IsEmpty()) return;

	AActor* const OwnerActor = CombatComponent.GetOwner();
	UAbilitySystemComponent* const InstigatorASC = CombatComponent.AbilitySystemComponent;
	UAbilitySystemComponent* const TargetASC = HitActor
		? USyncCombatFunctionLibrary::GetAbilitySystemComponent(HitActor)
		: nullptr;

	for (const FSyncCombatHitWindowGameplayCue& Cue : ActiveHitWindowSettings.GameplayCuesToExecute)
	{
		if (Cue.TriggerTiming != TriggerTiming) continue;

		if (Cue.CueTag.MatchesTag(TAG_GameplayCue_Hit_CameraShake))
		{
			ExecuteLocalCameraShakeCue(Cue, HitResult);
			continue;
		}

		if (!OwnerActor || !OwnerActor->HasAuthority()) continue;

		switch (Cue.Recipient)
		{
		case ESyncCombatHitWindowCueRecipient::Instigator:
			ExecuteGameplayCueOnASC(InstigatorASC, TargetASC, Cue, HitResult);
			break;

		case ESyncCombatHitWindowCueRecipient::Target:
			ExecuteGameplayCueOnASC(TargetASC, TargetASC, Cue, HitResult);
			break;

		case ESyncCombatHitWindowCueRecipient::Both:
			ExecuteGameplayCueOnASC(InstigatorASC, TargetASC, Cue, HitResult);
			if (TargetASC && TargetASC != InstigatorASC)
			{
				ExecuteGameplayCueOnASC(TargetASC, TargetASC, Cue, HitResult);
			}
			break;

		default:
			break;
		}
	}
}

void FSyncCombatHitWindowRuntime::ExecuteGameplayCueOnASC(UAbilitySystemComponent* ASC, UAbilitySystemComponent* TargetASC,
	const FSyncCombatHitWindowGameplayCue& Cue, const FHitResult* HitResult) const
{
	if (!ASC || !Cue.HasValidCueTag()) return;

	AActor* const OwnerActor = CombatComponent.GetOwner();

	FGameplayCueParameters Params;
	Params.Instigator = OwnerActor;
	Params.EffectCauser = OwnerActor;
	Params.SourceObject = const_cast<USyncCombatComponent*>(&CombatComponent);
	Params.Location = GetGameplayCueSpawnLocation(Cue, HitResult);
	Params.Normal = HitResult
		? FVector(HitResult->ImpactNormal)
		: (OwnerActor ? OwnerActor->GetActorForwardVector() : FVector::ForwardVector);
	Params.TargetAttachComponent = GetGameplayCueAttachComponent(ASC, TargetASC, Cue, HitResult);

	ASC->ExecuteGameplayCue(Cue.CueTag, Params);
}

void FSyncCombatHitWindowRuntime::ExecuteLocalCameraShakeCue(const FSyncCombatHitWindowGameplayCue& Cue,
	const FHitResult* HitResult) const
{
	if (!ShouldExecuteLocalCameraShakeCue()) return;

	AActor* const OwnerActor = CombatComponent.GetOwner();

	FGameplayCueParameters Params;
	Params.Instigator = OwnerActor;
	Params.EffectCauser = OwnerActor;
	Params.SourceObject = const_cast<USyncCombatComponent*>(&CombatComponent);
	Params.Location = GetGameplayCueSpawnLocation(Cue, HitResult);
	Params.Normal = HitResult
		? FVector(HitResult->ImpactNormal)
		: (OwnerActor ? OwnerActor->GetActorForwardVector() : FVector::ForwardVector);
	Params.TargetAttachComponent = GetGameplayCueAttachComponent(CombatComponent.AbilitySystemComponent, nullptr, Cue, HitResult);

	CombatComponent.AbilitySystemComponent->InvokeGameplayCueEvent(Cue.CueTag, EGameplayCueEvent::Executed, Params);
}

bool FSyncCombatHitWindowRuntime::ShouldExecuteLocalCameraShakeCue() const
{
	if (CombatComponent.GetNetMode() == NM_DedicatedServer || !CombatComponent.AbilitySystemComponent) return false;

	const AActor* LocalCueActor = CombatComponent.AbilitySystemComponent->GetAvatarActor_Direct();
	if (!LocalCueActor)
	{
		LocalCueActor = CombatComponent.AbilitySystemComponent->GetOwnerActor();
	}

	const APawn* LocalCuePawn = Cast<APawn>(LocalCueActor);
	if (!LocalCuePawn) return false;

	return LocalCuePawn->IsLocallyControlled();
}

FVector FSyncCombatHitWindowRuntime::GetGameplayCueSpawnLocation(const FSyncCombatHitWindowGameplayCue& Cue,
	const FHitResult* HitResult) const
{
	AActor* const OwnerActor = CombatComponent.GetOwner();
	FVector SpawnLocation = OwnerActor ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
	if (HitResult)
	{
		switch (Cue.SpawnPoint)
		{
		case ESyncCombatHitWindowCueSpawnPoint::OwnerLocation:
			SpawnLocation = OwnerActor ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
			break;

		case ESyncCombatHitWindowCueSpawnPoint::HitImpactPoint:
			SpawnLocation = HitResult->ImpactPoint;
			break;

		case ESyncCombatHitWindowCueSpawnPoint::HitLocation:
			SpawnLocation = HitResult->Location;
			break;

		default:
			break;
		}
	}

	return SpawnLocation + Cue.LocationOffset;
}

USceneComponent* FSyncCombatHitWindowRuntime::GetGameplayCueAttachComponent(UAbilitySystemComponent* ASC,
	UAbilitySystemComponent* TargetASC, const FSyncCombatHitWindowGameplayCue& Cue, const FHitResult* HitResult) const
{
	if (!Cue.bAttachToTarget) return nullptr;

	if (ASC == TargetASC && HitResult)
	{
		return HitResult->GetComponent();
	}

	AActor* const OwnerActor = CombatComponent.GetOwner();
	return OwnerActor ? OwnerActor->GetRootComponent() : nullptr;
}

bool FSyncCombatHitWindowRuntime::BeginHitDetectionWindow(const UAnimNotifyState* NotifyState,
	USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEvent* NotifyEvent,
	const float TotalDuration,
	const FName TraceSocketName, const FSyncCombatHitWindowSettings& HitWindowSettings)
{
	if (!NotifyState || !MeshComp) return false;

	AActor* OwnerActor = CombatComponent.GetOwner();
	if (!OwnerActor || MeshComp->GetOwner() != OwnerActor) return false;

	const FObjectKey NotifyKey(NotifyState);
	const UAnimMontage* ActiveMontage = nullptr;
	float ActiveMontagePosition = -1.f;
	PLGetActiveMontageDebugData(MeshComp, ActiveMontage, ActiveMontagePosition);
	bActiveScopedHitWindowKeyValid = BuildScopedHitWindowKey(Animation, NotifyEvent, ActiveScopedHitWindowKey);

	ActiveHitDetectionWindows.FindOrAdd(NotifyKey) = TraceSocketName;

	const bool bStartingFirstActiveWindow = ActiveHitDebugWindowDepth == 0;

	int32& ActiveWindowCount = ActiveHitDetectionWindowCounts.FindOrAdd(NotifyKey);
	++ActiveWindowCount;
	++ActiveHitDebugWindowDepth;

	if (OwnerActor->HasAuthority())
	{
		UE_LOG(LogPLCombatHitDetectionRuntime, Display,
			TEXT("HitWindowBeginV4 Owner=%s Notify=%s Mesh=%s Animation=%s AnimationClass=%s NotifyDuration=%.3f ActiveMontage=%s MontagePosition=%.3f Socket=%s Scoped=%d Depth=%d NotifyCount=%d StartingFirst=%d ReactionEffects=[%s] DamageEffects=[%s] DelayedEffects=%d"),
			*GetNameSafe(OwnerActor),
			*GetNameSafe(NotifyState),
			*GetNameSafe(MeshComp),
			*GetNameSafe(Animation),
			Animation ? *Animation->GetClass()->GetName() : TEXT("None"),
			TotalDuration,
			*GetNameSafe(ActiveMontage),
			ActiveMontagePosition,
			*TraceSocketName.ToString(),
			bActiveScopedHitWindowKeyValid,
			ActiveHitDebugWindowDepth,
			ActiveWindowCount,
			bStartingFirstActiveWindow,
			*SyncCombatHitWindowEffectListToString(HitWindowSettings.GameplayEffectsToApply),
			*SyncCombatHitWindowEffectListToString(HitWindowSettings.DamageSettings.DamageGameplayEffectsToApply),
			HitWindowSettings.DelayedGameplayEffectsToApply.Num());
	}

	if (!TraceSocketName.IsNone() && !MeshComp->DoesSocketExist(TraceSocketName))
	{
		UE_LOG(LogPLCombatHitDetectionRuntime, Warning,
			TEXT("[%s] Socket %s was not found. Falling back to mesh location."),
			*GetNameSafe(OwnerActor),
			*TraceSocketName.ToString());
	}

	ActiveHitDebugMesh = MeshComp;
	ActiveHitDebugSocketName = TraceSocketName;
	ActiveHitWindowSettings = HitWindowSettings;
	if (bStartingFirstActiveWindow)
	{
		HitActorsThisWindow.Reset();
	}

	ApplyActivationTransformEffects();
	ExecuteHitWindowGameplayCues(nullptr, nullptr, ESyncCombatHitWindowCueTriggerTiming::OnActivation);

	const FTransform InitialTransform = GetHitTraceWorldTransform(MeshComp,
		TraceSocketName, HitWindowSettings.ShapeSettings);

	RunHitDebugQuery(InitialTransform, InitialTransform, false);

	PreviousHitDebugTransform = InitialTransform;
	bHasPreviousHitDebugLocation = true;
	bHitDebugWindowActive = true;

	CombatComponent.SetComponentTickEnabled(true);
	return true;
}

void FSyncCombatHitWindowRuntime::EndHitDetectionWindow(const UAnimNotifyState* NotifyState, USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation)
{
	if (!NotifyState || !MeshComp) return;

	AActor* OwnerActor = CombatComponent.GetOwner();
	if (!OwnerActor || MeshComp->GetOwner() != OwnerActor) return;

	if (bHitDebugWindowActive && ActiveHitDebugMesh && bHasPreviousHitDebugLocation)
	{
		const FTransform CurrentTransform = GetHitTraceWorldTransform(
			ActiveHitDebugMesh,
			ActiveHitDebugSocketName,
			ActiveHitWindowSettings.ShapeSettings);

		RunHitDebugQuery(PreviousHitDebugTransform, CurrentTransform, false);
		PreviousHitDebugTransform = CurrentTransform;
	}

	const FObjectKey NotifyKey(NotifyState);
	const int32 PreviousHitDebugWindowDepth = ActiveHitDebugWindowDepth;
	int32 PreviousActiveWindowCount = 0;
	int32 NewActiveWindowCount = 0;

	if (int32* ActiveWindowCount = ActiveHitDetectionWindowCounts.Find(NotifyKey))
	{
		PreviousActiveWindowCount = *ActiveWindowCount;
		*ActiveWindowCount = FMath::Max(0, *ActiveWindowCount - 1);
		NewActiveWindowCount = *ActiveWindowCount;

		if (*ActiveWindowCount == 0)
		{
			ActiveHitDetectionWindowCounts.Remove(NotifyKey);
			ActiveHitDetectionWindows.Remove(NotifyKey);
		}
	}
	else
	{
		ActiveHitDetectionWindows.Remove(NotifyKey);
	}

	ActiveHitDebugWindowDepth = FMath::Max(0, ActiveHitDebugWindowDepth - 1);

	if (OwnerActor->HasAuthority())
	{
		const UAnimMontage* ActiveMontage = nullptr;
		float ActiveMontagePosition = -1.f;
		PLGetActiveMontageDebugData(MeshComp, ActiveMontage, ActiveMontagePosition);

		UE_LOG(LogPLCombatHitDetectionRuntime, VeryVerbose,
			TEXT("HitWindowEndV3 Owner=%s Notify=%s Animation=%s AnimationClass=%s ActiveMontage=%s MontagePosition=%.3f DepthBefore=%d DepthAfter=%d NotifyCountBefore=%d NotifyCountAfter=%d HitActors=%d ResetHitWindow=%d"),
			*GetNameSafe(OwnerActor),
			*GetNameSafe(NotifyState),
			*GetNameSafe(Animation),
			Animation ? *Animation->GetClass()->GetName() : TEXT("None"),
			*GetNameSafe(ActiveMontage),
			ActiveMontagePosition,
			PreviousHitDebugWindowDepth,
			ActiveHitDebugWindowDepth,
			PreviousActiveWindowCount,
			NewActiveWindowCount,
			HitActorsThisWindow.Num(),
			ActiveHitDebugWindowDepth == 0);
	}

	if (ActiveHitDebugWindowDepth == 0)
	{
		ResetActiveHitDebugWindow();
	}
}
