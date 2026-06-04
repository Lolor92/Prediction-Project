// Copyright ProjectLogos

#include "GAS/Attribute/ExecCalc/ExecCalc_Damage.h"
#include "AbilitySystemComponent.h"
#include "GAS/Attribute/PL_AttributeSet.h"


struct PLDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalAttack);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MagicalAttack);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalDefense);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MagicalDefense);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DamageReduction);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalDamageReduction);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MagicalDamageReduction);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamageReduction);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PVPAttack);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PVPDefense);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalDefensePenetration);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MagicalDefensePenetration);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamage);
	
	PLDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPL_AttributeSet, PhysicalAttack, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPL_AttributeSet, MagicalAttack, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPL_AttributeSet, PhysicalDefense, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPL_AttributeSet, MagicalDefense, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPL_AttributeSet, DamageReduction, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPL_AttributeSet, PhysicalDamageReduction, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPL_AttributeSet, MagicalDamageReduction, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPL_AttributeSet, CriticalDamageReduction, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPL_AttributeSet, PVPAttack, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPL_AttributeSet, PVPDefense, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPL_AttributeSet, IncomingDamage, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPL_AttributeSet, PhysicalDefensePenetration, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPL_AttributeSet, MagicalDefensePenetration, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPL_AttributeSet, CriticalChance, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPL_AttributeSet, CriticalDamage, Source, true);
	}
	
};

static const PLDamageStatics& DamageStatics()
{
	static PLDamageStatics DStatics;
	return DStatics;
}

UExecCalc_Damage::UExecCalc_Damage()
{
	RelevantAttributesToCapture.Add(DamageStatics().PhysicalAttackDef);
	RelevantAttributesToCapture.Add(DamageStatics().MagicalAttackDef);
	RelevantAttributesToCapture.Add(DamageStatics().PhysicalDefenseDef);
	RelevantAttributesToCapture.Add(DamageStatics().MagicalDefenseDef);
	RelevantAttributesToCapture.Add(DamageStatics().DamageReductionDef);
	RelevantAttributesToCapture.Add(DamageStatics().PhysicalDamageReductionDef);
	RelevantAttributesToCapture.Add(DamageStatics().MagicalDamageReductionDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalDamageReductionDef);
	RelevantAttributesToCapture.Add(DamageStatics().PVPAttackDef);
	RelevantAttributesToCapture.Add(DamageStatics().PVPDefenseDef);
	RelevantAttributesToCapture.Add(DamageStatics().IncomingDamageDef);
	RelevantAttributesToCapture.Add(DamageStatics().PhysicalDefensePenetrationDef);
	RelevantAttributesToCapture.Add(DamageStatics().MagicalDefensePenetrationDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalChanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalDamageDef);
}

void UExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
                                              FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float SourcePhysicalAttack = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().PhysicalAttackDef, EvaluationParameters, SourcePhysicalAttack);
	SourcePhysicalAttack = FMath::Max(SourcePhysicalAttack, 0.f);

	float SourceMagicalAttack = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MagicalAttackDef, EvaluationParameters, SourceMagicalAttack);
	SourceMagicalAttack = FMath::Max(SourceMagicalAttack, 0.f);

	float TargetPhysicalDefense = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().PhysicalDefenseDef, EvaluationParameters, TargetPhysicalDefense);
	TargetPhysicalDefense = FMath::Max<float>(0.f, TargetPhysicalDefense);

	float TargetMagicalDefense = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MagicalDefenseDef, EvaluationParameters, TargetMagicalDefense);
	TargetMagicalDefense = FMath::Max<float>(0.f, TargetMagicalDefense);

	float TargetDamageReduction = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DamageReductionDef, EvaluationParameters, TargetDamageReduction);
	TargetDamageReduction = FMath::Max<float>(0.f, TargetDamageReduction);

	float TargetPhysicalDamageReduction = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().PhysicalDamageReductionDef, EvaluationParameters, TargetPhysicalDamageReduction);
	TargetPhysicalDamageReduction = FMath::Max<float>(0.f, TargetPhysicalDamageReduction);

	float TargetMagicalDamageReduction = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MagicalDamageReductionDef, EvaluationParameters, TargetMagicalDamageReduction);
	TargetMagicalDamageReduction = FMath::Max<float>(0.f, TargetMagicalDamageReduction);

	float TargetCriticalDamageReduction = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalDamageReductionDef, EvaluationParameters, TargetCriticalDamageReduction);
	TargetCriticalDamageReduction = FMath::Max<float>(0.f, TargetCriticalDamageReduction);

	float SourcePVPAttack = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().PVPAttackDef, EvaluationParameters, SourcePVPAttack);
	SourcePVPAttack = FMath::Max<float>(0.f, SourcePVPAttack);

	float TargetPVPDefense = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().PVPDefenseDef, EvaluationParameters, TargetPVPDefense);
	TargetPVPDefense = FMath::Max<float>(0.f, TargetPVPDefense);

	float IncomingDamage = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().IncomingDamageDef, EvaluationParameters, IncomingDamage);
	IncomingDamage = FMath::Max<float>(0.f, IncomingDamage);

	float SourcePhysicalDefensePenetration = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().PhysicalDefensePenetrationDef, EvaluationParameters, SourcePhysicalDefensePenetration);
	SourcePhysicalDefensePenetration = FMath::Max<float>(0.f, SourcePhysicalDefensePenetration);

	float SourceMagicalDefensePenetration = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MagicalDefensePenetrationDef, EvaluationParameters, SourceMagicalDefensePenetration);
	SourceMagicalDefensePenetration = FMath::Max<float>(0.f, SourceMagicalDefensePenetration);

	float CriticalChance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalChanceDef, EvaluationParameters, CriticalChance);
	CriticalChance = FMath::Max<float>(0.f, CriticalChance);

	float CriticalDamage = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalDamageDef, EvaluationParameters, CriticalDamage);
	CriticalDamage = FMath::Max<float>(0.f, CriticalDamage);

	if (Spec.DynamicGrantedTags.HasTag(FGameplayTag::RequestGameplayTag(FName("Hit.Dodged"), false)))
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UPL_AttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, 0.f));
		return;
	}

	float Damage = 0.f;
	float PhysicalAttackPercent = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Damage.Type.Physical"), false));
	float MagicalAttackPercent = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Damage.Type.Magical"), false));
	
	float PhysicalDamage = CalculateBaseDamage(SourcePhysicalAttack, TargetPhysicalDefense, SourcePhysicalDefensePenetration, PhysicalAttackPercent);
	float MagicalDamage = CalculateBaseDamage(SourceMagicalAttack, TargetMagicalDefense, SourceMagicalDefensePenetration, MagicalAttackPercent);
	
	Damage = PhysicalDamage + MagicalDamage;
	Damage = ApplySharedCalculations(Damage, TargetDamageReduction, SourcePVPAttack, TargetPVPDefense, false);

	if (const bool bIsCritical = FMath::RandRange(0.f, 1.f) <= CriticalChance)
	{
		Damage *= (1.f + CriticalDamage);
		Damage *= (1.f - TargetCriticalDamageReduction);
		
		FGameplayEffectSpec* MutableSpec = const_cast<FGameplayEffectSpec*>(&ExecutionParams.GetOwningSpec());
		if (MutableSpec)
		{
			MutableSpec->DynamicGrantedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Hit.Critical"), false));
		}
	}
	
	// Make damage wiggle within 10%
	Damage *= FMath::FRandRange(0.9f, 1.1f);
	Damage = FMath::Max(0.f, Damage);

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UPL_AttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, Damage));
}

// Helper function for base damage calculation
float UExecCalc_Damage::CalculateBaseDamage(float SourceAttack, float TargetDefense, float SourceDefensePenetration, float DamagePercent) const
{
    const float EffectiveDefense = TargetDefense * (1.f - SourceDefensePenetration);

    // Calculate the damage reduction factor based on defense
    float DamageReduction = EffectiveDefense / (EffectiveDefense + 1000.f);

	// Calculate the initial damage based on the source's attack and the damage percentage
	const float DamageValue = SourceAttack * DamagePercent;

    // Calculate the attack-defense ratio
    const float AttackDefenseRatio = DamageValue / (EffectiveDefense + 1000.f);

    // Scale the damage reduction based on the attack-defense ratio
    DamageReduction = DamageReduction * (1 - AttackDefenseRatio);

    // Apply the overall damage reduction to the damage value
    const float ReducedDamage = DamageValue * (1.f - DamageReduction);

    return ReducedDamage;
}

float UExecCalc_Damage::ApplySharedCalculations(float Damage, float TargetDamageReduction, float SourcePVPAttack, float TargetPVPDefense, bool bIsPVP)
{
    // Apply target's overall damage reduction
    Damage *= (1.f - TargetDamageReduction);

    if (bIsPVP)
    {
        // Adjust damage based on PvP stats
        Damage *= (1.f + SourcePVPAttack - TargetPVPDefense);
    }

    return Damage;
}
