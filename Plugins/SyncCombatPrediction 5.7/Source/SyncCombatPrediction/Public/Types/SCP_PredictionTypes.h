#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "GameplayTagContainer.h"
#include "Types/SCP_EffectTypes.h"
#include "SCP_PredictionTypes.generated.h"

USTRUCT(BlueprintType)
struct SYNCCOMBATPREDICTION_API FSCP_CombatPredictionContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Sync Combat Prediction")
	int32 AbilityPredictionKey = 0;

	UPROPERTY(BlueprintReadOnly, Category="Sync Combat Prediction")
	int32 AbilityPredictionBase = 0;

	UPROPERTY(BlueprintReadOnly, Category="Sync Combat Prediction")
	int32 AbilitySpecHandle = 0;

	UPROPERTY(BlueprintReadOnly, Category="Sync Combat Prediction")
	int32 PredictionEventId = 0;

	UPROPERTY(BlueprintReadOnly, Category="Sync Combat Prediction")
	bool bIsLocallyControlled = false;

	UPROPERTY(BlueprintReadOnly, Category="Sync Combat Prediction")
	bool bIsAuthority = false;

	UPROPERTY(BlueprintReadOnly, Category="Sync Combat Prediction")
	bool bHasValidPredictionKey = false;

	bool IsValidForPrediction() const
	{
		return bHasValidPredictionKey && AbilityPredictionKey != 0;
	}

	FString ToDebugString() const
	{
		return FString::Printf(
			TEXT("Key=%d Base=%d Spec=%d Event=%d Local=%s Authority=%s"),
			AbilityPredictionKey,
			AbilityPredictionBase,
			AbilitySpecHandle,
			PredictionEventId,
			bIsLocallyControlled ? TEXT("true") : TEXT("false"),
			bIsAuthority ? TEXT("true") : TEXT("false"));
	}
};

UENUM(BlueprintType)
enum class ESCP_HitMoveDirection : uint8
{
	None UMETA(DisplayName="None"),
	KeepCurrentDistance UMETA(DisplayName="Keep Current Distance"),
	MoveCloser UMETA(DisplayName="Move Closer"),
	MoveAway UMETA(DisplayName="Move Away"),
	SnapToDistance UMETA(DisplayName="Snap To Distance")
};

UENUM(BlueprintType)
enum class ESCP_HitLateralOffsetMode : uint8
{
	KeepCurrent UMETA(DisplayName="Keep Current"),
	AddOffset UMETA(DisplayName="Add Offset"),
	SnapToOffset UMETA(DisplayName="Snap To Offset")
};

UENUM(BlueprintType)
enum class ESCP_HitTransformTriggerTiming : uint8
{
	OnHit UMETA(DisplayName="On Hit"),
	OnActivation UMETA(DisplayName="On Activation"),
	Both UMETA(DisplayName="Both")
};

UENUM(BlueprintType)
enum class ESCP_HitTransformRecipient : uint8
{
	Instigator UMETA(DisplayName="Instigator"),
	Target UMETA(DisplayName="Target"),
	Both UMETA(DisplayName="Both")
};

UENUM(BlueprintType)
enum class ESCP_HitTransformReferenceActorSource : uint8
{
	Instigator UMETA(DisplayName="Instigator"),
	Target UMETA(DisplayName="Target")
};

UENUM(BlueprintType)
enum class ESCP_HitTransformTeleportType : uint8
{
	None UMETA(DisplayName="None"),
	ResetPhysics UMETA(DisplayName="Reset Physics")
};

static FORCEINLINE ETeleportType SCPToTeleportType(const ESCP_HitTransformTeleportType InTeleportType)
{
	return InTeleportType == ESCP_HitTransformTeleportType::ResetPhysics
		? ETeleportType::ResetPhysics
		: ETeleportType::None;
}

