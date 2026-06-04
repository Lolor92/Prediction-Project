#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "PL_BaseCharacter.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;

UCLASS()
class PROJECTLOGOS_API APL_BaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APL_BaseCharacter();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAttributeSet* GetAttributeSet() const;

protected:
	// GAS references. Player characters receive these from PlayerState.
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;
};
