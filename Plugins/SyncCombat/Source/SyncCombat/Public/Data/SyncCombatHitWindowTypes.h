// Copyright ProjectLogos

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SyncCombatHitWindowTypes.generated.h"

class UGameplayEffect;

UENUM(BlueprintType)
enum class ESyncCombatHitDetectionShapeType : uint8
{
	Sphere UMETA(DisplayName="Sphere"),
	Capsule UMETA(DisplayName="Capsule"),
	Box UMETA(DisplayName="Box")
};

USTRUCT(BlueprintType)
struct FSyncCombatHitWindowShapeSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Shape")
	ESyncCombatHitDetectionShapeType ShapeType = ESyncCombatHitDetectionShapeType::Capsule;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Shape",
		meta=(EditCondition="ShapeType == ESyncCombatHitDetectionShapeType::Sphere", EditConditionHides, ClampMin="0.0"))
	float SphereRadius = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Shape",
		meta=(EditCondition="ShapeType == ESyncCombatHitDetectionShapeType::Capsule", EditConditionHides, ClampMin="0.0"))
	float CapsuleRadius = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Shape",
		meta=(EditCondition="ShapeType == ESyncCombatHitDetectionShapeType::Capsule", EditConditionHides, ClampMin="0.0"))
	float CapsuleHalfHeight = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Shape",
		meta=(EditCondition="ShapeType == ESyncCombatHitDetectionShapeType::Box", EditConditionHides))
	FVector BoxHalfExtent = FVector(10.f, 10.f, 40.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Shape")
	FVector LocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Shape")
	FRotator LocalRotation = FRotator::ZeroRotator;
};

UENUM(BlueprintType)
enum class ESyncCombatHitWindowMoveDirection : uint8
{
	None UMETA(DisplayName="None"),
	KeepCurrentDistance UMETA(DisplayName="Keep Current Distance"),
	MoveCloser UMETA(DisplayName="Move Closer"),
	MoveAway UMETA(DisplayName="Move Away"),
	SnapToDistance UMETA(DisplayName="Snap To Distance")
};

UENUM(BlueprintType)
enum class ESyncCombatHitWindowLateralOffsetMode : uint8
{
	KeepCurrent UMETA(DisplayName="Keep Current"),
	AddOffset UMETA(DisplayName="Add Offset"),
	SnapToOffset UMETA(DisplayName="Snap To Offset")
};

UENUM(BlueprintType)
enum class ESyncCombatHitWindowTransformTriggerTiming : uint8
{
	OnHit UMETA(DisplayName="On Hit"),
	OnActivation UMETA(DisplayName="On Activation"),
	Both UMETA(DisplayName="Both")
};

UENUM(BlueprintType)
enum class ESyncCombatHitWindowTransformRecipient : uint8
{
	Instigator UMETA(DisplayName="Instigator"),
	Target UMETA(DisplayName="Target"),
	Both UMETA(DisplayName="Both")
};

UENUM(BlueprintType)
enum class ESyncCombatHitWindowReferenceActorSource : uint8
{
	Instigator UMETA(DisplayName="Instigator"),
	Target UMETA(DisplayName="Target"),
	LastCombatReferenceActor UMETA(DisplayName="Last Combat Reference Actor")
};

UENUM(BlueprintType)
enum class ESyncCombatHitWindowTeleportType : uint8
{
	None UMETA(DisplayName="None"),
	ResetPhysics UMETA(DisplayName="Reset Physics")
};

static FORCEINLINE ETeleportType ToTeleportType(const ESyncCombatHitWindowTeleportType InTeleportType)
{
	return InTeleportType == ESyncCombatHitWindowTeleportType::ResetPhysics
		? ETeleportType::ResetPhysics
		: ETeleportType::None;
}

USTRUCT(BlueprintType)
struct FSyncCombatHitWindowMovementSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Movement")
	ESyncCombatHitWindowMoveDirection MoveDirection = ESyncCombatHitWindowMoveDirection::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Movement",
		meta=(EditCondition="MoveDirection != ESyncCombatHitWindowMoveDirection::None", EditConditionHides))
	ESyncCombatHitWindowTransformTriggerTiming TriggerTiming = ESyncCombatHitWindowTransformTriggerTiming::OnHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Movement",
		meta=(EditCondition="MoveDirection != ESyncCombatHitWindowMoveDirection::None", EditConditionHides))
	ESyncCombatHitWindowTransformRecipient Recipient = ESyncCombatHitWindowTransformRecipient::Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Movement",
		meta=(EditCondition="MoveDirection != ESyncCombatHitWindowMoveDirection::None", EditConditionHides))
	ESyncCombatHitWindowReferenceActorSource ReferenceActorSource = ESyncCombatHitWindowReferenceActorSource::Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Movement",
		meta=(EditCondition="MoveDirection != ESyncCombatHitWindowMoveDirection::None && MoveDirection != ESyncCombatHitWindowMoveDirection::KeepCurrentDistance", EditConditionHides, ClampMin="0.0"))
	float MoveDistance = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Movement",
		meta=(EditCondition="MoveDirection != ESyncCombatHitWindowMoveDirection::None", EditConditionHides))
	ESyncCombatHitWindowLateralOffsetMode LateralOffsetMode = ESyncCombatHitWindowLateralOffsetMode::KeepCurrent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Movement",
		meta=(EditCondition="MoveDirection != ESyncCombatHitWindowMoveDirection::None && LateralOffsetMode != ESyncCombatHitWindowLateralOffsetMode::KeepCurrent", EditConditionHides))
	float LateralOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Movement",
		meta=(EditCondition="MoveDirection != ESyncCombatHitWindowMoveDirection::None", EditConditionHides))
	bool bSweep = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Movement",
		meta=(EditCondition="MoveDirection != ESyncCombatHitWindowMoveDirection::None", EditConditionHides))
	ESyncCombatHitWindowTeleportType TeleportType = ESyncCombatHitWindowTeleportType::ResetPhysics;
};

