#include "Component/SyncInputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "AbilitySystemComponent.h"
#include "FunctionLibrary/SyncInputFunctionLibrary.h"
#include "Tag/SyncInputTags.h"
#include "InputActionValue.h"
#include "Abilities/GameplayAbility.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "Modules/ModuleManager.h"

namespace SyncInputTags
{
	static const FGameplayTag& Move()
	{
		static FGameplayTag T = FGameplayTag::RequestGameplayTag(TEXT("SyncInput.Move"));
		return T;
	}
	static const FGameplayTag& Look()
	{
		static FGameplayTag T = FGameplayTag::RequestGameplayTag(TEXT("SyncInput.Look"));
		return T;
	}
	static const FGameplayTag& Zoom()
	{
		static FGameplayTag T = FGameplayTag::RequestGameplayTag(TEXT("SyncInput.Zoom"));
		return T;
	}
}

USyncInputComponent::USyncInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bComboSupportAvailable = FModuleManager::Get().ModuleExists(TEXT("SyncAbilityMotion"));
}

void USyncInputComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocallyControlledOwner()) return;

	if (APlayerController* PC = GetOwningPlayerController())
	{
		NewPawnHandle = PC->GetOnNewPawnNotifier().AddUObject(this, &USyncInputComponent::HandleNewPawn);
		HandleNewPawn(PC->GetPawn());
	}
}

void USyncInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PC = GetOwningPlayerController())
	{
		if (NewPawnHandle.IsValid())
		{
			PC->GetOnNewPawnNotifier().Remove(NewPawnHandle);
			NewPawnHandle.Reset();
		}
	}

	UninstallFromPawn();
	Super::EndPlay(EndPlayReason);
}

void USyncInputComponent::HandleNewPawn(APawn* NewPawn)
{
	UninstallFromPawn();
	if (!NewPawn || !IsLocallyControlledOwner()) return;

	InstallForPawn(NewPawn);
}

void USyncInputComponent::InstallForPawn(APawn* Pawn)
{
	CachedPlayerController = GetOwningPlayerController();
	CachedSpringArm = Pawn ? Pawn->FindComponentByClass<USpringArmComponent>() : nullptr;
	CachedCamera = Pawn ? Pawn->FindComponentByClass<UCameraComponent>() : nullptr;
	if (MaxZoomLevel >= MinZoomLevel)
	{
		ZoomLevel = FMath::Clamp(ZoomLevel, MinZoomLevel, MaxZoomLevel);
	}
	DesiredZoomArmLength = ZoomLevel * ZoomStepDistance;

	AddMappingContextsForLocalPlayer();

	if (APlayerController* PC = CachedPlayerController.Get())
	{
		if (!InjectedEnhancedInputComponent)
		{
			InjectedEnhancedInputComponent = NewObject<UEnhancedInputComponent>(
				PC, UEnhancedInputComponent::StaticClass(), TEXT("SyncInput_InjectedInput"));
			InjectedEnhancedInputComponent->RegisterComponent();
			PC->PushInputComponent(InjectedEnhancedInputComponent);
		}
	}

	BindActionsFromConfig();
}

void USyncInputComponent::UninstallFromPawn()
{
	HeldInputTags.Reset();
	ClearHeldInputRetry();
	ClearAllComboChains();
	RemoveMappingContextsForLocalPlayer();
	CachedSpringArm = nullptr;
	CachedCamera = nullptr;
	DesiredZoomArmLength = 0.f;
	SetComponentTickEnabled(false);

	if (APlayerController* PC = CachedPlayerController.Get())
	{
		if (InjectedEnhancedInputComponent)
		{
			PC->PopInputComponent(InjectedEnhancedInputComponent);
			InjectedEnhancedInputComponent->DestroyComponent();
			InjectedEnhancedInputComponent = nullptr;
		}
	}
	CachedPlayerController = nullptr;
}

void USyncInputComponent::AddMappingContextsForLocalPlayer() const
{
	if (!InputConfig) { UE_LOG(LogTemp, Warning, TEXT("SyncInput: InputConfig is null.")); return; }

	const APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	if (const ULocalPlayer* LP = PC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
		{
			for (const FSyncInputMappingContextEntry& E : InputConfig->MappingContexts)
			{
				if (E.InputMappingContext)
				{
					Subsys->AddMappingContext(E.InputMappingContext, E.Priority);
				}
			}
		}
	}
}