USTRUCT(BlueprintType)
struct SYNCCOMBATPREDICTION_API FSCP_HitMovementSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	ESCP_HitMoveDirection MoveDirection = ESCP_HitMoveDirection::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement",
		meta=(EditCondition="MoveDirection != ESCP_HitMoveDirection::None", EditConditionHides))
	ESCP_HitTransformTriggerTiming TriggerTiming = ESCP_HitTransformTriggerTiming::OnHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement",
		meta=(EditCondition="MoveDirection != ESCP_HitMoveDirection::None", EditConditionHides))
	ESCP_HitTransformRecipient Recipient = ESCP_HitTransformRecipient::Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement",
		meta=(EditCondition="MoveDirection != ESCP_HitMoveDirection::None", EditConditionHides))
	ESCP_HitTransformReferenceActorSource ReferenceActorSource = ESCP_HitTransformReferenceActorSource::Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement",
		meta=(EditCondition="MoveDirection != ESCP_HitMoveDirection::None && MoveDirection != ESCP_HitMoveDirection::KeepCurrentDistance", EditConditionHides, ClampMin="0.0", Units="cm"))
	float MoveDistance = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement",
		meta=(EditCondition="MoveDirection != ESCP_HitMoveDirection::None", EditConditionHides))
	ESCP_HitLateralOffsetMode LateralOffsetMode = ESCP_HitLateralOffsetMode::KeepCurrent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement",
		meta=(EditCondition="MoveDirection != ESCP_HitMoveDirection::None && LateralOffsetMode != ESCP_HitLateralOffsetMode::KeepCurrent", EditConditionHides, Units="cm"))
	float LateralOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement",
		meta=(EditCondition="MoveDirection != ESCP_HitMoveDirection::None", EditConditionHides))
	bool bSweep = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement",
		meta=(EditCondition="MoveDirection != ESCP_HitMoveDirection::None", EditConditionHides))
	ESCP_HitTransformTeleportType TeleportType = ESCP_HitTransformTeleportType::ResetPhysics;
};

UENUM(BlueprintType)
enum class ESCP_HitRotationDirection : uint8
{
	None UMETA(DisplayName="None"),
	FaceReferenceActor UMETA(DisplayName="Face Reference Actor"),
	FaceAwayFromReference UMETA(DisplayName="Face Away From Reference"),
	FaceOppositeReferenceForward UMETA(DisplayName="Face Opposite Reference Forward"),
	FaceDirection UMETA(DisplayName="Face Direction")
};

USTRUCT(BlueprintType)
struct SYNCCOMBATPREDICTION_API FSCP_HitRotationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rotation")
	ESCP_HitRotationDirection RotationDirection = ESCP_HitRotationDirection::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rotation",
		meta=(EditCondition="RotationDirection != ESCP_HitRotationDirection::None", EditConditionHides))
	ESCP_HitTransformTriggerTiming TriggerTiming = ESCP_HitTransformTriggerTiming::OnHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rotation",
		meta=(EditCondition="RotationDirection != ESCP_HitRotationDirection::None", EditConditionHides))
	ESCP_HitTransformRecipient Recipient = ESCP_HitTransformRecipient::Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rotation",
		meta=(EditCondition="RotationDirection != ESCP_HitRotationDirection::None", EditConditionHides))
	ESCP_HitTransformReferenceActorSource ReferenceActorSource = ESCP_HitTransformReferenceActorSource::Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rotation",
		meta=(EditCondition="RotationDirection == ESCP_HitRotationDirection::FaceDirection", EditConditionHides))
	FRotator DirectionToFace = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rotation",
		meta=(EditCondition="RotationDirection != ESCP_HitRotationDirection::None", EditConditionHides))
	ESCP_HitTransformTeleportType TeleportType = ESCP_HitTransformTeleportType::ResetPhysics;
};

USTRUCT(BlueprintType)
struct SYNCCOMBATPREDICTION_API FSCP_HitTransformSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement", meta=(ShowOnlyInnerProperties))
	FSCP_HitMovementSettings MovementSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rotation", meta=(ShowOnlyInnerProperties))
	FSCP_HitRotationSettings RotationSettings;
};

USTRUCT(BlueprintType)
struct SYNCCOMBATPREDICTION_API FSCP_PredictedHit
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Sync Combat Prediction")
	FSCP_CombatPredictionContext PredictionContext;

	UPROPERTY(BlueprintReadOnly, Category="Sync Combat Prediction")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Sync Combat Prediction")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Sync Combat Prediction")
	FHitResult HitResult;

	UPROPERTY(BlueprintReadOnly, Category="Sync Combat Prediction")
	FGameplayTag ReactionTag;

	UPROPERTY(BlueprintReadOnly, Category="Sync Combat Prediction")
	FSCP_GameplayEffectSettings GameplayEffects;

	UPROPERTY(BlueprintReadOnly, Category="Sync Combat Prediction")
	FSCP_HitTransformSettings TransformSettings;

	UPROPERTY(BlueprintReadOnly, Category="Sync Combat Prediction")
	bool bIsAuthority = false;
};
