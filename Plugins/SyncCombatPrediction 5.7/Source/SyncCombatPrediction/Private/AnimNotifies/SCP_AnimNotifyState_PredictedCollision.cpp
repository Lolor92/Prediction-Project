#include "AnimNotifies/SCP_AnimNotifyState_PredictedCollision.h"

#include "Components/SCP_CombatPredictionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSyncCombatPredictionNotifyDiag, Log, All);

namespace
{
	bool IsSyncCombatPredictionNotifyDiagnosticsEnabled()
	{
		const IConsoleVariable* DiagnosticsCVar =
			IConsoleManager::Get().FindConsoleVariable(TEXT("sync.CombatPrediction.Diagnostics"));
		return DiagnosticsCVar && DiagnosticsCVar->GetInt() != 0;
	}

	FString BuildNotifyActorDebugString(const AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("None");
		}

		const APawn* Pawn = Cast<APawn>(Actor);
		return FString::Printf(
			TEXT("Name=%s Local=%s Authority=%s NetMode=%d"),
			*Actor->GetName(),
			Pawn && Pawn->IsLocallyControlled() ? TEXT("true") : TEXT("false"),
			Actor->HasAuthority() ? TEXT("true") : TEXT("false"),
			static_cast<int32>(Actor->GetNetMode()));
	}
}

void USCP_AnimNotifyState_PredictedCollision::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		if (IsSyncCombatPredictionNotifyDiagnosticsEnabled())
		{
			UE_LOG(
				LogSyncCombatPredictionNotifyDiag,
				Log,
				TEXT("CollisionNotifyBegin skipped Animation=%s Reason=NoMesh"),
				*GetNameSafe(Animation));
		}
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!ShouldRunCollision(OwnerActor))
	{
		if (IsSyncCombatPredictionNotifyDiagnosticsEnabled())
		{
			UE_LOG(
				LogSyncCombatPredictionNotifyDiag,
				Log,
				TEXT("CollisionNotifyBegin skipped Owner={%s} Animation=%s Reason=RoleFiltered"),
				*BuildNotifyActorDebugString(OwnerActor),
				*GetNameSafe(Animation));
		}
		return;
	}

	USCP_CombatPredictionComponent* PredictionComponent =
		OwnerActor->FindComponentByClass<USCP_CombatPredictionComponent>();
	if (!PredictionComponent || !PredictionComponent->HasActivePrediction())
	{
		if (IsSyncCombatPredictionNotifyDiagnosticsEnabled())
		{
			UE_LOG(
				LogSyncCombatPredictionNotifyDiag,
				Log,
				TEXT("CollisionNotifyBegin skipped Owner={%s} Animation=%s Reason=%s"),
				*BuildNotifyActorDebugString(OwnerActor),
				*GetNameSafe(Animation),
				PredictionComponent ? TEXT("NoActivePrediction") : TEXT("NoPredictionComponent"));
		}
		return;
	}

	FSCP_ActiveCollisionWindow Window;
	Window.PredictionComponent = PredictionComponent;
	Window.PredictionContext = PredictionComponent->BeginPredictionEvent();

	if (!Window.PredictionContext.IsValidForPrediction())
	{
		if (IsSyncCombatPredictionNotifyDiagnosticsEnabled())
		{
			UE_LOG(
				LogSyncCombatPredictionNotifyDiag,
				Log,
				TEXT("CollisionNotifyBegin skipped Owner={%s} Animation=%s Reason=InvalidPredictionContext Context={%s}"),
				*BuildNotifyActorDebugString(OwnerActor),
				*GetNameSafe(Animation),
				*Window.PredictionContext.ToDebugString());
		}
		return;
	}

	if (!BuildSampleTransforms(MeshComp, Window.PreviousSampleTransforms))
	{
		if (IsSyncCombatPredictionNotifyDiagnosticsEnabled())
		{
			UE_LOG(
				LogSyncCombatPredictionNotifyDiag,
				Log,
				TEXT("CollisionNotifyBegin skipped Owner={%s} Animation=%s Reason=BuildSampleTransformsFailed Socket=%s CustomSocket=%s"),
				*BuildNotifyActorDebugString(OwnerActor),
				*GetNameSafe(Animation),
				*SourceSocketName.ToString(),
				*CustomSourceSocketName.ToString());
		}
		return;
	}

	ActiveWindows.Add(MeshComp, Window);

	if (IsSyncCombatPredictionNotifyDiagnosticsEnabled())
	{
		UE_LOG(
			LogSyncCombatPredictionNotifyDiag,
			Log,
			TEXT("CollisionNotifyBegin active Owner={%s} Animation=%s Context={%s} Samples=%d Socket=%s Shape=%d"),
			*BuildNotifyActorDebugString(OwnerActor),
			*GetNameSafe(Animation),
			*Window.PredictionContext.ToDebugString(),
			Window.PreviousSampleTransforms.Num(),
			*ResolveSourceSocketName().ToString(),
			static_cast<int32>(CollisionShape));
	}
}

