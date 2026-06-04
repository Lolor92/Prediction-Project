// Copyright ProjectLogos

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Data/SyncCombatHitWindowTypes.h"
#include "SyncCombatHitDetectionNotifyState.generated.h"

class USyncCombatComponent;

UCLASS(meta=(DisplayName="HitWindow"))
class SYNCCOMBAT_API USyncCombatHitDetectionNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

private:
	static USyncCombatComponent* ResolveCombatComponent(USkeletalMeshComponent* MeshComp);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection")
	FName TraceSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection",
		meta=(ShowOnlyInnerProperties))
	FSyncCombatHitWindowSettings HitWindowSettings;
};
