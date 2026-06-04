#include "GAS/Attribute/ModMagCalc/MMC_PhysicalDefense.h"
#include "GAS/Attribute/PL_AttributeSet.h"
#include "Character/PL_BaseCharacter.h"

UMMC_PhysicalDefense::UMMC_PhysicalDefense()
{
	StrengthDef.AttributeToCapture = UPL_AttributeSet::GetStrengthAttribute();
	StrengthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	StrengthDef.bSnapshot = false;
	RelevantAttributesToCapture.Add(StrengthDef);
}

float UMMC_PhysicalDefense::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// Gather tags from source and target
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// Get source Actor from Spec.
	/*FGameplayEffectContextHandle GameplayEffectContextHandle = Spec.GetContext();
	const APL_BaseCharacter* SourceCharacter = Cast<APL_BaseCharacter>(GameplayEffectContextHandle.GetSourceObject());*/

	float Strength = 0;
	GetCapturedAttributeMagnitude(StrengthDef, Spec, EvaluationParameters, Strength);
	Strength = FMath::Max<float>(Strength, 0.f);
	
	return (Strength * 2);
}
