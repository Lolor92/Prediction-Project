// Copyright ProjectLogos

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SyncCombatFunctionLibrary.generated.h"

class UAbilitySystemComponent;

UCLASS()
class SYNCCOMBAT_API USyncCombatFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Supports character-owned ASC first, then PlayerState-owned ASC.
	UFUNCTION(BlueprintCallable, Category="Combat|Ability")
	static UAbilitySystemComponent* GetAbilitySystemComponent(AActor* Actor);
};
