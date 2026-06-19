#include "Notifies/SP_AnimNotifyState.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SP_PredictionComponent.h"
#include "Data/SP_ReactionData.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

void USP_AnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp) return;

	FSP_NotifyRuntimeWindow& Window = ActiveWindowsByMesh.FindOrAdd(MeshComp);
	Window.WindowId = FGuid::NewGuid();
	Window.ProcessedTargets.Reset();

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!ShouldRunPredictedCollision(OwnerActor)) return;

	FTransform CurrentTransform;
	if (!BuildTraceTransform(MeshComp, CurrentTransform)) return;

	PreviousTransforms.Add(MeshComp, CurrentTransform);

	UE_LOG(LogTemp, Warning, TEXT("SP Collision Begin Owner=%s Anim=%s"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(Animation));
}

void USP_AnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp) return;

	FTransform* PreviousTransform = PreviousTransforms.Find(MeshComp);
	if (!PreviousTransform) return;

	FTransform CurrentTransform;
	if (!BuildTraceTransform(MeshComp, CurrentTransform)) return;

	SweepCollision(MeshComp, *PreviousTransform, CurrentTransform);

	*PreviousTransform = CurrentTransform;
}

void USP_AnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp)
	{
		ActiveWindowsByMesh.Remove(MeshComp);
	}

	if (MeshComp)
	{
		PreviousTransforms.Remove(MeshComp);

		UE_LOG(LogTemp, Warning, TEXT("SP Collision End Owner=%s Anim=%s"),
			*GetNameSafe(MeshComp->GetOwner()),
			*GetNameSafe(Animation));
	}
}

FString USP_AnimNotifyState::GetNotifyName_Implementation() const
{
	return TEXT("SP Predicted Collision");
}

bool USP_AnimNotifyState::ShouldRunPredictedCollision(const AActor* OwnerActor) const
{
	if (!OwnerActor) return false;

	if (OwnerActor->HasAuthority()) return true;

	const APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	if (OwnerPawn && OwnerPawn->IsLocallyControlled()) return true;

	return false;
}

bool USP_AnimNotifyState::BuildTraceTransform(USkeletalMeshComponent* MeshComp, FTransform& OutTransform) const
{
	if (!MeshComp) return false;

	const bool bHasSocket = !SourceSocketName.IsNone() && MeshComp->DoesSocketExist(SourceSocketName);
	const FTransform SourceTransform = bHasSocket
		? MeshComp->GetSocketTransform(SourceSocketName, RTS_World)
		: MeshComp->GetComponentTransform();

	const FTransform RelativeTransform(RelativeRotation, RelativeLocation);

	OutTransform = RelativeTransform * SourceTransform;
	return true;
}

FCollisionShape USP_AnimNotifyState::MakeCollisionShape() const
{
	switch (CollisionShape)
	{
	case ESP_CollisionShape::Sphere:
		return FCollisionShape::MakeSphere(FMath::Max(SphereRadius, 1.f));

	case ESP_CollisionShape::Capsule:
		return FCollisionShape::MakeCapsule(FMath::Max(CapsuleRadius, 1.f),
			FMath::Max(CapsuleHalfHeight, 1.f));

	case ESP_CollisionShape::Box:
	default:
		return FCollisionShape::MakeBox(BoxExtent.ComponentMax(FVector(1.f)));
	}
}