void USyncInputComponent::RemoveMappingContextsForLocalPlayer() const
{
	if (!InputConfig) return;

	const APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	if (const ULocalPlayer* LP = PC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
		{
			for (const FSyncInputMappingContextEntry& E : InputConfig->MappingContexts)
			{
				if (E.InputMappingContext)
				{
					Subsys->RemoveMappingContext(E.InputMappingContext);
				}
			}
		}
	}
}

void USyncInputComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	USpringArmComponent* SpringArm = FindSpringArm();
	if (!SpringArm)
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (ZoomInterpSpeed <= 0.f)
	{
		SpringArm->TargetArmLength = DesiredZoomArmLength;
		SetComponentTickEnabled(false);
		return;
	}

	SpringArm->TargetArmLength = FMath::FInterpTo(
		SpringArm->TargetArmLength,
		DesiredZoomArmLength,
		DeltaTime,
		ZoomInterpSpeed);

	if (FMath::IsNearlyEqual(SpringArm->TargetArmLength, DesiredZoomArmLength, 1.f))
	{
		SpringArm->TargetArmLength = DesiredZoomArmLength;
		SetComponentTickEnabled(false);
	}
}

void USyncInputComponent::BindActionsFromConfig()
{
	if (!InjectedEnhancedInputComponent) return;
	if (!InputConfig) return;

	if (InputConfig->SyncInputActions.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("SyncInput: SyncInputActions is empty on %s"), *GetNameSafe(InputConfig));
	}

	for (const FSyncInputAction& Row : InputConfig->SyncInputActions)
	{
		if (!Row.InputAction || !Row.InputTag.IsValid()) continue;

		// Special-case Move/Look: bind to axis handlers
		if (Row.InputTag.MatchesTagExact(SyncInputTags::Move()))
		{
			InjectedEnhancedInputComponent->BindAction(
				Row.InputAction, ETriggerEvent::Triggered, this, &USyncInputComponent::Move);
			continue;
		}
		if (Row.InputTag.MatchesTagExact(SyncInputTags::Look()))
		{
			InjectedEnhancedInputComponent->BindAction(
				Row.InputAction, ETriggerEvent::Triggered, this, &USyncInputComponent::Look);
			continue;
		}
		if (Row.InputTag.MatchesTagExact(SyncInputTags::Zoom()))
		{
			InjectedEnhancedInputComponent->BindAction(
				Row.InputAction, ETriggerEvent::Triggered, this, &USyncInputComponent::Zoom);
			continue;
		}

		// Everything else forwards to GAS via the tag
		InjectedEnhancedInputComponent->BindAction(
			Row.InputAction, ETriggerEvent::Started,
			this, &USyncInputComponent::HandleActionPressed, Row.InputTag);

		InjectedEnhancedInputComponent->BindAction(
			Row.InputAction, ETriggerEvent::Completed,
			this, &USyncInputComponent::HandleActionReleased, Row.InputTag);

		InjectedEnhancedInputComponent->BindAction(
			Row.InputAction, ETriggerEvent::Canceled,
			this, &USyncInputComponent::HandleActionReleased, Row.InputTag);
	}
}

bool USyncInputComponent::IsLocallyControlledOwner() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	return PC && PC->IsLocalController();
}

APlayerController* USyncInputComponent::GetOwningPlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
}

void USyncInputComponent::HandleActionPressed(FGameplayTag InputTag)
{
	OnSyncInputPressed.Broadcast(InputTag);

	AddHeldInputTag(InputTag);
	const bool bActivated = TryActivateInputTag(InputTag, true);
	if (bActivated && bConsumeHeldInputAfterActivation)
	{
		RemoveHeldInputTag(InputTag);
	}

	ScheduleHeldInputRetry();
}

void USyncInputComponent::HandleActionReleased(FGameplayTag InputTag)
{
	OnSyncInputReleased.Broadcast(InputTag);

	RemoveHeldInputTag(InputTag);
	if (HeldInputTags.IsEmpty())
	{
		ClearHeldInputRetry();
	}

	if (!AbilitySystemComponent)
	{
		AbilitySystemComponent = USyncInputFunctionLibrary::GetAbilitySystemComponent(GetOwner());
	}
	if (!AbilitySystemComponent) return;

	for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (DoesSpecMatchInputTag(Spec, InputTag) && Spec.IsActive())
		{
			ForwardInputReleasedToSpec(Spec);
		}
	}
}

