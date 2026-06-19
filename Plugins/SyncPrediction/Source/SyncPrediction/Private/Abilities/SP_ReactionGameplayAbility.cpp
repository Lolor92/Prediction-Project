#include "Abilities/SP_ReactionGameplayAbility.h"
#include "Components/SP_PredictionComponent.h"

void USP_ReactionGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	const FGameplayTag ReactionTag = ResolveReactionTagFromActivationData(TriggerEventData);

	if (ShouldSkipPredictedReactionReplay(ReactionTag))
	{
		UE_LOG(LogTemp, Warning, TEXT("SP ReactionAbility SKIP predicted replay Avatar=%s Tag=%s"),
			*GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr), *ReactionTag.ToString());

		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

bool USP_ReactionGameplayAbility::ShouldSkipPredictedReactionReplay(FGameplayTag ReactionTag) const
{
	if (!ReactionTag.IsValid()) return false;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor || AvatarActor->HasAuthority()) return false;

	USP_PredictionComponent* PredictionComponent = AvatarActor->FindComponentByClass<USP_PredictionComponent>();
	if (!PredictionComponent) return false;

	return PredictionComponent->ConsumePendingPredictedReaction(AvatarActor, ReactionTag);
}

FGameplayTag USP_ReactionGameplayAbility::ResolveReactionTagFromActivationData(
	const FGameplayEventData* TriggerEventData) const
{
	if (TriggerEventData && TriggerEventData->EventTag.IsValid()) return TriggerEventData->EventTag;

	if (AbilityTriggers.Num() == 1 && AbilityTriggers[0].TriggerTag.IsValid())
	{
		return AbilityTriggers[0].TriggerTag;
	}

	return FGameplayTag();
}