UENUM(BlueprintType)
enum class ESyncCombatHitWindowRotationDirection : uint8
{
	None UMETA(DisplayName="None"),
	FaceToFace UMETA(DisplayName="Face Reference Actor"),
	FaceAway UMETA(DisplayName="Face Away From Reference"),
	FaceOppositeInstigatorForward UMETA(DisplayName="Face Opposite Reference Forward"),
	FaceDirection UMETA(DisplayName="Face Direction")
};

USTRUCT(BlueprintType)
struct FSyncCombatHitWindowRotationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Rotation")
	ESyncCombatHitWindowRotationDirection RotationDirection = ESyncCombatHitWindowRotationDirection::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Rotation",
		meta=(EditCondition="RotationDirection != ESyncCombatHitWindowRotationDirection::None", EditConditionHides))
	ESyncCombatHitWindowTransformTriggerTiming TriggerTiming = ESyncCombatHitWindowTransformTriggerTiming::OnHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Rotation",
		meta=(EditCondition="RotationDirection != ESyncCombatHitWindowRotationDirection::None", EditConditionHides))
	ESyncCombatHitWindowTransformRecipient Recipient = ESyncCombatHitWindowTransformRecipient::Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Rotation",
		meta=(EditCondition="RotationDirection != ESyncCombatHitWindowRotationDirection::None", EditConditionHides))
	ESyncCombatHitWindowReferenceActorSource ReferenceActorSource = ESyncCombatHitWindowReferenceActorSource::Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Rotation",
		meta=(EditCondition="RotationDirection == ESyncCombatHitWindowRotationDirection::FaceDirection", EditConditionHides))
	FRotator DirectionToFace = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Rotation",
		meta=(EditCondition="RotationDirection != ESyncCombatHitWindowRotationDirection::None", EditConditionHides))
	ESyncCombatHitWindowTeleportType TeleportType = ESyncCombatHitWindowTeleportType::ResetPhysics;
};

USTRUCT(BlueprintType)
struct FSyncCombatHitWindowBlockSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Block")
	bool bBlockable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Block",
		meta=(EditCondition="bBlockable", EditConditionHides, ClampMin="0.0", ClampMax="180.0"))
	float BlockAngleDegrees = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Block",
		meta=(EditCondition="bBlockable", EditConditionHides))
	bool bAllowMovementWhenBlocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Block",
		meta=(EditCondition="bBlockable", EditConditionHides))
	bool bAllowRotationWhenBlocked = false;
};

USTRUCT(BlueprintType)
struct FSyncCombatHitWindowDodgeSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Dodge")
	bool bDodgeable = false;
};

UENUM(BlueprintType)
enum class ESyncCombatHitWindowSuperArmorLevel : uint8
{
	None UMETA(DisplayName="None"),
	SuperArmor1 UMETA(DisplayName="Super Armor 1"),
	SuperArmor2 UMETA(DisplayName="Super Armor 2"),
	SuperArmor3 UMETA(DisplayName="Super Armor 3")
};

USTRUCT(BlueprintType)
struct FSyncCombatHitWindowDefenseSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Defense",
		meta=(ShowOnlyInnerProperties))
	FSyncCombatHitWindowBlockSettings BlockSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Defense",
		meta=(ShowOnlyInnerProperties))
	FSyncCombatHitWindowDodgeSettings DodgeSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Defense")
	ESyncCombatHitWindowSuperArmorLevel RequiredSuperArmor = ESyncCombatHitWindowSuperArmorLevel::None;
};

USTRUCT(BlueprintType)
struct FSyncCombatHitWindowDebugSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Debug")
	bool bDrawDebugTrace = false;
};

UENUM(BlueprintType)
enum class ESyncCombatHitWindowCueSpawnPoint : uint8
{
	OwnerLocation UMETA(DisplayName="Owner Location"),
	HitImpactPoint UMETA(DisplayName="Hit Impact Point"),
	HitLocation UMETA(DisplayName="Hit Location")
};

