#include "FunctionLibrary/SP_FunctionLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"

UAbilitySystemComponent* USP_FunctionLibrary::FindAbilitySystemComponent(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	// 1. Actor implements IAbilitySystemInterface.
	if (const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Actor))
	{
		if (UAbilitySystemComponent* ASC = AbilitySystemInterface->GetAbilitySystemComponent())
		{
			return ASC;
		}
	}

	// 2. Actor directly owns an ASC component.
	if (UAbilitySystemComponent* ASC = Actor->FindComponentByClass<UAbilitySystemComponent>())
	{
		return ASC;
	}

	// 3. Pawn's PlayerState implements IAbilitySystemInterface or owns ASC.
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		APlayerState* PlayerState = Pawn->GetPlayerState();

		if (!PlayerState)
		{
			return nullptr;
		}

		if (const IAbilitySystemInterface* PlayerStateAbilitySystemInterface = Cast<IAbilitySystemInterface>(PlayerState))
		{
			if (UAbilitySystemComponent* ASC = PlayerStateAbilitySystemInterface->GetAbilitySystemComponent())
			{
				return ASC;
			}
		}

		if (UAbilitySystemComponent* ASC = PlayerState->FindComponentByClass<UAbilitySystemComponent>())
		{
			return ASC;
		}
	}

	return nullptr;
}