void USCP_AnimNotifyState_PredictedCollision::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp)
	{
		return;
	}

	FSCP_ActiveCollisionWindow* Window = ActiveWindows.Find(MeshComp);
	if (!Window)
	{
		return;
	}

	SweepCollisionWindow(MeshComp, *Window);
}

void USCP_AnimNotifyState_PredictedCollision::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp)
	{
		if (IsSyncCombatPredictionNotifyDiagnosticsEnabled())
		{
			if (const FSCP_ActiveCollisionWindow* Window = ActiveWindows.Find(MeshComp))
			{
				UE_LOG(
					LogSyncCombatPredictionNotifyDiag,
					Log,
					TEXT("CollisionNotifyEnd Owner={%s} Animation=%s Context={%s}"),
					*BuildNotifyActorDebugString(MeshComp->GetOwner()),
					*GetNameSafe(Animation),
					*Window->PredictionContext.ToDebugString());
			}
		}

		ActiveWindows.Remove(MeshComp);
	}
}

FString USCP_AnimNotifyState_PredictedCollision::GetNotifyName_Implementation() const
{
	return TEXT("SCP Predicted Collision");
}

TArray<FName> USCP_AnimNotifyState_PredictedCollision::GetSourceSocketNameOptions() const
{
	return {
		TEXT("OffHandWeaponSocket"),
		TEXT("MainHandWeaponSocket"),
		TEXT("LeftFootSocket"),
		TEXT("RightFootSocket"),
		TEXT("Custom")
	};
}

FName USCP_AnimNotifyState_PredictedCollision::ResolveSourceSocketName() const
{
	static const FName CustomSocketOption(TEXT("Custom"));
	return SourceSocketName == CustomSocketOption
		? CustomSourceSocketName
		: SourceSocketName;
}

bool USCP_AnimNotifyState_PredictedCollision::ShouldRunCollision(const AActor* OwnerActor) const
{
	if (!OwnerActor)
	{
		return false;
	}

	if (OwnerActor->HasAuthority())
	{
		return true;
	}

	const APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}

bool USCP_AnimNotifyState_PredictedCollision::BuildSampleTransforms(
	USkeletalMeshComponent* MeshComp,
	TArray<FTransform>& OutTransforms) const
{
	OutTransforms.Reset();

	if (!MeshComp)
	{
		return false;
	}

	const FName ResolvedSourceSocketName = ResolveSourceSocketName();
	const bool bHasSourceSocket =
		!ResolvedSourceSocketName.IsNone() && MeshComp->DoesSocketExist(ResolvedSourceSocketName);
	const FTransform SourceTransform = bHasSourceSocket
		? MeshComp->GetSocketTransform(ResolvedSourceSocketName, RTS_World)
		: MeshComp->GetComponentTransform();

	const FTransform RelativeTransform(
		RelativeRotation,
		RelativeLocation,
		RelativeScale);

	OutTransforms.Add(RelativeTransform * SourceTransform);
	return true;
}

FCollisionShape USCP_AnimNotifyState_PredictedCollision::MakeCollisionShape() const
{
	switch (CollisionShape)
	{
	case ESCP_CollisionShape::Sphere:
		return FCollisionShape::MakeSphere(FMath::Max(SphereRadius, 0.1f));

	case ESCP_CollisionShape::Capsule:
		return FCollisionShape::MakeCapsule(
			FMath::Max(CapsuleRadius, 0.1f),
			FMath::Max(CapsuleHalfHeight, 0.1f));

	case ESCP_CollisionShape::Box:
	default:
		return FCollisionShape::MakeBox(BoxExtent.ComponentMax(FVector(0.1f)));
	}
}

int32 USCP_AnimNotifyState_PredictedCollision::CalculateSubstepCount(
	const FTransform& PreviousTransform,
	const FTransform& CurrentTransform) const
{
	const float Distance = FVector::Distance(
		PreviousTransform.GetLocation(),
		CurrentTransform.GetLocation());
	const float DistanceStep = FMath::Max(MaxSweepStepDistance, 1.f);
	const int32 DistanceSteps = FMath::CeilToInt(Distance / DistanceStep);

	const float RotationAngleDegrees = FMath::RadiansToDegrees(
		PreviousTransform.GetRotation().AngularDistance(CurrentTransform.GetRotation()));
	const float AngleStep = FMath::Clamp(MaxSweepStepAngleDegrees, 1.f, 180.f);
	const int32 RotationSteps = FMath::CeilToInt(RotationAngleDegrees / AngleStep);

	return FMath::Clamp(
		FMath::Max3(1, DistanceSteps, RotationSteps),
		1,
		FMath::Max(MaxSweepSubsteps, 1));
}

FTransform USCP_AnimNotifyState_PredictedCollision::InterpolateTransform(
	const FTransform& PreviousTransform,
	const FTransform& CurrentTransform,
	float Alpha) const
{
	const FVector Location = FMath::Lerp(
		PreviousTransform.GetLocation(),
		CurrentTransform.GetLocation(),
		Alpha);
	const FQuat Rotation = FQuat::Slerp(
		PreviousTransform.GetRotation(),
		CurrentTransform.GetRotation(),
		Alpha);
	const FVector Scale = FMath::Lerp(
		PreviousTransform.GetScale3D(),
		CurrentTransform.GetScale3D(),
		Alpha);

	return FTransform(Rotation, Location, Scale);
}

