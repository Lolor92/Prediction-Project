#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SP_FunctionLibrary.generated.h"

class UAbilitySystemComponent;

UCLASS()
class SYNCPREDICTION_API USP_FunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category="Sync Prediction|Ability System")
	static UAbilitySystemComponent* FindAbilitySystemComponent(AActor* Actor);
};
