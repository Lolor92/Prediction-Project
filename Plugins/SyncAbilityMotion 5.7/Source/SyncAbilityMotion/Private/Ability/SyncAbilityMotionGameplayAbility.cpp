#include "Ability/SyncAbilityMotionGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Component/SyncAbilityMotionComponent.h"
#include "Components/ActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSyncAbilityMotionAbilityDiag, Log, All);

namespace
{
	TAutoConsoleVariable<int32> CVarSyncAbilityMotionAbilityDiagnostics(
		TEXT("sync.AbilityMotion.AbilityDiagnostics"),
		0,
		TEXT("Enable low-volume SyncAbilityMotion ability diagnostic logs."),
		ECVF_Default);

	bool IsSyncAbilityMotionAbilityDiagnosticsEnabled()
	{
		return CVarSyncAbilityMotionAbilityDiagnostics.GetValueOnGameThread() != 0;
	}

	bool IsAbilityActivationSuppressedByAvatarComponent(const FGameplayAbilityActorInfo* ActorInfo)
	{
		AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
		if (!Avatar)
		{
			return false;
		}

		struct FIsAbilityActivationSuppressedByReactionParams
		{
			bool ReturnValue = false;
		};

		TArray<UActorComponent*> Components;
		Avatar->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			if (!Component)
			{
				continue;
			}

			UFunction* Function = Component->FindFunction(TEXT("IsAbilityActivationSuppressedByReaction"));
			if (!Function)
			{
				continue;
			}

			FIsAbilityActivationSuppressedByReactionParams Params;
			Component->ProcessEvent(Function, &Params);
			if (Params.ReturnValue)
			{
				return true;
			}
		}

		return false;
	}
}

USyncAbilityMotionGameplayAbility::USyncAbilityMotionGameplayAbility()
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	bServerRespectsRemoteAbilityCancellation = false;
	bRetriggerInstancedAbility = true;
	bComboSupportAvailable = FModuleManager::Get().ModuleExists(TEXT("SyncInput"));
}

void USyncAbilityMotionGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ActivationSequenceId = (ActivationSequenceId == MAX_uint32) ? 1u : (ActivationSequenceId + 1u);
	ResetComboWindow();

	if (IsSyncAbilityMotionAbilityDiagnosticsEnabled())
	{
		const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : GetAvatarActorFromActorInfo();
		UE_LOG(
			LogSyncAbilityMotionAbilityDiag,
			Log,
			TEXT("Activate Ability=%s Avatar=%s Seq=%u Authority=%s Local=%s Montage=%s"),
			*GetNameSafe(this),
			*GetNameSafe(Avatar),
			ActivationSequenceId,
			Avatar && Avatar->HasAuthority() ? TEXT("true") : TEXT("false"),
			ActorInfo && ActorInfo->IsLocallyControlled() ? TEXT("true") : TEXT("false"),
			*GetNameSafe(GetCurrentMontage()));
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	InterruptOtherActiveAbilities();
	RotateAvatarToControllerYawOnActivate();
	ResetGameplayEffectWindows();
	ScheduleGameplayEffectWindowUpdate();
	OpenComboWindow();
}

void USyncAbilityMotionGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsSyncAbilityMotionAbilityDiagnosticsEnabled())
	{
		const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : GetAvatarActorFromActorInfo();
		UE_LOG(
			LogSyncAbilityMotionAbilityDiag,
			Log,
			TEXT("End Ability=%s Avatar=%s Seq=%u Cancelled=%s ReplicateEnd=%s Authority=%s Local=%s"),
			*GetNameSafe(this),
			*GetNameSafe(Avatar),
			ActivationSequenceId,
			bWasCancelled ? TEXT("true") : TEXT("false"),
			bReplicateEndAbility ? TEXT("true") : TEXT("false"),
			Avatar && Avatar->HasAuthority() ? TEXT("true") : TEXT("false"),
			ActorInfo && ActorInfo->IsLocallyControlled() ? TEXT("true") : TEXT("false"));
	}

	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (USyncAbilityMotionComponent* MotionComponent =
			Character->FindComponentByClass<USyncAbilityMotionComponent>())
		{
			MotionComponent->ResetAbilityMotionState();
		}
	}

	CleanupActiveGameplayEffectWindows();
	ClearGameplayEffectWindowUpdate();
	ResetComboWindow();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool USyncAbilityMotionGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		if (IsSyncAbilityMotionAbilityDiagnosticsEnabled())
		{
			const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : GetAvatarActorFromActorInfo();
			UE_LOG(
				LogSyncAbilityMotionAbilityDiag,
				Log,
				TEXT("CanActivate denied by GAS Ability=%s Avatar=%s Authority=%s Local=%s"),
				*GetNameSafe(this),
				*GetNameSafe(Avatar),
				Avatar && Avatar->HasAuthority() ? TEXT("true") : TEXT("false"),
				ActorInfo && ActorInfo->IsLocallyControlled() ? TEXT("true") : TEXT("false"));
		}
		return false;
	}

	if (!bBypassReactionActivationSuppression && IsAbilityActivationSuppressedByAvatarComponent(ActorInfo))
	{
		if (IsSyncAbilityMotionAbilityDiagnosticsEnabled())
		{
			const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : GetAvatarActorFromActorInfo();
			UE_LOG(
				LogSyncAbilityMotionAbilityDiag,
				Log,
				TEXT("CanActivate denied by reaction suppression Ability=%s Avatar=%s Authority=%s Local=%s"),
				*GetNameSafe(this),
				*GetNameSafe(Avatar),
				Avatar && Avatar->HasAuthority() ? TEXT("true") : TEXT("false"),
				ActorInfo && ActorInfo->IsLocallyControlled() ? TEXT("true") : TEXT("false"));
		}
		return false;
	}

	return CanInterruptAnimatingAbility(ActorInfo);
}

bool USyncAbilityMotionGameplayAbility::DoesAbilityUseGameplayTag(FGameplayTag GameplayTag) const
{
	return GameplayTag.IsValid() &&
		(GetAssetTags().HasTag(GameplayTag) || ActivationOwnedTags.HasTag(GameplayTag));
}

bool USyncAbilityMotionGameplayAbility::CanInterruptAnimatingAbility(
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	const auto LogLockoutDenied =
		[this, ActorInfo](const TCHAR* Reason, const UGameplayAbility* AnimatingAbility,
			const UAnimMontage* AnimatingMontage, float CurrentPosition, float PlayedPercent,
			float UnlockPercent)
		{
			if (IsSyncAbilityMotionAbilityDiagnosticsEnabled())
			{
				const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : GetAvatarActorFromActorInfo();
				UE_LOG(
					LogSyncAbilityMotionAbilityDiag,
					Log,
					TEXT("CanActivate denied by montage lockout Ability=%s Avatar=%s Reason=%s AnimatingAbility=%s Montage=%s Position=%.3f Played=%.2f Unlock=%.2f Authority=%s Local=%s"),
					*GetNameSafe(this),
					*GetNameSafe(Avatar),
					Reason,
					*GetNameSafe(AnimatingAbility),
					*GetNameSafe(AnimatingMontage),
					CurrentPosition,
					PlayedPercent,
					UnlockPercent,
					Avatar && Avatar->HasAuthority() ? TEXT("true") : TEXT("false"),
					ActorInfo && ActorInfo->IsLocallyControlled() ? TEXT("true") : TEXT("false"));
			}
		};

	if (MontageLockout.bBypassMontageLockout)
	{
		return true;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		ASC = GetAbilitySystemComponentFromActorInfo();
	}

	if (!ASC)
	{
		return true;
	}

	const UGameplayAbility* AnimatingAbility = ASC->GetAnimatingAbility();
	const USyncAbilityMotionGameplayAbility* AnimatingMotionAbility =
		Cast<USyncAbilityMotionGameplayAbility>(AnimatingAbility);
	if (!AnimatingMotionAbility)
	{
		return true;
	}

	if (!AnimatingMotionAbility->MontageLockout.bUseMontageProgressLockout ||
		!AnimatingMotionAbility->MontageLockout.bBlockAbilityActivationUntilUnlock)
	{
		return true;
	}

	const ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Character)
	{
		Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	}

	const USyncAbilityMotionComponent* MotionComponent =
		Character ? Character->FindComponentByClass<USyncAbilityMotionComponent>() : nullptr;
	if (MotionComponent && MotionComponent->GetAbilityMotionState().bCanBlendMontage)
	{
		return true;
	}

	const UAnimMontage* AnimatingMontage = AnimatingAbility->GetCurrentMontage();
	if (!AnimatingMontage)
	{
		LogLockoutDenied(TEXT("NoAnimatingMontage"), AnimatingAbility, nullptr, -1.f, -1.f, -1.f);
		return false;
	}

	const float MontageLength = AnimatingMontage->GetPlayLength();
	if (MontageLength <= KINDA_SMALL_NUMBER)
	{
		LogLockoutDenied(TEXT("InvalidMontageLength"), AnimatingAbility, AnimatingMontage, -1.f, -1.f, -1.f);
		return false;
	}

	const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	const UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		LogLockoutDenied(TEXT("NoAnimInstance"), AnimatingAbility, AnimatingMontage, -1.f, -1.f, -1.f);
		return false;
	}

	const float CurrentPosition = AnimInstance->Montage_GetPosition(AnimatingMontage);
	const float PlayedPercent = FMath::Clamp((CurrentPosition / MontageLength) * 100.f, 0.f, 100.f);
	const float UnlockPercent = FMath::Clamp(
		AnimatingMotionAbility->MontageLockout.MontageProgressBeforeInterrupt,
		0.f,
		100.f);

	if (PlayedPercent < UnlockPercent)
	{
		LogLockoutDenied(
			TEXT("ProgressBeforeUnlock"),
			AnimatingAbility,
			AnimatingMontage,
			CurrentPosition,
			PlayedPercent,
			UnlockPercent);
	}

	return PlayedPercent >= UnlockPercent;
}