UENUM(BlueprintType)
enum class ESyncCombatHitWindowCueRecipient : uint8
{
	Instigator UMETA(DisplayName="Instigator"),
	Target UMETA(DisplayName="Target"),
	Both UMETA(DisplayName="Both")
};

UENUM(BlueprintType)
enum class ESyncCombatHitWindowCueTriggerTiming : uint8
{
	OnActivation UMETA(DisplayName="On Activation"),
	OnHit UMETA(DisplayName="On Hit")
};

USTRUCT(BlueprintType)
struct FSyncCombatHitWindowGameplayEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Effects")
	TSubclassOf<UGameplayEffect> GameplayEffectClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Effects", meta=(ClampMin="0.0"))
	float EffectLevel = 1.f;
};

USTRUCT(BlueprintType)
struct FSyncCombatHitWindowDelayedGameplayEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Delayed Effects")
	TSubclassOf<UGameplayEffect> GameplayEffectClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Delayed Effects", meta=(ClampMin="0.0"))
	float EffectLevel = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Delayed Effects", meta=(ClampMin="0.0", UIMin="0.0", Units="Seconds"))
	float DelaySeconds = 0.1f;
};

USTRUCT(BlueprintType)
struct FSyncCombatHitWindowDamageSettings
{
	GENERATED_BODY()

	// Damage-only effects. Put GE_Damage, GE_Damage_Light, GE_Damage_Heavy, etc. here.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Damage", meta=(TitleProperty="GameplayEffectClass"))
	TArray<FSyncCombatHitWindowGameplayEffect> DamageGameplayEffectsToApply;

	// Normal fighting-game style defaults:
	// Block / parry / dodge stop damage.
	// Super armor can still take damage while ignoring reaction.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Damage")
	bool bApplyDamageWhenBlocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Damage")
	bool bApplyDamageWhenParried = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Damage")
	bool bApplyDamageWhenDodged = false;

	// Damage is allowed through super armor only if the defender's super armor level
	// is below or equal to this value.
	// Example:
	// SuperArmor1 / SuperArmor2 take damage, SuperArmor3 ignores damage.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Damage")
	ESyncCombatHitWindowSuperArmorLevel MaxSuperArmorLevelThatTakesDamage = ESyncCombatHitWindowSuperArmorLevel::SuperArmor2;
};

USTRUCT(BlueprintType)
struct FSyncCombatHitWindowGameplayCue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Gameplay Cues",
		meta=(Categories="GameplayCue", DisplayName="Gameplay Cue Tag"))
	FGameplayTag CueTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Gameplay Cues")
	ESyncCombatHitWindowCueSpawnPoint SpawnPoint = ESyncCombatHitWindowCueSpawnPoint::HitImpactPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Gameplay Cues")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Gameplay Cues")
	ESyncCombatHitWindowCueTriggerTiming TriggerTiming = ESyncCombatHitWindowCueTriggerTiming::OnHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Gameplay Cues")
	bool bAttachToTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Gameplay Cues")
	ESyncCombatHitWindowCueRecipient Recipient = ESyncCombatHitWindowCueRecipient::Target;

	bool HasValidCueTag() const
	{
		return CueTag.IsValid();
	}
};

USTRUCT(BlueprintType)
struct FSyncCombatHitWindowSettings
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Debug",
		meta=(ShowOnlyInnerProperties))
	FSyncCombatHitWindowDebugSettings DebugSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Shape",
		meta=(ShowOnlyInnerProperties))
	FSyncCombatHitWindowShapeSettings ShapeSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Movement",
		meta=(ShowOnlyInnerProperties))
	FSyncCombatHitWindowMovementSettings MovementSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Rotation",
		meta=(ShowOnlyInnerProperties))
	FSyncCombatHitWindowRotationSettings RotationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Defense",
		meta=(ShowOnlyInnerProperties))
	FSyncCombatHitWindowDefenseSettings DefenseSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Damage",
		meta=(ShowOnlyInnerProperties))
	FSyncCombatHitWindowDamageSettings DamageSettings;

	// Keep the variable name for asset compatibility.
	// Treat this as reaction/stagger/knockdown/pushback effects, not damage.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Effects",
		meta=(TitleProperty="GameplayEffectClass", DisplayName="Reaction Gameplay Effects to Apply"))
	TArray<FSyncCombatHitWindowGameplayEffect> GameplayEffectsToApply;

	// Keep GameplayEffectsToApply as your immediate reaction effects.
	// Stagger / knockdown / pushback trigger effects go there.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Effects",
		meta=(TitleProperty="GameplayEffectClass", DisplayName="Delayed Reaction Gameplay Effects to Apply"))
	TArray<FSyncCombatHitWindowDelayedGameplayEffect> DelayedGameplayEffectsToApply;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Detection|Gameplay Cues",
		meta=(TitleProperty="CueTag"))
	TArray<FSyncCombatHitWindowGameplayCue> GameplayCuesToExecute;
};
