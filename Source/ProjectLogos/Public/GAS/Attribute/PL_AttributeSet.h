// Copyright ProjectLogos

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include <AbilitySystemComponent.h>
#include "PL_AttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \

USTRUCT()
struct FEffectProperties
{
	GENERATED_BODY()

	FEffectProperties(){}

	FGameplayEffectContextHandle EffectContextHandle;

	UPROPERTY()
	UAbilitySystemComponent* SourceASC = nullptr;

	UPROPERTY()
	AActor* SourceAvatarActor = nullptr;

	UPROPERTY()
	AController* SourceController = nullptr;

	UPROPERTY()
	ACharacter* SourceCharacter = nullptr;

	UPROPERTY()
	UAbilitySystemComponent* TargetASC = nullptr;

	UPROPERTY()
	AActor* TargetAvatarActor = nullptr;

	UPROPERTY()
	AController* TargetController = nullptr;

	UPROPERTY()
	ACharacter* TargetCharacter = nullptr;
};

UCLASS()
class PROJECTLOGOS_API UPL_AttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	
	// Vital Attributes
	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_Health, Category = "Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, Health);

	UPROPERTY(BlueprintReadOnly ,ReplicatedUsing = OnRep_MaxHealth, Category = "Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, MaxHealth);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Energy, Category = "Attributes")
	FGameplayAttributeData Energy;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, Energy);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxEnergy, Category = "Attributes")
	FGameplayAttributeData MaxEnergy;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, MaxEnergy);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "Attributes")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, Stamina);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, Category = "Attributes")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, MaxStamina);

	// Meta Attributes
	UPROPERTY(BlueprintReadOnly, Category = "Meta Attributes")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, IncomingDamage);

	// Primary Attributes
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "Attributes")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, Strength);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Intelligence, Category = "Attributes")
	FGameplayAttributeData Intelligence;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, Intelligence);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Agility, Category = "Attributes")
	FGameplayAttributeData Agility;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, Agility);

	// Secondary Attributes
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalAttack, Category = "Attributes")
	FGameplayAttributeData PhysicalAttack;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, PhysicalAttack);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicalAttack, Category = "Attributes")
	FGameplayAttributeData MagicalAttack;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, MagicalAttack);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalDefense, Category = "Attributes")
	FGameplayAttributeData PhysicalDefense;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, PhysicalDefense);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicalDefense, Category = "Attributes")
	FGameplayAttributeData MagicalDefense;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, MagicalDefense);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalChance, Category = "Attributes")
	FGameplayAttributeData CriticalChance;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, CriticalChance);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalDamage, Category = "Attributes")
	FGameplayAttributeData CriticalDamage;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, CriticalDamage);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackSpeed, Category = "Attributes")
	FGameplayAttributeData AttackSpeed; // Casting speed for spells and attack speed for physical attacks
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, AttackSpeed);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MovementSpeed, Category = "Attributes")
	FGameplayAttributeData MovementSpeed;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, MovementSpeed);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MountSpeed, Category = "Attributes")
	FGameplayAttributeData MountSpeed;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, MountSpeed);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CooldownReduction, Category = "Attributes")
	FGameplayAttributeData CooldownReduction;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, CooldownReduction);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HealthRegen, Category = "Attributes")
	FGameplayAttributeData HealthRegen;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, HealthRegen);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EnergyRegen, Category = "Attributes")
	FGameplayAttributeData EnergyRegen;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, EnergyRegen);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaRegen, Category = "Attributes")
	FGameplayAttributeData StaminaRegen;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, StaminaRegen);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HealingEffectiveness, Category = "Attributes")
	FGameplayAttributeData HealingEffectiveness;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, HealingEffectiveness);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ShieldingEffectiveness, Category = "Attributes")
	FGameplayAttributeData ShieldingEffectiveness;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, ShieldingEffectiveness);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DamageReduction, Category = "Attributes")
	FGameplayAttributeData DamageReduction;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, DamageReduction);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalDamageReduction, Category = "Attributes")
	FGameplayAttributeData PhysicalDamageReduction;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, PhysicalDamageReduction);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicalDamageReduction, Category = "Attributes")
	FGameplayAttributeData MagicalDamageReduction;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, MagicalDamageReduction);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalDamageReduction, Category = "Attributes")
	FGameplayAttributeData CriticalDamageReduction;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, CriticalDamageReduction);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PVPAttack, Category = "Attributes")
	FGameplayAttributeData PVPAttack;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, PVPAttack);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PVPDefense, Category = "Attributes")
	FGameplayAttributeData PVPDefense;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, PVPDefense);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalDefensePenetration, Category = "Attributes")
	FGameplayAttributeData PhysicalDefensePenetration; // got to here
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, PhysicalDefensePenetration);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicalDefensePenetration, Category = "Attributes")
	FGameplayAttributeData MagicalDefensePenetration;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, MagicalDefensePenetration);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DamageIncrease, Category = "Attributes")
	FGameplayAttributeData DamageIncrease;
	ATTRIBUTE_ACCESSORS(UPL_AttributeSet, DamageIncrease);

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth) const;

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const;

	UFUNCTION()
	void OnRep_Energy(const FGameplayAttributeData& OldEnergy) const;

	UFUNCTION()
	void OnRep_MaxEnergy(const FGameplayAttributeData& OldMaxEnergy) const;

	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldStamina) const;

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina) const;

	UFUNCTION()
	void OnRep_Strength(const FGameplayAttributeData& OldStrength) const;

	UFUNCTION()
	void OnRep_Intelligence(const FGameplayAttributeData& OldIntelligence) const;

	UFUNCTION()
	void OnRep_Agility(const FGameplayAttributeData& OldAgility) const;

	UFUNCTION()
	void OnRep_PhysicalAttack(const FGameplayAttributeData& OldPhysicalAttack) const;

	UFUNCTION()
    void OnRep_MagicalAttack(const FGameplayAttributeData& OldMagicalAttack) const;

    UFUNCTION()
    void OnRep_PhysicalDefense(const FGameplayAttributeData& OldPhysicalDefense) const;

    UFUNCTION()
    void OnRep_MagicalDefense(const FGameplayAttributeData& OldMagicalDefense) const;

    UFUNCTION()
    void OnRep_CriticalChance(const FGameplayAttributeData& OldCriticalChance) const;

    UFUNCTION()
    void OnRep_CriticalDamage(const FGameplayAttributeData& OldCriticalDamage) const;

    UFUNCTION()
    void OnRep_AttackSpeed(const FGameplayAttributeData& OldAttackSpeed) const;

    UFUNCTION()
    void OnRep_MovementSpeed(const FGameplayAttributeData& OldMovementSpeed) const;

    UFUNCTION()
    void OnRep_MountSpeed(const FGameplayAttributeData& OldMountSpeed) const;

    UFUNCTION()
    void OnRep_CooldownReduction(const FGameplayAttributeData& OldCooldownReduction) const;

    UFUNCTION()
    void OnRep_HealthRegen(const FGameplayAttributeData& OldHealthRegen) const;

    UFUNCTION()
    void OnRep_EnergyRegen(const FGameplayAttributeData& OldEnergyRegen) const;

    UFUNCTION()
    void OnRep_StaminaRegen(const FGameplayAttributeData& OldStaminaRegen) const;

    UFUNCTION()
    void OnRep_HealingEffectiveness(const FGameplayAttributeData& OldHealingEffectiveness) const;

    UFUNCTION()
    void OnRep_ShieldingEffectiveness(const FGameplayAttributeData& OldShieldingEffectiveness) const;

	UFUNCTION()
	void OnRep_DamageReduction(const FGameplayAttributeData& OldDamageReduction) const;
	
    UFUNCTION()
    void OnRep_PhysicalDamageReduction(const FGameplayAttributeData& OldPhysicalDamageReduction) const;

    UFUNCTION()
    void OnRep_MagicalDamageReduction(const FGameplayAttributeData& OldMagicalDamageReduction) const;

    UFUNCTION()
    void OnRep_CriticalDamageReduction(const FGameplayAttributeData& OldCriticalDamageReduction) const;

    UFUNCTION()
    void OnRep_PVPAttack(const FGameplayAttributeData& OldPVPAttack) const;

    UFUNCTION()
    void OnRep_PVPDefense(const FGameplayAttributeData& OldPVPDefense) const;

    UFUNCTION()
    void OnRep_PhysicalDefensePenetration(const FGameplayAttributeData& OldPhysicalDefensePenetration) const;

    UFUNCTION()
    void OnRep_MagicalDefensePenetration(const FGameplayAttributeData& OldMagicalDefensePenetration) const;

	UFUNCTION()
	void OnRep_DamageIncrease(const FGameplayAttributeData& OldDamageIncrease) const;

private:
	void SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props) const;
};
