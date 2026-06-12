#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "Types/SCP_EffectTypes.h"
#include "Types/SCP_PredictionTypes.h"
#include "Engine/EngineTypes.h"
#include "CollisionShape.h" 
#include "SCP_AnimNotifyState_PredictedCollision.generated.h"

class USCP_CombatPredictionComponent;

UENUM(BlueprintType)
enum class ESCP_CollisionShape : uint8
{
	Sphere,
	Box,
	Capsule
};

USTRUCT()
struct FSCP_ActiveCollisionWindow
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<USCP_CombatPredictionComponent> PredictionComponent = nullptr;

	UPROPERTY()
	FSCP_CombatPredictionContext PredictionContext;

	UPROPERTY()
	TArray<FTransform> PreviousSampleTransforms;
};

UCLASS(meta=(DisplayName="SCP Predicted Collision"))
class SYNCCOMBATPREDICTION_API USCP_AnimNotifyState_PredictedCollision : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Reaction")
	FGameplayTag ReactionTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Effects", meta=(ShowOnlyInnerProperties))
	FSCP_GameplayEffectSettings GameplayEffects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Hit Transform", meta=(ShowOnlyInnerProperties))
	FSCP_HitMovementSettings MovementSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Hit Transform", meta=(ShowOnlyInnerProperties))
	FSCP_HitRotationSettings RotationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Defense", meta=(ShowOnlyInnerProperties))
	FSCP_HitDefenseSettings DefenseSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Damage", meta=(ShowOnlyInnerProperties))
	FSCP_HitDamageDefenseSettings DamageDefenseSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Socket")
	FName SourceSocketName = TEXT("RightHandSocket");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Shape")
	ESCP_CollisionShape CollisionShape = ESCP_CollisionShape::Box;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Shape", meta=(EditCondition="CollisionShape==ESCP_CollisionShape::Sphere", EditConditionHides, ClampMin="0.1", UIMin="1.0", Units="cm"))
	float SphereRadius = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Shape", meta=(EditCondition="CollisionShape==ESCP_CollisionShape::Box", EditConditionHides, ClampMin="0.1", UIMin="1.0", Units="cm"))
	FVector BoxExtent = FVector(15.f, 40.f, 15.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Shape", meta=(EditCondition="CollisionShape==ESCP_CollisionShape::Capsule", EditConditionHides, ClampMin="0.1", UIMin="1.0", Units="cm"))
	float CapsuleRadius = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Shape", meta=(EditCondition="CollisionShape==ESCP_CollisionShape::Capsule", EditConditionHides, ClampMin="0.1", UIMin="1.0", Units="cm"))
	float CapsuleHalfHeight = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Shape")
	FVector RelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Shape")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Shape")
	FVector RelativeScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Collision")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Collision", meta=(ClampMin="1.0", UIMin="1.0", Units="cm"))
	float MaxSweepStepDistance = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Collision", meta=(ClampMin="1.0", ClampMax="180.0", UIMin="1.0", UIMax="45.0", Units="Degrees"))
	float MaxSweepStepAngleDegrees = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Collision", meta=(ClampMin="1", UIMin="1", UIMax="16"))
	int32 MaxSweepSubsteps = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sync Combat Prediction|Debug")
	bool bDrawDebug = false;

private:
	UPROPERTY(Transient)
	TMap<USkeletalMeshComponent*, FSCP_ActiveCollisionWindow> ActiveWindows;

	bool ShouldRunCollision(const AActor* OwnerActor) const;
	bool BuildSampleTransforms(USkeletalMeshComponent* MeshComp, TArray<FTransform>& OutTransforms) const;
	FCollisionShape MakeCollisionShape() const;
	int32 CalculateSubstepCount(const FTransform& PreviousTransform, const FTransform& CurrentTransform) const;
	FTransform InterpolateTransform(const FTransform& PreviousTransform, const FTransform& CurrentTransform, float Alpha) const;
	void SweepCollisionWindow(USkeletalMeshComponent* MeshComp, FSCP_ActiveCollisionWindow& Window);
};