bool USyncInputComponent::TryActivateInputTag(FGameplayTag InputTag, bool bForwardPressedToAlreadyActiveSpecs)
{
	if (!InputTag.IsValid()) return false;

	if (!AbilitySystemComponent)
	{
		AbilitySystemComponent = USyncInputFunctionLibrary::GetAbilitySystemComponent(GetOwner());
	}
	if (!AbilitySystemComponent) return false;

	bool bActivated = false;
	for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (!DoesSpecMatchInputTag(Spec, InputTag)) continue;

		bool bComboConsumedInput = false;
		if (TryActivateComboAbility(Spec, bComboConsumedInput))
		{
			bActivated = true;
			break;
		}

		if (bComboConsumedInput)
		{
			continue;
		}

		if (Spec.IsActive())
		{
			if (bForwardPressedToAlreadyActiveSpecs)
			{
				ForwardInputPressedToSpec(Spec);
			}

			continue;
		}

		if (!CanLocallyActivateSpec(Spec)) continue;

		if (AbilitySystemComponent->TryActivateAbility(Spec.Handle))
		{
			ForwardInputPressedToSpec(Spec);
			UpdateComboChain(Spec.Handle, Spec);
			bActivated = true;
			break;
		}
	}

	return bActivated;
}

bool USyncInputComponent::DoesSpecMatchInputTag(const FGameplayAbilitySpec& Spec, const FGameplayTag& InputTag) const
{
	return InputTag.IsValid() &&
		(Spec.GetDynamicSpecSourceTags().HasTag(InputTag) ||
			(Spec.Ability && Spec.Ability->GetAssetTags().HasTag(InputTag)));
}

bool USyncInputComponent::CanLocallyActivateSpec(const FGameplayAbilitySpec& Spec) const
{
	if (!AbilitySystemComponent || !Spec.Ability) return false;

	const FGameplayAbilityActorInfo* ActorInfo = AbilitySystemComponent->AbilityActorInfo.Get();
	if (!ActorInfo) return false;

	return Spec.Ability->CanActivateAbility(Spec.Handle, ActorInfo);
}

void USyncInputComponent::ForwardInputPressedToSpec(FGameplayAbilitySpec& Spec) const
{
	if (!AbilitySystemComponent) return;

	AbilitySystemComponent->AbilitySpecInputPressed(Spec);

	FPredictionKey PredictionKey;
	if (UGameplayAbility* PrimaryInstance = Spec.GetPrimaryInstance())
	{
		PredictionKey = PrimaryInstance->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	AbilitySystemComponent->InvokeReplicatedEvent(
		EAbilityGenericReplicatedEvent::InputPressed,
		Spec.Handle,
		PredictionKey);
}

void USyncInputComponent::ForwardInputReleasedToSpec(FGameplayAbilitySpec& Spec) const
{
	if (!AbilitySystemComponent) return;

	AbilitySystemComponent->AbilitySpecInputReleased(Spec);

	FPredictionKey PredictionKey;
	if (UGameplayAbility* PrimaryInstance = Spec.GetPrimaryInstance())
	{
		PredictionKey = PrimaryInstance->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	AbilitySystemComponent->InvokeReplicatedEvent(
		EAbilityGenericReplicatedEvent::InputReleased,
		Spec.Handle,
		PredictionKey);
}

bool USyncInputComponent::TryActivateComboAbility(
	const FGameplayAbilitySpec& RequestedAbilitySpec,
	bool& bOutConsumedInput)
{
	bOutConsumedInput = false;

	if (!bComboSupportAvailable || !bEnableComboInputRouting || !AbilitySystemComponent)
	{
		return false;
	}

	FSyncInputActiveComboChain* ComboChain = ActiveComboChains.Find(RequestedAbilitySpec.Handle);
	if (!ComboChain || !ComboChain->NextAbilityClass)
	{
		return false;
	}

	bOutConsumedInput = true;

	for (FGameplayAbilitySpec& ComboSpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (!ComboSpec.Ability || !ComboSpec.Ability->GetClass()->IsChildOf(ComboChain->NextAbilityClass)) continue;
		if (!CanLocallyActivateSpec(ComboSpec)) return false;

		if (AbilitySystemComponent->TryActivateAbility(ComboSpec.Handle))
		{
			ForwardInputPressedToSpec(ComboSpec);
			UpdateComboChain(RequestedAbilitySpec.Handle, ComboSpec);
			return true;
		}

		return false;
	}

	ClearComboChain(RequestedAbilitySpec.Handle);
	return false;
}

void USyncInputComponent::UpdateComboChain(
	const FGameplayAbilitySpecHandle StarterHandle,
	const FGameplayAbilitySpec& CurrentAbilitySpec)
{
	if (!bComboSupportAvailable || !bEnableComboInputRouting)
	{
		ClearComboChain(StarterHandle);
		return;
	}

	const TSubclassOf<UGameplayAbility> NextAbilityClass = GetComboAbilityClassFromSpec(CurrentAbilitySpec);
	const float ComboWindowDuration = GetComboWindowDurationFromSpec(CurrentAbilitySpec);
	if (!NextAbilityClass || ComboWindowDuration <= 0.f)
	{
		ClearComboChain(StarterHandle);
		return;
	}

	FSyncInputActiveComboChain& ComboChain = ActiveComboChains.FindOrAdd(StarterHandle);
	ComboChain.CurrentAbilityHandle = CurrentAbilitySpec.Handle;
	ComboChain.NextAbilityClass = NextAbilityClass;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ComboChain.TimerHandle);

		FTimerDelegate TimerDelegate =
			FTimerDelegate::CreateUObject(this, &ThisClass::ClearComboChain, StarterHandle);

		World->GetTimerManager().SetTimer(
			ComboChain.TimerHandle,
			TimerDelegate,
			ComboWindowDuration,
			false);
	}
}

