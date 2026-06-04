// Copyright ProjectLogos

#include "GAS/Attribute/PL_AttributeSet.h"
#include <Net/UnrealNetwork.h>
#include <GameFramework/CharacterMovementComponent.h>
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "GameplayEffectExtension.h"
#include "Player/PL_PlayerController.h"

void UPL_AttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, Energy, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, MaxEnergy, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, Intelligence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, Agility, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, PhysicalAttack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, MagicalAttack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, PhysicalDefense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, MagicalDefense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, CriticalChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, CriticalDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, MovementSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, MountSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, CooldownReduction, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, HealthRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, EnergyRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, StaminaRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, HealingEffectiveness, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, ShieldingEffectiveness, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, DamageReduction, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, PhysicalDamageReduction, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, MagicalDamageReduction, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, CriticalDamageReduction, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, PVPAttack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, PVPDefense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, PhysicalDefensePenetration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, MagicalDefensePenetration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPL_AttributeSet, DamageIncrease, COND_None, REPNOTIFY_Always);
}

void UPL_AttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	if (Attribute == GetEnergyAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxEnergy());
	}
	if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	if (FGameplayAbilityActorInfo* ActorInfo = GetActorInfo())
	{
		if (Attribute == GetMovementSpeedAttribute())
		{
			if (UCharacterMovementComponent* CharacterMovement = Cast<UCharacterMovementComponent>(ActorInfo->MovementComponent))
			{
				CharacterMovement->MaxWalkSpeed = NewValue;
			}
		}
	}
}

void UPL_AttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}

void UPL_AttributeSet::SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props) const
{
	// Source = causer of the effect, Target = target of the effect (owner of this AS)

	Props.EffectContextHandle = Data.EffectSpec.GetContext();
	Props.SourceASC = Props.EffectContextHandle.GetOriginalInstigatorAbilitySystemComponent();

	if (IsValid(Props.SourceASC) && Props.SourceASC->AbilityActorInfo.IsValid() && Props.SourceASC->AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.SourceAvatarActor = Props.SourceASC->AbilityActorInfo->AvatarActor.Get();
		Props.SourceController = Props.SourceASC->AbilityActorInfo->PlayerController.Get();
		if (Props.SourceController == nullptr && Props.SourceAvatarActor != nullptr)
		{
			if (const APawn* Pawn = Cast<APawn>(Props.SourceAvatarActor))
			{
				Props.SourceController = Pawn->GetController();
			}
		}
		if (Props.SourceController)
		{
			Props.SourceCharacter = Cast<ACharacter>(Props.SourceController->GetPawn());
		}
	}

	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.TargetAvatarActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		Props.TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		Props.TargetCharacter = Cast<ACharacter>(Props.TargetAvatarActor);
		Props.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Props.TargetAvatarActor);
	}
}

void UPL_AttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FEffectProperties Props;
	SetEffectProperties(Data, Props);

	const FGameplayAttribute Attribute = Data.EvaluatedData.Attribute;
	
	if (Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	if (Attribute == GetEnergyAttribute())
	{
		SetEnergy(FMath::Clamp(GetEnergy(), 0.f, GetMaxEnergy()));
	}
	if (Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.f, GetMaxStamina()));
	}
	if (Attribute == GetIncomingDamageAttribute())
	{
		if (GetIncomingDamage() <= 0.f) return;
		float Damage = static_cast<float>(FMath::RoundToInt(GetIncomingDamage()));
		SetIncomingDamage(0.f);

		const float CurrentHealth = GetHealth();
		const float NewHealth = FMath::Clamp(CurrentHealth - Damage, 0.f, GetMaxHealth());
		
		if (!FMath::IsNearlyEqual(NewHealth, CurrentHealth))
		{
			SetHealth(NewHealth);
		}
	}
}

void UPL_AttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, Health, OldHealth);
}

void UPL_AttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, MaxHealth, OldMaxHealth);
}

void UPL_AttributeSet::OnRep_Energy(const FGameplayAttributeData& OldEnergy) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, Energy, OldEnergy);
}

void UPL_AttributeSet::OnRep_MaxEnergy(const FGameplayAttributeData& OldMaxEnergy) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, MaxEnergy, OldMaxEnergy);
}

void UPL_AttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, Stamina, OldStamina);
}

void UPL_AttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, MaxStamina, OldMaxStamina);
}

void UPL_AttributeSet::OnRep_Strength(const FGameplayAttributeData& OldStrength) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, Strength, OldStrength);
}

