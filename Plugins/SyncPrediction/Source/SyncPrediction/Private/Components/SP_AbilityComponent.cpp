#include "Components/SP_AbilityComponent.h"
#include "AbilitySystemComponent.h"
#include "FunctionLibrary/SP_FunctionLibrary.h"

USP_AbilityComponent::USP_AbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void USP_AbilityComponent::InitializeWithAbilitySystem(UAbilitySystemComponent* InASC)
{
	AbilitySystemComponent = USP_FunctionLibrary::FindAbilitySystemComponent(GetOwner());

	if (!AbilitySystemComponent) return;

	GrantAbilitySets();
}

void USP_AbilityComponent::GrantAbilitySets()
{
	if (!AbilitySystemComponent || bAbilitySetsGranted) return;

	if (!AbilitySystemComponent->IsOwnerActorAuthoritative()) return;

	for (const USP_AbilitySet* AbilitySet : AbilitySetsToGrant)
	{
		if (!AbilitySet) continue;

		AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, &GrantedHandles, GetOwner());
	}

	bAbilitySetsGranted = true;
}

void USP_AbilityComponent::RemoveGrantedAbilitySets()
{
	if (!AbilitySystemComponent || !bAbilitySetsGranted)
	{
		return;
	}

	if (!AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		return;
	}

	GrantedHandles.TakeFromAbilitySystem(AbilitySystemComponent);
	bAbilitySetsGranted = false;
}

void USP_AbilityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveGrantedAbilitySets();
	
	Super::EndPlay(EndPlayReason);
}