void USyncInputComponent::ClearComboChain(FGameplayAbilitySpecHandle StarterHandle)
{
	if (FSyncInputActiveComboChain* ComboChain = ActiveComboChains.Find(StarterHandle))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ComboChain->TimerHandle);
		}
	}

	ActiveComboChains.Remove(StarterHandle);
}

void USyncInputComponent::ClearAllComboChains()
{
	UWorld* World = GetWorld();
	for (TPair<FGameplayAbilitySpecHandle, FSyncInputActiveComboChain>& ComboChainPair : ActiveComboChains)
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(ComboChainPair.Value.TimerHandle);
		}
	}

	ActiveComboChains.Reset();
}

UGameplayAbility* USyncInputComponent::GetComboDataAbilityFromSpec(const FGameplayAbilitySpec& Spec) const
{
	if (UGameplayAbility* PrimaryInstance = Spec.GetPrimaryInstance())
	{
		return PrimaryInstance;
	}

	return Spec.Ability;
}

TSubclassOf<UGameplayAbility> USyncInputComponent::GetComboAbilityClassFromSpec(
	const FGameplayAbilitySpec& Spec) const
{
	UGameplayAbility* Ability = GetComboDataAbilityFromSpec(Spec);
	if (!Ability) return nullptr;

	UFunction* Function = Ability->FindFunction(TEXT("GetComboAbilityClass"));
	if (!Function) return nullptr;

	struct FGetComboAbilityClassParams
	{
		TSubclassOf<UGameplayAbility> ReturnValue = nullptr;
	};

	FGetComboAbilityClassParams Params;
	Ability->ProcessEvent(Function, &Params);
	return Params.ReturnValue;
}

float USyncInputComponent::GetComboWindowDurationFromSpec(const FGameplayAbilitySpec& Spec) const
{
	UGameplayAbility* Ability = GetComboDataAbilityFromSpec(Spec);
	if (!Ability) return 0.f;

	UFunction* Function = Ability->FindFunction(TEXT("GetComboWindowDuration"));
	if (!Function) return 0.f;

	struct FGetComboWindowDurationParams
	{
		float ReturnValue = 0.f;
	};

	FGetComboWindowDurationParams Params;
	Ability->ProcessEvent(Function, &Params);
	return Params.ReturnValue;
}

void USyncInputComponent::AddHeldInputTag(FGameplayTag InputTag)
{
	if (!InputTag.IsValid()) return;

	HeldInputTags.Remove(InputTag);
	HeldInputTags.Add(InputTag);
}

