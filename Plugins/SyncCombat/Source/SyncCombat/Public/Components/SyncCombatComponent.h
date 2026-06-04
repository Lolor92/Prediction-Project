// Copyright ProjectLogos

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Data/SyncCombatHitWindowTypes.h"
#include "Data/SyncCombatAbilitySet.h"
#include "Data/SyncCombatTagReactionData.h"
#include "Runtime/SyncCombatHitWindowRuntime.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "UObject/ObjectKey.h"
#include "SyncCombatComponent.generated.h"

class AActor;
class ACharacter;
struct FAnimNotifyEvent;
class UAbilitySystemComponent;
class UAnimSequenceBase;
class UAnimMontage;
class UGameplayEffect;
class FBoolProperty;
class UAnimNotifyState;
class USkeletalMeshComponent;
class FSyncCombatTagReactionRuntime;

// Drives an AnimInstance bool from one or more gameplay tags.
USTRUCT(BlueprintType)
struct FSyncCombatAnimBoolBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim Bool")
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim Bool")
	FName AnimBoolName;

	FBoolProperty* CachedBoolProperty = nullptr;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName="SyncCombatComponent"))
class SYNCCOMBAT_API USyncCombatComponent : public UActorComponent
{
	GENERATED_BODY()
	friend class FSyncCombatHitWindowRuntime;

public:
	USyncCombatComponent(FVTableHelper& Helper);
	USyncCombatComponent();
	virtual ~USyncCombatComponent() override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void InitializeCombat(AActor* InOwnerActor, UAbilitySystemComponent* InAbilitySystemComponent);
	void DeinitializeCombat();
	void HandleMovementModeChanged(EMovementMode NewMovementMode);
	bool IsBlockingActive() const;
	bool IsParryingActive() const;
	UFUNCTION(BlueprintPure, Category="Combat|Crowd Control")
	bool IsCrowdControlActive() const;
	UFUNCTION(BlueprintPure, Category="Combat|Defense")
	ESyncCombatHitWindowSuperArmorLevel GetCurrentSuperArmorLevel() const;
	void SetLastCombatReferenceActor(AActor* InActor);
	const FGameplayTag& GetBlockingTag() const { return BlockingTag; }
	const FGameplayTag& GetParryingTag() const { return ParryingTag; }
	AActor* GetOwningActor() const { return OwningActor; }
	UAbilitySystemComponent* GetAbilitySystemComponent() const { return AbilitySystemComponent; }
	USyncCombatTagReactionData* GetTagReactionData() const { return TagReactionData; }
	const FSyncCombatTagReactionBinding* FindTagReactionBindingForTriggerTag(const FGameplayTag& TriggerTag) const;
	bool FindReactionAbilityTag(const FGameplayTag& TriggerTag, FGameplayTag& OutAbilityTag) const;

	bool BeginHitDetectionWindow(const UAnimNotifyState* NotifyState, USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation, const FAnimNotifyEvent* NotifyEvent, float TotalDuration, FName TraceSocketName,
		const FSyncCombatHitWindowSettings& HitWindowSettings);

