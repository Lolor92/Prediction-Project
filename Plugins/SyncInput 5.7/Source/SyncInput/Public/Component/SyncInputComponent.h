#pragma once

#include "CoreMinimal.h"
#include <Components/ActorComponent.h>
#include "Data/SyncInputConfig.h"
#include "GameplayAbilitySpecHandle.h"
#include "InputActionValue.h"
#include "TimerManager.h"
#include "SyncInputComponent.generated.h"

class UAbilitySystemComponent;
class APlayerController;
class UCameraComponent;
class UEnhancedInputComponent;  
class UGameplayAbility;
class USpringArmComponent;
struct FGameplayAbilitySpec;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSyncInputTag, FGameplayTag, InputTag);

struct FSyncInputActiveComboChain
{
	FGameplayAbilitySpecHandle CurrentAbilityHandle;
	TSubclassOf<UGameplayAbility> NextAbilityClass = nullptr;
	FTimerHandle TimerHandle;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), meta = (DisplayName = "ActorComponent (SyncInput)"))
class SYNCINPUT_API USyncInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USyncInputComponent();

	/** Data asset that holds mapping contexts and Action↔Tag pairs */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SyncInput")
	TObjectPtr<USyncInputConfig> InputConfig = nullptr;

	UPROPERTY(BlueprintAssignable, Category="SyncInput|Events")
	FOnSyncInputTag OnSyncInputPressed;

	UPROPERTY(BlueprintAssignable, Category="SyncInput|Events")
	FOnSyncInputTag OnSyncInputReleased;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Held Input")
	bool bRetryHeldInputActivation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Held Input", meta=(ClampMin="0.02", Units="Seconds"))
	float HeldInputRetryInterval = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Held Input")
	bool bConsumeHeldInputAfterActivation = true;

	UPROPERTY(Transient)
	bool bComboSupportAvailable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Combo",
		meta=(EditCondition="bComboSupportAvailable", EditConditionHides))
	bool bEnableComboInputRouting = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Zoom")
	bool bEnableZoom = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Zoom", meta=(ClampMin="0"))
	int32 MinZoomLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Zoom", meta=(ClampMin="0"))
	int32 MaxZoomLevel = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Zoom", meta=(ClampMin="0"))
	int32 ZoomLevel = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Zoom", meta=(ClampMin="0.0"))
	float ZoomStepDistance = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Zoom", meta=(ClampMin="0.0"))
	float ZoomInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Zoom")
	FVector DefaultCameraOffset = FVector(0.f, 0.f, 10.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Zoom")
	FVector ClosestZoomCameraOffset = FVector(50.f, 0.f, 70.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Simulation")
	bool bSimulateMovementInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Simulation", meta=(Categories="SyncInput"))
	TArray<FGameplayTag> SimulatedInputTagPool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Simulation", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SimulatedMovementChance = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Simulation", meta=(ClampMin="0.01", Units="Seconds"))
	float SimulatedInputMinHoldDuration = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Simulation", meta=(ClampMin="0.01", Units="Seconds"))
	float SimulatedInputMaxHoldDuration = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Simulation", meta=(ClampMin="0.0", Units="Seconds"))
	float SimulatedInputMinPauseDuration = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInput|Simulation", meta=(ClampMin="0.0", Units="Seconds"))
	float SimulatedInputMaxPauseDuration = 0.25f;

	UFUNCTION(BlueprintCallable, Category="SyncInput|Simulation")
	void StartInputSimulation(const TArray<FGameplayTag>& InputTags);

	UFUNCTION(BlueprintCallable, Category="SyncInput|Simulation")
	void StopInputSimulation();

	UFUNCTION(BlueprintPure, Category="SyncInput|Simulation")
	bool IsInputSimulationRunning() const { return bInputSimulationRunning; }

	UFUNCTION(BlueprintCallable, Category="SyncInput|Held Input")
	void SuppressHeldInputForAbilityTag(FGameplayTag AbilityOrOwnedTag);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// lifecycle around possession changes
	void HandleNewPawn(class APawn* NewPawn);
	void InstallForPawn(APawn* Pawn);
	void UninstallFromPawn();

	// steps
	void AddMappingContextsForLocalPlayer() const;
	void RemoveMappingContextsForLocalPlayer() const;
	void BindActionsFromConfig();
	bool TryActivateInputTag(FGameplayTag InputTag, bool bForwardPressedToAlreadyActiveSpecs);
	bool DoesSpecMatchInputTag(const FGameplayAbilitySpec& Spec, const FGameplayTag& InputTag) const;
	bool DoesSpecUseGameplayTag(const FGameplayAbilitySpec& Spec, const FGameplayTag& GameplayTag) const;
	bool CanLocallyActivateSpec(const FGameplayAbilitySpec& Spec) const;
	void ForwardInputPressedToSpec(FGameplayAbilitySpec& Spec) const;
	void ForwardInputReleasedToSpec(FGameplayAbilitySpec& Spec) const;
	bool TryActivateComboAbility(const FGameplayAbilitySpec& RequestedAbilitySpec, bool& bOutConsumedInput);
	void UpdateComboChain(const FGameplayAbilitySpecHandle StarterHandle, const FGameplayAbilitySpec& CurrentAbilitySpec);
	void ClearComboChain(FGameplayAbilitySpecHandle StarterHandle);
	void ClearAllComboChains();
	UGameplayAbility* GetComboDataAbilityFromSpec(const FGameplayAbilitySpec& Spec) const;
	TSubclassOf<UGameplayAbility> GetComboAbilityClassFromSpec(const FGameplayAbilitySpec& Spec) const;
	float GetComboWindowDurationFromSpec(const FGameplayAbilitySpec& Spec) const;
	void AddHeldInputTag(FGameplayTag InputTag);
	void RemoveHeldInputTag(FGameplayTag InputTag);
	void RetryHeldInputActivation();
	void ScheduleHeldInputRetry();
	void ClearHeldInputRetry();
	void ScheduleNextSimulatedInput();
	void PressRandomSimulatedInput();
	void ReleaseSimulatedInput();
	void SetSimulatedMovementHeld(bool bHeld);
	void ClearSimulatedInputTimers();
	TArray<FGameplayTag> GetDefaultSimulatedInputTags() const;

	// helpers
	bool IsLocallyControlledOwner() const;
	class APlayerController* GetOwningPlayerController() const;

	// bound handlers
	void HandleActionPressed(FGameplayTag InputTag);
	void HandleActionReleased(FGameplayTag InputTag);
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Zoom(const FInputActionValue& Value);
	void ApplyZoom();
	USpringArmComponent* FindSpringArm() const;
	UCameraComponent* FindCamera() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> InjectedEnhancedInputComponent = nullptr;

	FDelegateHandle NewPawnHandle;

	// (optional) cached ASC – only looked up on first use
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent = nullptr;

	TArray<FGameplayTag> HeldInputTags;
	FTimerHandle HeldInputRetryTimerHandle;
	TMap<FGameplayAbilitySpecHandle, FSyncInputActiveComboChain> ActiveComboChains;

	TWeakObjectPtr<APlayerController> CachedPlayerController;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> CachedSpringArm = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> CachedCamera = nullptr;

	UPROPERTY(Transient)
	float DesiredZoomArmLength = 0.f;

	UPROPERTY(Transient)
	bool bZoomInterpolationActive = false;

	UPROPERTY(Transient)
	bool bInputSimulationRunning = false;

	UPROPERTY(Transient)
	TArray<FGameplayTag> SimulatedInputTags;

	UPROPERTY(Transient)
	FGameplayTag ActiveSimulatedInputTag;

	UPROPERTY(Transient)
	FVector2D ActiveSimulatedMoveInput = FVector2D::ZeroVector;

	FTimerHandle SimulatedInputPressTimerHandle;
	FTimerHandle SimulatedInputReleaseTimerHandle;
};