void USyncInputComponent::RemoveHeldInputTag(FGameplayTag InputTag)
{
	HeldInputTags.Remove(InputTag);
}

void USyncInputComponent::RetryHeldInputActivation()
{
	if (!bRetryHeldInputActivation || HeldInputTags.IsEmpty())
	{
		ClearHeldInputRetry();
		return;
	}

	for (int32 Index = HeldInputTags.Num() - 1; Index >= 0; --Index)
	{
		if (!HeldInputTags[Index].IsValid())
		{
			HeldInputTags.RemoveAtSwap(Index);
			continue;
		}

		if (TryActivateInputTag(HeldInputTags[Index], false))
		{
			if (bConsumeHeldInputAfterActivation)
			{
				HeldInputTags.RemoveAtSwap(Index);
			}

			break;
		}
	}

	if (HeldInputTags.IsEmpty())
	{
		ClearHeldInputRetry();
	}
}

void USyncInputComponent::ScheduleHeldInputRetry()
{
	if (!bRetryHeldInputActivation || HeldInputTags.IsEmpty() || HeldInputRetryTimerHandle.IsValid()) return;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			HeldInputRetryTimerHandle,
			this,
			&ThisClass::RetryHeldInputActivation,
			HeldInputRetryInterval,
			true);
	}
}

void USyncInputComponent::ClearHeldInputRetry()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HeldInputRetryTimerHandle);
	}

	HeldInputRetryTimerHandle.Invalidate();
}

void USyncInputComponent::Move(const FInputActionValue& Value)
{
	// Retrieve 2D input vector (X: right/left, Y: forward/backward)
	const FVector2D InputVector = Value.Get<FVector2D>();

	// Get Yaw rotation from controller
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController) return;

	const FRotator YawRotation(0.f, PlayerController->GetControlRotation().Yaw, 0.f);

	// Calculate forward and right directions based on Yaw rotation
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// Add movement input to pawn if valid
	if (APawn* ControlledPawn = Cast<APawn>(GetOwner()))
	{
		ControlledPawn->AddMovementInput(Forward, InputVector.Y);
		ControlledPawn->AddMovementInput(Right,   InputVector.X);
	}
}

void USyncInputComponent::Look(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();

	if (APlayerController* PlayerController = GetOwningPlayerController())
	{
		PlayerController->AddYawInput(LookVector.X);
		PlayerController->AddPitchInput(LookVector.Y);
	}
}

void USyncInputComponent::Zoom(const FInputActionValue& Value)
{
	if (!bEnableZoom || MaxZoomLevel < MinZoomLevel) return;

	const float InputAxisValue = Value.Get<float>();
	if (FMath::IsNearlyZero(InputAxisValue)) return;

	if (InputAxisValue > 0.f && ZoomLevel > MinZoomLevel)
	{
		--ZoomLevel;
		ApplyZoom();
	}
	else if (InputAxisValue < 0.f && ZoomLevel < MaxZoomLevel)
	{
		++ZoomLevel;
		ApplyZoom();
	}
}

void USyncInputComponent::ApplyZoom()
{
	if (MaxZoomLevel < MinZoomLevel) return;

	USpringArmComponent* SpringArm = FindSpringArm();
	if (!SpringArm) return;

	ZoomLevel = FMath::Clamp(ZoomLevel, MinZoomLevel, MaxZoomLevel);
	DesiredZoomArmLength = ZoomLevel * ZoomStepDistance;
	SetComponentTickEnabled(true);

	if (UCameraComponent* Camera = FindCamera())
	{
		const FVector& CameraOffset = ZoomLevel == MinZoomLevel
			? ClosestZoomCameraOffset
			: DefaultCameraOffset;

		Camera->SetRelativeLocation(CameraOffset);
	}
}

USpringArmComponent* USyncInputComponent::FindSpringArm() const
{
	if (CachedSpringArm)
	{
		return CachedSpringArm;
	}

	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		return Pawn->FindComponentByClass<USpringArmComponent>();
	}

	return nullptr;
}

UCameraComponent* USyncInputComponent::FindCamera() const
{
	if (CachedCamera)
	{
		return CachedCamera;
	}

	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		return Pawn->FindComponentByClass<UCameraComponent>();
	}

	return nullptr;
}