	void EndHitDetectionWindow(const UAnimNotifyState* NotifyState, USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability|Initialization")
	bool bAutoInitializeCombat = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability|Initialization", meta=(ClampMin="0.01", Units="Seconds"))
	float AutoInitializeRetryInterval = 0.25f;

	// Default abilities granted on authority.
	UPROPERTY(EditDefaultsOnly, Category="Ability")
	TArray<TObjectPtr<USyncCombatAbilitySet>> DefaultAbilitySets;

	// Gameplay tag driven reactions.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tag Reactions")
	TObjectPtr<USyncCombatTagReactionData> TagReactionData = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tag Reactions", meta=(TitleProperty="AnimBoolName"))
	TArray<FSyncCombatAnimBoolBinding> AnimBoolBindings;

	// Input lock tag.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd Control")
	FGameplayTag CrowdControlTag;

	// Applied while falling.
	UPROPERTY(EditDefaultsOnly, Category="Effects")
	TSubclassOf<UGameplayEffect> AirborneEffectClass;

	// Tag required on a defender for hits to be considered blockable.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Block")
	FGameplayTag BlockingTag;

	// Tag required on a defender for hits to be considered parrying.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Parry")
	FGameplayTag ParryingTag;

	// Tag required on a defender for hits to be considered dodgeable.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dodge")
	FGameplayTag DodgingTag;

	// Tags that mark the owner as having super armor at each level.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Super Armor")
	FGameplayTag SuperArmorTag1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Super Armor")
	FGameplayTag SuperArmorTag2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Super Armor")
	FGameplayTag SuperArmorTag3;

	// Applied when this component's owner lands a hit that gets blocked.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Block")
	TSubclassOf<UGameplayEffect> AttackerBlockedEffectClass;

	// Applied when this component's owner successfully blocks an incoming hit.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Block")
	TSubclassOf<UGameplayEffect> DefenderBlockedEffectClass;

	// Applied when this component's owner lands a hit that gets parried.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Parry")
	TSubclassOf<UGameplayEffect> AttackerParriedEffectClass;

	// Applied when this component's owner successfully parries an incoming hit.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Parry")
	TSubclassOf<UGameplayEffect> DefenderParrySuccessEffectClass;

	// Applied when this component's owner lands a hit that gets dodged.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dodge")
	TSubclassOf<UGameplayEffect> AttackerDodgedEffectClass;

	// Applied when this component's owner successfully dodges an incoming hit.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dodge")
	TSubclassOf<UGameplayEffect> DefenderDodgedEffectClass;

	// Applied when this component's owner lands a hit that is resisted by super armor.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Super Armor")
	TSubclassOf<UGameplayEffect> AttackerSuperArmoredEffectClass;

	// Applied when this component's owner successfully resists a hit via super armor.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Super Armor")
	TSubclassOf<UGameplayEffect> DefenderSuperArmoredEffectClass;

private:
	// Ability setup.
	void GrantDefaultAbilities();
	void ClearDefaultAbilities();

	// Crowd control.
	void BindCrowdControlTagEvent();
	void ClearCrowdControlTagEvent();
	void OnCrowdControlTagChanged(const FGameplayTag Tag, int32 NewCount);

	// Gameplay effects.
	FActiveGameplayEffectHandle ApplyEffectToSelf(const TSubclassOf<UGameplayEffect>& GameplayEffectClass, float Level) const;

	void RemoveGameplayEffect(FActiveGameplayEffectHandle& EffectHandle);
	void BindMovementModeChanged();
	void ClearMovementModeChanged();
	UFUNCTION()
	void OnCharacterMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);
	UFUNCTION(Client, Unreliable)
	void ClientApplyHitWindowTransformPrediction(bool bApplyRotation, FRotator Rotation,
		ESyncCombatHitWindowTeleportType RotationTeleportType, bool bApplyLocation, FVector Location,
		bool bSweepLocation, ESyncCombatHitWindowTeleportType LocationTeleportType);
	void TryAutoInitializeCombat();
	void ScheduleAutoInitializeRetry();
	void ClearAutoInitializeRetry();

	// Cached owner references.
	UPROPERTY()
	TObjectPtr<AActor> OwningActor = nullptr;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent = nullptr;

	// Granted default abilities.
	FSyncCombatAbilitySetGrantedHandles DefaultAbilityHandles;

	// Crowd control binding state.
	FGameplayTag BoundCrowdControlTag;
	FDelegateHandle CrowdControlTagDelegateHandle;

	// Airborne effect state.
	FActiveGameplayEffectHandle AirborneEffectHandle;
	bool bAirborneEffectApplied = false;
	bool bMovementModeChangedBound = false;

	FTimerHandle AutoInitializeRetryTimerHandle;

	FSyncCombatTagReactionRuntime* TagReactionRuntime = nullptr;
	bool HasSuperArmorAtOrAbove(ESyncCombatHitWindowSuperArmorLevel RequiredSuperArmor) const;
	FSyncCombatHitWindowRuntime HitWindowRuntime;
};
