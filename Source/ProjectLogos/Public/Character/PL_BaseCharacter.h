#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
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
	void BindBodyCollisionPassthroughTag();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Collision|Escape")
	FGameplayTag BodyCollisionPassthroughTag;

	UPROPERTY(EditDefaultsOnly, Category = "Collision|Escape")
	TArray<TEnumAsByte<ECollisionChannel>> BodyCollisionPassthroughChannels;

	// GAS references. Player characters receive these from PlayerState.
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

private:
	void OnBodyCollisionPassthroughTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	void SetBodyCollisionPassthroughEnabled(bool bEnabled);

	UPROPERTY(Transient)
	bool bBodyCollisionPassthroughActive = false;

	UPROPERTY(Transient)
	TArray<TEnumAsByte<ECollisionResponse>> SavedCapsuleResponses;

	UPROPERTY(Transient)
	TArray<TEnumAsByte<ECollisionResponse>> SavedMeshResponses;

	TWeakObjectPtr<UAbilitySystemComponent> BoundBodyCollisionASC;
	FDelegateHandle BodyCollisionPassthroughDelegateHandle;
};
