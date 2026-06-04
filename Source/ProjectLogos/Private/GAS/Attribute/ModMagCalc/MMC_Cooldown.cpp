#include "GAS/Attribute/ModMagCalc/MMC_Cooldown.h"


UMMC_Cooldown::UMMC_Cooldown()
{
}

float UMMC_Cooldown::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const UGameplayAbility* SourceAbility = Spec.GetEffectContext().GetAbilityInstance_NotReplicated();
	
	UE_LOG(LogTemp, Warning, TEXT("Ability not found or invalid type."));
	return Super::CalculateBaseMagnitude_Implementation(Spec);
}