void USCP_AnimNotifyState_PredictedCollision::SweepCollisionWindow(
	USkeletalMeshComponent* MeshComp,
	FSCP_ActiveCollisionWindow& Window)
{
	if (!MeshComp || !Window.PredictionComponent)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	UWorld* World = MeshComp->GetWorld();
	if (!OwnerActor || !World)
	{
		return;
	}

	TArray<FTransform> CurrentSampleTransforms;
	if (!BuildSampleTransforms(MeshComp, CurrentSampleTransforms))
	{
		return;
	}

	if (CurrentSampleTransforms.Num() != Window.PreviousSampleTransforms.Num())
	{
		Window.PreviousSampleTransforms = CurrentSampleTransforms;
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SCP_PredictedCollisionSweep), false, OwnerActor);
	QueryParams.AddIgnoredActor(OwnerActor);

	const FCollisionShape QueryShape = MakeCollisionShape();

	for (int32 Index = 0; Index < CurrentSampleTransforms.Num(); ++Index)
	{
		const FTransform& PreviousTransform = Window.PreviousSampleTransforms[Index];
		const FTransform& CurrentTransform = CurrentSampleTransforms[Index];
		const int32 SubstepCount = CalculateSubstepCount(PreviousTransform, CurrentTransform);

		FTransform StepStartTransform = PreviousTransform;
		for (int32 SubstepIndex = 1; SubstepIndex <= SubstepCount; ++SubstepIndex)
		{
			const float Alpha = static_cast<float>(SubstepIndex) / static_cast<float>(SubstepCount);
			const FTransform StepEndTransform =
				InterpolateTransform(PreviousTransform, CurrentTransform, Alpha);

			const FVector Start = StepStartTransform.GetLocation();
			const FVector End = StepEndTransform.GetLocation();

			TArray<FHitResult> Hits;
			World->SweepMultiByChannel(
				Hits,
				Start,
				End,
				StepEndTransform.GetRotation(),
				TraceChannel.GetValue(),
				QueryShape,
				QueryParams);

			if (bDrawDebug)
			{
				const bool bHadHit = Hits.Num() > 0;
				const FColor Color = bHadHit ? FColor::Green : FColor::Red;
				DrawDebugLine(World, Start, End, Color, false, 0.25f, 0, 1.5f);

				switch (CollisionShape)
				{
				case ESCP_CollisionShape::Sphere:
					DrawDebugSphere(World, End, SphereRadius, 12, Color, false, 0.25f);
					break;

				case ESCP_CollisionShape::Capsule:
					DrawDebugCapsule(
						World,
						End,
						CapsuleHalfHeight,
						CapsuleRadius,
						StepEndTransform.GetRotation(),
						Color,
						false,
						0.25f);
					break;

				case ESCP_CollisionShape::Box:
				default:
					DrawDebugBox(
						World,
						End,
						BoxExtent,
						StepEndTransform.GetRotation(),
						Color,
						false,
						0.25f);
					break;
				}
			}

			for (const FHitResult& Hit : Hits)
			{
				AActor* HitActor = Hit.GetActor();
				if (!HitActor || HitActor == OwnerActor)
				{
					continue;
				}

				FSCP_HitTransformSettings TransformSettings;
				TransformSettings.MovementSettings = MovementSettings;
				TransformSettings.RotationSettings = RotationSettings;

				FSCP_HitDefenseSettings HitDefenseSettings;
				HitDefenseSettings.BlockSettings.bBlockable = bBlockable;
				HitDefenseSettings.BlockSettings.BlockAngleDegrees = BlockAngleDegrees;
				HitDefenseSettings.BlockSettings.bAllowMovementWhenBlocked = bAllowMovementWhenBlocked;
				HitDefenseSettings.BlockSettings.bAllowRotationWhenBlocked = bAllowRotationWhenBlocked;
				HitDefenseSettings.DodgeSettings.bDodgeable = bDodgeable;
				HitDefenseSettings.RequiredSuperArmor = RequiredSuperArmor;

				FSCP_HitDamageDefenseSettings HitDamageDefenseSettings;
				HitDamageDefenseSettings.bApplyDamageWhenBlocked = bApplyDamageWhenBlocked;
				HitDamageDefenseSettings.bApplyDamageWhenDodged = bApplyDamageWhenDodged;
				HitDamageDefenseSettings.MaxSuperArmorLevelThatTakesDamage = MaxSuperArmorLevelThatTakesDamage;

				Window.PredictionComponent->ReportHitWithSettings(
					Window.PredictionContext,
					Hit,
					ReactionTag,
					GameplayEffects,
					TransformSettings,
					HitDefenseSettings,
					HitDamageDefenseSettings);
			}

			StepStartTransform = StepEndTransform;
		}
	}

	Window.PreviousSampleTransforms = CurrentSampleTransforms;
}
