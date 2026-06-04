// Copyright ProjectLogos

#include "AnimNotifies/SyncCombatHitDetectionNotifyState.h"
#include "Components/SyncCombatComponent.h"
#include "Components/SkeletalMeshComponent.h"


void USyncCombatHitDetectionNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	USyncCombatComponent* CombatComponent = ResolveCombatComponent(MeshComp);

	if (CombatComponent)
	{
		CombatComponent->BeginHitDetectionWindow(
			this,
			MeshComp,
			Animation,
			EventReference.GetNotify(),
			TotalDuration,
			TraceSocketName,
			HitWindowSettings);
	}
}

void USyncCombatHitDetectionNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	USyncCombatComponent* CombatComponent = ResolveCombatComponent(MeshComp);

	if (CombatComponent)
	{
		CombatComponent->EndHitDetectionWindow(this, MeshComp, Animation);
	}
}

USyncCombatComponent* USyncCombatHitDetectionNotifyState::ResolveCombatComponent(USkeletalMeshComponent* MeshComp)
{
	if (!MeshComp) return nullptr;

	return MeshComp->GetOwner()
		? MeshComp->GetOwner()->FindComponentByClass<USyncCombatComponent>()
		: nullptr;
}
