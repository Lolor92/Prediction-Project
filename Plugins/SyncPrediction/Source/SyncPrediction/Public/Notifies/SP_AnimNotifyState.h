#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "SP_AnimNotifyState.generated.h"

class USP_ReactionData;
class AActor;

UENUM(BlueprintType)
enum class ESP_CollisionShape : uint8
{
	Sphere,
	Box,
	Capsule
};

struct FSP_NotifyRuntimeWindow
{
	FGuid WindowId;

	TSet<TWeakObjectPtr<AActor>> ProcessedTargets;

	bool bHasPreviousSweepTransform = false;
	FTransform PreviousSweepTransform = FTransform::Identity;
};

UCLASS()
class SYNCPREDICTION_API USP_AnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sync Prediction|Predicted Reaction")
	bool bPlayPredictedReactionOnClient = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Prediction|Predicted Reaction", meta=(EditCondition="bPlayPredictedReactionOnClient"))
	FGameplayTag PredictedReactionTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Prediction|Socket")
	FName SourceSocketName = TEXT("MainHandWeaponSocket");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Prediction|Shape")
	ESP_CollisionShape CollisionShape = ESP_CollisionShape::Box;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Prediction|Shape", meta=(EditCondition="CollisionShape==ESP_CollisionShape::Sphere", EditConditionHides))
	float SphereRadius = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Prediction|Shape", meta=(EditCondition="CollisionShape==ESP_CollisionShape::Box", EditConditionHides))
	FVector BoxExtent = FVector(20.f, 45.f, 20.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Prediction|Shape", meta=(EditCondition="CollisionShape==ESP_CollisionShape::Capsule", EditConditionHides))
	float CapsuleRadius = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Prediction|Shape", meta=(EditCondition="CollisionShape==ESP_CollisionShape::Capsule", EditConditionHides))
	float CapsuleHalfHeight = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Prediction|Shape")
	FVector RelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Prediction|Shape")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Prediction|Collision")
	float MaxSweepStepDistance = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Prediction|Collision")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Prediction|Debug")
	bool bDrawDebug = false;

private:
	UPROPERTY(Transient)
	TMap<TObjectPtr<USkeletalMeshComponent>, FTransform> PreviousTransforms;

	bool ShouldRunPredictedCollision(const AActor* OwnerActor) const;
	bool BuildTraceTransform(USkeletalMeshComponent* MeshComp, FTransform& OutTransform) const;
	FCollisionShape MakeCollisionShape() const;
	void SweepCollision(USkeletalMeshComponent* MeshComp, const FTransform& PreviousTransform, const FTransform& CurrentTransform);

	void TryPlayPredictedReaction(AActor* AttackerActor, AActor* HitActor) const;
	void TryApplyReactionEffects(AActor* AttackerActor, AActor* HitActor) const;

	TMap<TWeakObjectPtr<USkeletalMeshComponent>, FSP_NotifyRuntimeWindow> ActiveWindowsByMesh;

	bool HasAlreadyProcessedTarget(USkeletalMeshComponent* MeshComp, AActor* TargetActor) const;

	void MarkTargetProcessed(USkeletalMeshComponent* MeshComp, AActor* TargetActor);
};
