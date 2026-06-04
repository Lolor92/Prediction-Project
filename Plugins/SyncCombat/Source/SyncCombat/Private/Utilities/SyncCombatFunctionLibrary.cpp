// Copyright ProjectLogos

#include "Utilities/SyncCombatFunctionLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"

UAbilitySystemComponent* USyncCombatFunctionLibrary::GetAbilitySystemComponent(AActor* Actor)
{
	if (!Actor) return nullptr;

	// Project characters cache their ASC pointer after initialization.
	if (const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Actor))
	{
		if (UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface->GetAbilitySystemComponent())
		{
			return AbilitySystemComponent;
		}
	}

	if (const ACharacter* Character = Cast<ACharacter>(Actor))
	{
		if (APlayerState* PlayerState = Character->GetPlayerState<APlayerState>())
		{
			if (UAbilitySystemComponent* PlayerStateAbilitySystem =
				PlayerState->FindComponentByClass<UAbilitySystemComponent>())
			{
				return PlayerStateAbilitySystem;
			}

			if (const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(PlayerState))
			{
				return AbilitySystemInterface->GetAbilitySystemComponent();
			}
		}

		return nullptr;
	}

	// Fallback for non-character actors that own an ASC component directly.
	if (UAbilitySystemComponent* ActorAbilitySystem = Actor->FindComponentByClass<UAbilitySystemComponent>())
	{
		return ActorAbilitySystem;
	}

	return nullptr;
}
