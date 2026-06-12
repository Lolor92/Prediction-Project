#include "AnimNotifies/SCP_AnimNotifyState_PredictedCollision.h"

#include "Components/SCP_CombatPredictionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

void USCP_AnimNotifyState_PredictedCollision::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!ShouldRunCollision(OwnerActor))
	{
		return;
	}

	USCP_CombatPredictionComponent* PredictionComponent =
		OwnerActor->FindComponentByClass<USCP_CombatPredictionComponent>();
	if (!PredictionComponent || !PredictionComponent->HasActivePrediction())
	{
		return;
	}

	FSCP_ActiveCollisionWindow Window;
	Window.PredictionComponent = PredictionComponent;
	Window.PredictionContext = PredictionComponent->BeginPredictionEvent();

	if (!Window.PredictionContext.IsValidForPrediction())
	{
		return;
	}

	if (!BuildSampleTransforms(MeshComp, Window.PreviousSampleTransforms))
	{
		return;
	}

	ActiveWindows.Add(MeshComp, Window);
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
		ActiveWindows.Remove(MeshComp);
	}
}

FString USCP_AnimNotifyState_PredictedCollision::GetNotifyName_Implementation() const
{
	return TEXT("SCP Predicted Collision");
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

	const bool bHasSourceSocket =
		!SourceSocketName.IsNone() && MeshComp->DoesSocketExist(SourceSocketName);
	const FTransform SourceTransform = bHasSourceSocket
		? MeshComp->GetSocketTransform(SourceSocketName, RTS_World)
		: MeshComp->GetComponentTransform();

	const FTransform RelativeTransform(
		RelativeRotation,
		RelativeLocation,
		RelativeScale);

	if (!bUseEndSocketSegment)
	{
		OutTransforms.Add(RelativeTransform * SourceTransform);
		return true;
	}

	const bool bHasEndSocket =
		!EndSocketName.IsNone() && MeshComp->DoesSocketExist(EndSocketName);
	if (!bHasEndSocket)
	{
		OutTransforms.Add(RelativeTransform * SourceTransform);
		return true;
	}

	const FTransform EndTransform = MeshComp->GetSocketTransform(EndSocketName, RTS_World);
	const int32 SampleCount = FMath::Max(SegmentSamples, 1);
	OutTransforms.Reserve(SampleCount);

	for (int32 Index = 0; Index < SampleCount; ++Index)
	{
		const float Alpha = SampleCount == 1
			? 0.f
			: static_cast<float>(Index) / static_cast<float>(SampleCount - 1);

		const FVector Location = FMath::Lerp(
			SourceTransform.GetLocation(),
			EndTransform.GetLocation(),
			Alpha);
		const FQuat Rotation = FQuat::Slerp(
			SourceTransform.GetRotation(),
			EndTransform.GetRotation(),
			Alpha);
		const FVector Scale = FMath::Lerp(
			SourceTransform.GetScale3D(),
			EndTransform.GetScale3D(),
			Alpha);

		OutTransforms.Add(RelativeTransform * FTransform(Rotation, Location, Scale));
	}

	return OutTransforms.Num() > 0;
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

				Window.PredictionComponent->ReportHitWithTransformSettings(
					Window.PredictionContext,
					Hit,
					ReactionTag,
					GameplayEffects,
					HitTransformSettings);
			}

			StepStartTransform = StepEndTransform;
		}
	}

	Window.PreviousSampleTransforms = CurrentSampleTransforms;
}
