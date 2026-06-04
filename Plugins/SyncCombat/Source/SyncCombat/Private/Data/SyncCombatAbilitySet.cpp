// Copyright ProjectLogos


#include "Data/SyncCombatAbilitySet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"

void FSyncCombatAbilitySetGrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (!Handle.IsValid()) return;

	AbilitySpecHandles.Add(Handle);
}

void FSyncCombatAbilitySetGrantedHandles::TakeFromAbilitySystem(UAbilitySystemComponent* AbilitySystemComponent)
{
	if (AbilitySystemComponent)
	{
		for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitySpecHandles)
		{
			if (!AbilitySpecHandle.IsValid()) continue;

			AbilitySystemComponent->ClearAbility(AbilitySpecHandle);
		}
	}

	AbilitySpecHandles.Reset();
}

void USyncCombatAbilitySet::GiveToAbilitySystem(UAbilitySystemComponent* AbilitySystemComponent,
	FSyncCombatAbilitySetGrantedHandles* OutGrantedHandles, UObject* SourceObject) const
{
	if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative()) return;

	for (const FSyncCombatAbilityGrant& AbilityGrant : Abilities)
	{
		if (!AbilityGrant.AbilityClass) continue;

		const bool bAlreadyGranted = AbilitySystemComponent->GetActivatableAbilities().ContainsByPredicate(
			[&AbilityGrant](const FGameplayAbilitySpec& AbilitySpec)
			{
				return AbilitySpec.Ability
					&& AbilitySpec.Ability->GetClass() == AbilityGrant.AbilityClass;
			});

		if (bAlreadyGranted) continue;

		const FGameplayAbilitySpec AbilitySpec(AbilityGrant.AbilityClass, AbilityGrant.Level, INDEX_NONE, SourceObject);
		const FGameplayAbilitySpecHandle AbilitySpecHandle = AbilitySystemComponent->GiveAbility(AbilitySpec);
		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAbilitySpecHandle(AbilitySpecHandle);
		}
	}
}