void UPL_AttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldIntelligence) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, Intelligence, OldIntelligence);
}

void UPL_AttributeSet::OnRep_Agility(const FGameplayAttributeData& OldAgility) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, Agility, OldAgility);
}

void UPL_AttributeSet::OnRep_PhysicalAttack(const FGameplayAttributeData& OldPhysicalAttack) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, PhysicalAttack, OldPhysicalAttack);
}

void UPL_AttributeSet::OnRep_MagicalAttack(const FGameplayAttributeData& OldMagicalAttack) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, MagicalAttack, OldMagicalAttack);
}

void UPL_AttributeSet::OnRep_PhysicalDefense(const FGameplayAttributeData& OldPhysicalDefense) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, PhysicalDefense, OldPhysicalDefense);
}

void UPL_AttributeSet::OnRep_MagicalDefense(const FGameplayAttributeData& OldMagicalDefense) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, MagicalDefense, OldMagicalDefense);
}

void UPL_AttributeSet::OnRep_CriticalChance(const FGameplayAttributeData& OldCriticalChance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, CriticalChance, OldCriticalChance);
}

void UPL_AttributeSet::OnRep_CriticalDamage(const FGameplayAttributeData& OldCriticalDamage) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, CriticalDamage, OldCriticalDamage);
}

void UPL_AttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldAttackSpeed) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, AttackSpeed, OldAttackSpeed);
}

void UPL_AttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldMovementSpeed) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, MovementSpeed, OldMovementSpeed);
}

void UPL_AttributeSet::OnRep_MountSpeed(const FGameplayAttributeData& OldMountSpeed) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, MountSpeed, OldMountSpeed);
}

void UPL_AttributeSet::OnRep_CooldownReduction(const FGameplayAttributeData& OldCooldownReduction) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, CooldownReduction, OldCooldownReduction);
}

void UPL_AttributeSet::OnRep_HealthRegen(const FGameplayAttributeData& OldHealthRegen) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, HealthRegen, OldHealthRegen);
}

void UPL_AttributeSet::OnRep_EnergyRegen(const FGameplayAttributeData& OldEnergyRegen) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, EnergyRegen, OldEnergyRegen);
}

void UPL_AttributeSet::OnRep_StaminaRegen(const FGameplayAttributeData& OldStaminaRegen) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, StaminaRegen, OldStaminaRegen);
}

void UPL_AttributeSet::OnRep_HealingEffectiveness(const FGameplayAttributeData& OldHealingEffectiveness) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, HealingEffectiveness, OldHealingEffectiveness);
}

void UPL_AttributeSet::OnRep_ShieldingEffectiveness(const FGameplayAttributeData& OldShieldingEffectiveness) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, ShieldingEffectiveness, OldShieldingEffectiveness);
}

void UPL_AttributeSet::OnRep_DamageReduction(const FGameplayAttributeData& OldDamageReduction) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, DamageReduction, OldDamageReduction);
}

void UPL_AttributeSet::OnRep_PhysicalDamageReduction(const FGameplayAttributeData& OldPhysicalDamageReduction) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, PhysicalDamageReduction, OldPhysicalDamageReduction);
}

void UPL_AttributeSet::OnRep_MagicalDamageReduction(const FGameplayAttributeData& OldMagicalDamageReduction) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, MagicalDamageReduction, OldMagicalDamageReduction);
}

void UPL_AttributeSet::OnRep_CriticalDamageReduction(const FGameplayAttributeData& OldCriticalDamageReduction) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, CriticalDamageReduction, OldCriticalDamageReduction);
}

void UPL_AttributeSet::OnRep_PVPAttack(const FGameplayAttributeData& OldPVPAttack) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, PVPAttack, OldPVPAttack);
}

void UPL_AttributeSet::OnRep_PVPDefense(const FGameplayAttributeData& OldPVPDefense) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, PVPDefense, OldPVPDefense);
}

void UPL_AttributeSet::OnRep_PhysicalDefensePenetration(const FGameplayAttributeData& OldPhysicalDefensePenetration) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, PhysicalDefensePenetration, OldPhysicalDefensePenetration);
}

void UPL_AttributeSet::OnRep_MagicalDefensePenetration(const FGameplayAttributeData& OldMagicalDefensePenetration) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, MagicalDefensePenetration, OldMagicalDefensePenetration);
}

void UPL_AttributeSet::OnRep_DamageIncrease(const FGameplayAttributeData& OldDamageIncrease) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPL_AttributeSet, DamageIncrease, OldDamageIncrease);
}