void USP_AnimNotifyState::SweepCollision(USkeletalMeshComponent* MeshComp, const FTransform& PreviousTransform,
	const FTransform& CurrentTransform)
{
	if (!MeshComp) return;

	AActor* OwnerActor = MeshComp->GetOwner();
	UWorld* World = MeshComp->GetWorld();

	if (!OwnerActor || !World) return;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SP_PredictedCollisionSweep), false, OwnerActor);
	QueryParams.AddIgnoredActor(OwnerActor);

	TArray<FHitResult> Hits;

	World->SweepMultiByChannel(Hits, PreviousTransform.GetLocation(), CurrentTransform.GetLocation(),
		CurrentTransform.GetRotation(), TraceChannel.GetValue(), MakeCollisionShape(), QueryParams);

	if (bDrawDebug)
	{
		const bool bHadHit = Hits.Num() > 0;
		const FColor Color = bHadHit ? FColor::Green : FColor::Red;

		DrawDebugLine(World, PreviousTransform.GetLocation(), CurrentTransform.GetLocation(), Color, false, 0.25f, 0, 2.f);

		switch (CollisionShape)
		{
		case ESP_CollisionShape::Sphere:
			DrawDebugSphere(World, CurrentTransform.GetLocation(), SphereRadius, 12, Color, false, 0.25f);
			break;

		case ESP_CollisionShape::Capsule:
			DrawDebugCapsule(World, CurrentTransform.GetLocation(), CapsuleHalfHeight, CapsuleRadius, CurrentTransform.GetRotation(), Color, false, 0.25f);
			break;

		case ESP_CollisionShape::Box:
		default:
			DrawDebugBox(World, CurrentTransform.GetLocation(), BoxExtent, CurrentTransform.GetRotation(), Color, false, 0.25f);
			break;
		}
	}

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();

		if (!HitActor || HitActor == OwnerActor) continue;

		UE_LOG(LogTemp, Warning, TEXT("SP Collision Hit Owner=%s HitActor=%s Location=%s"),
			*GetNameSafe(OwnerActor),
			*GetNameSafe(HitActor),
			*Hit.ImpactPoint.ToString());

		if (HasAlreadyProcessedTarget(MeshComp, HitActor)) continue;

		MarkTargetProcessed(MeshComp, HitActor);

		TryPlayPredictedReaction(OwnerActor, HitActor);
		TryApplyReactionEffects(OwnerActor, HitActor);
	}
}

void USP_AnimNotifyState::TryPlayPredictedReaction(AActor* AttackerActor, AActor* HitActor) const
{
	if (!bPlayPredictedReactionOnClient) return;
	if (!AttackerActor || !HitActor) return;

	UWorld* World = AttackerActor->GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer) return;

	// Server does not do predicted target animation here.
	if (AttackerActor->HasAuthority()) return;

	const APawn* AttackerPawn = Cast<APawn>(AttackerActor);
	if (!AttackerPawn || !AttackerPawn->IsLocallyControlled()) return;

	// Only predict reaction on remote targets/proxies.
	if (HitActor->HasAuthority()) return;

	const APawn* HitPawn = Cast<APawn>(HitActor);
	if (HitPawn && HitPawn->IsLocallyControlled()) return;

	USP_PredictionComponent* PredictionComponent = AttackerActor->FindComponentByClass<USP_PredictionComponent>();

	if (!PredictionComponent) return;

	if (!PredictedReactionTag.IsValid()) return;

	PredictionComponent->PlayPredictedReactionOnTargetProxy(HitActor, PredictedReactionTag);
}

void USP_AnimNotifyState::TryApplyReactionEffects(AActor* AttackerActor, AActor* HitActor) const
{
	if (!AttackerActor || !HitActor) return;

	// Only server applies GameplayEffects.
	if (!AttackerActor->HasAuthority()) return;

	if (!PredictedReactionTag.IsValid()) return;

	USP_PredictionComponent* PredictionComponent =
		AttackerActor->FindComponentByClass<USP_PredictionComponent>();

	if (!PredictionComponent) return;

	PredictionComponent->ApplyReactionEffectsToTarget(HitActor, PredictedReactionTag);
}

bool USP_AnimNotifyState::HasAlreadyProcessedTarget(USkeletalMeshComponent* MeshComp, AActor* TargetActor) const
{
	if (!MeshComp || !TargetActor) return true;

	const TWeakObjectPtr<USkeletalMeshComponent> MeshKey(MeshComp);
	const FSP_NotifyRuntimeWindow* Window = ActiveWindowsByMesh.Find(MeshKey);
	if (!Window) return false;

	const TWeakObjectPtr<AActor> TargetKey(TargetActor);
	return Window->ProcessedTargets.Contains(TargetKey);
}

void USP_AnimNotifyState::MarkTargetProcessed(USkeletalMeshComponent* MeshComp, AActor* TargetActor)
{
	if (!MeshComp || !TargetActor) return;

	const TWeakObjectPtr<USkeletalMeshComponent> MeshKey(MeshComp);
	FSP_NotifyRuntimeWindow& Window = ActiveWindowsByMesh.FindOrAdd(MeshKey);

	const TWeakObjectPtr<AActor> TargetKey(TargetActor);
	Window.ProcessedTargets.Add(TargetKey);
}
