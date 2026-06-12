#include "BlueprintLibrary/SCP_CombatPredictionBlueprintLibrary.h"

#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "Components/SCP_CombatPredictionComponent.h"
#include "GameFramework/Actor.h"

namespace SyncCombatPrediction
{
	constexpr int32 DefaultPredictionEventId = 0;
}

FSCP_CombatPredictionContext USCP_CombatPredictionBlueprintLibrary::MakePredictionContextFromAbility(
	const UGameplayAbility* Ability)
{
	FSCP_CombatPredictionContext Context;
	Context.PredictionEventId = SyncCombatPrediction::DefaultPredictionEventId;

	if (!Ability)
	{
		return Context;
	}

	const FGameplayAbilityActivationInfo ActivationInfo = Ability->GetCurrentActivationInfo();
	const FPredictionKey PredictionKey = ActivationInfo.GetActivationPredictionKey();

	Context.AbilityPredictionKey = PredictionKey.Current;
	Context.AbilityPredictionBase = PredictionKey.Base;
	Context.bHasValidPredictionKey = PredictionKey.IsValidKey();

	const FGameplayAbilitySpecHandle SpecHandle = Ability->GetCurrentAbilitySpecHandle();
	Context.AbilitySpecHandle = SpecHandle.IsValid() ? static_cast<int32>(GetTypeHash(SpecHandle)) : INDEX_NONE;

	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	Context.bIsLocallyControlled = ActorInfo && ActorInfo->IsLocallyControlled();
	Context.bIsAuthority = ActorInfo && ActorInfo->IsNetAuthority();

	return Context;
}

bool USCP_CombatPredictionBlueprintLibrary::StartCombatPredictionFromAbility(
	const UGameplayAbility* Ability,
	FSCP_CombatPredictionContext& OutContext)
{
	OutContext = FSCP_CombatPredictionContext();

	if (!Ability)
	{
		return false;
	}

	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	USCP_CombatPredictionComponent* PredictionComponent = GetCombatPredictionComponent(AvatarActor);
	if (!PredictionComponent)
	{
		return false;
	}

	OutContext = PredictionComponent->StartPredictionFromAbility(Ability);
	return OutContext.IsValidForPrediction();
}

USCP_CombatPredictionComponent* USCP_CombatPredictionBlueprintLibrary::GetCombatPredictionComponent(
	const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<USCP_CombatPredictionComponent>() : nullptr;
}

bool USCP_CombatPredictionBlueprintLibrary::IsPlayingPredictedTargetReaction(
	const AActor* Actor)
{
	const USCP_CombatPredictionComponent* PredictionComponent =
		GetCombatPredictionComponent(Actor);

	return PredictionComponent &&
		PredictionComponent->IsPlayingPredictedTargetReaction();
}

void USCP_CombatPredictionBlueprintLibrary::ApplyPredictedTargetReactionAnimLock(
	const AActor* Actor,
	float CurrentGroundSpeed,
	bool bCurrentIsAccelerating,
	float CurrentMovementOffsetYaw,
	float& OutGroundSpeed,
	bool& bOutIsAccelerating,
	float& OutMovementOffsetYaw,
	bool& bOutApplied)
{
	bOutApplied = IsPlayingPredictedTargetReaction(Actor);

	if (bOutApplied)
	{
		OutGroundSpeed = 0.f;
		bOutIsAccelerating = false;
		OutMovementOffsetYaw = 0.f;
		return;
	}

	OutGroundSpeed = CurrentGroundSpeed;
	bOutIsAccelerating = bCurrentIsAccelerating;
	OutMovementOffsetYaw = CurrentMovementOffsetYaw;
}

bool USCP_CombatPredictionBlueprintLibrary::IsValidPredictionContext(
	const FSCP_CombatPredictionContext& Context)
{
	return Context.IsValidForPrediction();
}

FString USCP_CombatPredictionBlueprintLibrary::PredictionContextToString(
	const FSCP_CombatPredictionContext& Context)
{
	return Context.ToDebugString();
}