void USyncAbilityMotionGameplayAbility::InterruptOtherActiveAbilities() const
{
	if (!bInterruptOtherAbilitiesOnActivate) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	ASC->CancelAllAbilities(const_cast<USyncAbilityMotionGameplayAbility*>(this));
}

void USyncAbilityMotionGameplayAbility::OpenComboWindow()
{
	CloseComboWindow();

	if (!bComboSupportAvailable || !ComboAbilityClass || ComboWindowDuration <= 0.f) return;

	bComboWindowOpen = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ComboWindowTimerHandle,
			this,
			&ThisClass::CloseComboWindow,
			ComboWindowDuration,
			false);
	}
}

void USyncAbilityMotionGameplayAbility::CloseComboWindow()
{
	bComboWindowOpen = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ComboWindowTimerHandle);
	}
}

void USyncAbilityMotionGameplayAbility::ResetComboWindow()
{
	CloseComboWindow();
}

void USyncAbilityMotionGameplayAbility::ResetGameplayEffectWindows()
{
	GameplayEffectWindowRuntime.Reset();
	GameplayEffectWindowRuntime.SetNum(GameplayEffectWindows.Num());
}

void USyncAbilityMotionGameplayAbility::ScheduleGameplayEffectWindowUpdate()
{
	if (GameplayEffectWindows.IsEmpty()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	World->GetTimerManager().SetTimer(
		GameplayEffectWindowTimerHandle,
		this,
		&ThisClass::UpdateGameplayEffectWindows,
		FMath::Max(GameplayEffectWindowUpdateInterval, 0.01f),
		true,
		0.f);
}

void USyncAbilityMotionGameplayAbility::ClearGameplayEffectWindowUpdate()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GameplayEffectWindowTimerHandle);
	}

	GameplayEffectWindowTimerHandle.Invalidate();
}

