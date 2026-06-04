#include "GAS/Attribute/ModMagCalc/MMC_PVPDefense.h"
#include "Character/PL_BaseCharacter.h"


UMMC_PVPDefense::UMMC_PVPDefense()
{
}

float UMMC_PVPDefense::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// Gather tags from source and target.
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;
    
	// Get the source Actor from the Spec.
	FGameplayEffectContextHandle GameplayEffectContextHandle = Spec.GetContext();
	const APL_BaseCharacter* SourceCharacter = Cast<APL_BaseCharacter>(GameplayEffectContextHandle.GetSourceObject());
    
	float PVPDefense = 0.f;
    
	return PVPDefense;
}

