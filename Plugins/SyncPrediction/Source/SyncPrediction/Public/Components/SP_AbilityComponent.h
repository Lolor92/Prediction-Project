// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/SP_AbilitySet.h"
#include "SP_AbilityComponent.generated.h"


class UAbilitySystemComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SYNCPREDICTION_API USP_AbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USP_AbilityComponent();
	
	UFUNCTION(BlueprintCallable)
	void InitializeWithAbilitySystem(UAbilitySystemComponent* InASC);
	
	UFUNCTION(BlueprintCallable)
	void GrantAbilitySets();

	UFUNCTION(BlueprintCallable)
	void RemoveGrantedAbilitySets();
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Sync Prediction|Abilities")
	TArray<TObjectPtr<USP_AbilitySet>> AbilitySetsToGrant;

private:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	FSP_AbilitySet_GrantedHandles GrantedHandles;

	bool bAbilitySetsGranted = false;
};