void USyncAbilityMotionGameplayAbility::UpdateGameplayEffectWindows()
{
	if (!IsActive())
	{
		ClearGameplayEffectWindowUpdate();
		return;
	}

	const float PlayedPercent = GetCurrentMontagePlayedPercent();
	if (PlayedPercent < 0.f)
	{
		return;
	}

	for (int32 Index = 0; Index < GameplayEffectWindows.Num(); ++Index)
	{
		if (!GameplayEffectWindowRuntime.IsValidIndex(Index))
		{
			continue;
		}

		const FSyncAbilityMotionGameplayEffectWindow& Window = GameplayEffectWindows[Index];
		FSyncAbilityMotionGameplayEffectWindowRuntime& Runtime = GameplayEffectWindowRuntime[Index];
		const float StartPercent = FMath::Clamp(Window.StartPercent, 0.f, 100.f);
		const float EndPercent = FMath::Clamp(FMath::Max(Window.EndPercent, StartPercent), 0.f, 100.f);

		if (!Runtime.bHasEntered && PlayedPercent >= StartPercent)
		{
			EnterGameplayEffectWindow(Index);
		}

		if (Runtime.bHasEntered && !Runtime.bHasExited && PlayedPercent >= EndPercent)
		{
			ExitGameplayEffectWindow(Index);
		}
	}
}

void USyncAbilityMotionGameplayAbility::EnterGameplayEffectWindow(int32 WindowIndex)
{
	if (!GameplayEffectWindows.IsValidIndex(WindowIndex) || !GameplayEffectWindowRuntime.IsValidIndex(WindowIndex))
	{
		return;
	}

	FSyncAbilityMotionGameplayEffectWindowRuntime& Runtime = GameplayEffectWindowRuntime[WindowIndex];
	if (Runtime.bHasEntered)
	{
		return;
	}

	const FSyncAbilityMotionGameplayEffectWindow& Window = GameplayEffectWindows[WindowIndex];
	RemoveGameplayEffectsFromSelf(Window.StartEffectsToRemove);
	ApplyGameplayEffectsToSelf(Window.StartEffectsToApply);
	ApplyGameplayEffectsToSelf(Window.DurationEffects);
	Runtime.bHasEntered = true;
}

void USyncAbilityMotionGameplayAbility::ExitGameplayEffectWindow(int32 WindowIndex)
{
	if (!GameplayEffectWindows.IsValidIndex(WindowIndex) || !GameplayEffectWindowRuntime.IsValidIndex(WindowIndex))
	{
		return;
	}

	FSyncAbilityMotionGameplayEffectWindowRuntime& Runtime = GameplayEffectWindowRuntime[WindowIndex];
	if (!Runtime.bHasEntered || Runtime.bHasExited)
	{
		return;
	}

	const FSyncAbilityMotionGameplayEffectWindow& Window = GameplayEffectWindows[WindowIndex];
	RemoveGameplayEffectsFromSelf(Window.DurationEffects);
	RemoveGameplayEffectsFromSelf(Window.EndEffectsToRemove);
	ApplyGameplayEffectsToSelf(Window.EndEffectsToApply);
	Runtime.bHasExited = true;
}

void USyncAbilityMotionGameplayAbility::CleanupActiveGameplayEffectWindows()
{
	for (int32 Index = 0; Index < GameplayEffectWindows.Num(); ++Index)
	{
		if (!GameplayEffectWindowRuntime.IsValidIndex(Index))
		{
			continue;
		}

		if (GameplayEffectWindowRuntime[Index].bHasEntered && !GameplayEffectWindowRuntime[Index].bHasExited)
		{
			ExitGameplayEffectWindow(Index);
		}
	}
}

float USyncAbilityMotionGameplayAbility::GetCurrentMontagePlayedPercent() const
{
	const UAnimMontage* Montage = GetCurrentMontage();
	if (!Montage)
	{
		return -1.f;
	}

	const float MontageLength = Montage->GetPlayLength();
	if (MontageLength <= KINDA_SMALL_NUMBER)
	{
		return -1.f;
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	const UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return -1.f;
	}

	return FMath::Clamp((AnimInstance->Montage_GetPosition(Montage) / MontageLength) * 100.f, 0.f, 100.f);
}

