#include "Data/SP_AbilitySet.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"

void FSP_AbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

void FSP_AbilitySet_GrantedHandles::TakeFromAbilitySystem(UAbilitySystemComponent* ASC)
{
	if (!ASC) return;

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	AbilitySpecHandles.Reset();
}

void USP_AbilitySet::GiveToAbilitySystem(UAbilitySystemComponent* ASC, FSP_AbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject) const
{
	if (!ASC || !ASC->IsOwnerActorAuthoritative()) return;

	for (const FSP_AbilitySet_GameplayAbility& AbilityToGrant : GrantedGameplayAbilities)
	{
		if (!AbilityToGrant.Ability) continue;

		FGameplayAbilitySpec AbilitySpec(AbilityToGrant.Ability, AbilityToGrant.AbilityLevel, INDEX_NONE, SourceObject);

		const FGameplayAbilitySpecHandle AbilitySpecHandle = ASC->GiveAbility(AbilitySpec);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAbilitySpecHandle(AbilitySpecHandle);
		}
	}
}