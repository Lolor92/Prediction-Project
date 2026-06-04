#include "GAS/Attribute/ModMagCalc/MMC_EnergyRegen.h"
#include "GAS/Attribute/PL_AttributeSet.h"
#include "Character/PL_BaseCharacter.h"

UMMC_EnergyRegen::UMMC_EnergyRegen()
{
	MaxEnergyDef.AttributeToCapture = UPL_AttributeSet::GetMaxEnergyAttribute();
	MaxEnergyDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;
	MaxEnergyDef.bSnapshot = false;
	RelevantAttributesToCapture.Add(MaxEnergyDef);

	CurrentEnergyDef.AttributeToCapture = UPL_AttributeSet::GetEnergyAttribute();
	CurrentEnergyDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;
	CurrentEnergyDef.bSnapshot = false;
	RelevantAttributesToCapture.Add(CurrentEnergyDef);
}

float UMMC_EnergyRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// Gather tags from source and target
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;
	
	// Get target Actor from Spec
	FGameplayEffectContextHandle GameplayEffectContextHandle = Spec.GetContext();
	const APL_BaseCharacter* Character = Cast<APL_BaseCharacter>(GameplayEffectContextHandle.GetSourceObject());

	float MaxEnergy = 0.f;
	GetCapturedAttributeMagnitude(MaxEnergyDef, Spec, EvaluationParameters, MaxEnergy);
	MaxEnergy = FMath::Max(MaxEnergy, 0.f);
	
	float CurrentEnergy = 0.f;
	GetCapturedAttributeMagnitude(CurrentEnergyDef, Spec, EvaluationParameters, CurrentEnergy);
	CurrentEnergy = FMath::Max(CurrentEnergy, 0.f);
	
	if (CurrentEnergy == MaxEnergy || MaxEnergy <= 0.f)
	{
		return 0.f;
	}
	
	const float Regen = 25.f;
	return FMath::RoundToFloat(Regen);
}