void USyncAbilityMotionGameplayAbility::ApplyGameplayEffectsToSelf(
	const TArray<TSubclassOf<UGameplayEffect>>& EffectClasses) const
{
	if (EffectClasses.IsEmpty()) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!ASC || !Avatar) return;

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	EffectContext.AddInstigator(Avatar, Avatar);

	const float SourceLevel = FMath::Max(GetAbilityLevel(), 1);
	for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectClasses)
	{
		if (!EffectClass) continue;

		const FGameplayEffectSpecHandle SpecHandle =
			ASC->MakeOutgoingSpec(EffectClass, SourceLevel, EffectContext);
		if (SpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void USyncAbilityMotionGameplayAbility::RemoveGameplayEffectsFromSelf(
	const TArray<TSubclassOf<UGameplayEffect>>& EffectClasses) const
{
	if (EffectClasses.IsEmpty()) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectClasses)
	{
		if (!EffectClass) continue;

		FGameplayEffectQuery Query;
		Query.EffectDefinition = EffectClass;
		ASC->RemoveActiveEffects(Query);
	}
}

bool USyncAbilityMotionGameplayAbility::ShouldPauseRootMotionOnCharacterImpact(
	const ACharacter* Character,
	const FHitResult& Hit,
	const FVector& MoveDelta) const
{
	if (!bPauseRootMotionOnCharacterImpact || !Character)
	{
		return false;
	}

	const ACharacter* OtherCharacter = Cast<ACharacter>(Hit.GetActor());
	if (!OtherCharacter || OtherCharacter == Character)
	{
		return false;
	}

	const UPrimitiveComponent* OtherComp = Hit.GetComponent();
	if (OtherComp)
	{
		const bool bIsCapsule = OtherComp->IsA<UCapsuleComponent>();
		const bool bIsMesh = OtherComp->IsA<USkeletalMeshComponent>();

		if (!bIsCapsule && !bIsMesh)
		{
			return false;
		}
	}

	FVector Forward = MoveDelta;
	Forward.Z = 0.f;

	if (!Forward.Normalize())
	{
		if (const UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
		{
			Forward = MovementComponent->Velocity;
			Forward.Z = 0.f;
		}
	}

	if (!Forward.Normalize())
	{
		Forward = Character->GetActorForwardVector();
		Forward.Z = 0.f;
	}

	if (!Forward.Normalize())
	{
		return false;
	}

	FVector ToImpact = Hit.ImpactPoint - Character->GetActorLocation();
	ToImpact.Z = 0.f;

	if (Hit.bStartPenetrating || ToImpact.IsNearlyZero())
	{
		ToImpact = OtherCharacter->GetActorLocation() - Character->GetActorLocation();
		ToImpact.Z = 0.f;
	}

	if (!ToImpact.Normalize())
	{
		return false;
	}

	const float ClampedAngleDegrees = FMath::Clamp(
		RootMotionCharacterImpactForwardAngleDegrees,
		0.f,
		180.f);

	const float MinForwardDot = FMath::Cos(FMath::DegreesToRadians(ClampedAngleDegrees));
	const float ForwardDot = FVector::DotProduct(Forward, ToImpact);

	return ForwardDot >= MinForwardDot;
}

void USyncAbilityMotionGameplayAbility::RotateAvatarToControllerYawOnActivate() const
{
	if (!bRotateToControllerYawOnActivate) return;

	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	if (!Info) return;

	ACharacter* Character = Cast<ACharacter>(Info->AvatarActor.Get());
	if (!Character) return;

	if (!Info->IsLocallyControlled() && !Info->IsNetAuthority()) return;

	AController* Controller = Info->PlayerController.IsValid()
		? Info->PlayerController.Get()
		: Character->GetController();
	if (!Controller) return;

	FRotator NewRot = Character->GetActorRotation();
	NewRot.Yaw = Controller->GetControlRotation().Yaw;
	Character->SetActorRotation(NewRot, ETeleportType::ResetPhysics);

	if (Character->IsLocallyControlled())
	{
		if (USpringArmComponent* SpringArm = Character->FindComponentByClass<USpringArmComponent>())
		{
			SpringArm->TickComponent(0.f, LEVELTICK_All, nullptr);
		}
	}
}
