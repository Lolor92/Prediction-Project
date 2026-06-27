#pragma once

#include "CoreMinimal.h"
#include "PL_BaseCharacter.h"
#include "PL_PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USP_AbilityComponent;

UCLASS()
class PROJECTLOGOS_API APL_PlayerCharacter : public APL_BaseCharacter
{
	GENERATED_BODY()

public:
	APL_PlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Player-state ASC setup.
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	
	FORCEINLINE USpringArmComponent* GetSpringArm() const { return SpringArm.Get(); }
	FORCEINLINE UCameraComponent* GetCamera() const { return Camera.Get(); }
	
private:
	void InitializeAbilitySystem();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sync Prediction", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USP_AbilityComponent> AbilityComponent = nullptr;

	// Camera components.
	UPROPERTY(VisibleAnywhere, Category="Camera")
	TObjectPtr<USpringArmComponent> SpringArm = nullptr;

	UPROPERTY(VisibleAnywhere, Category="Camera")
	TObjectPtr<UCameraComponent> Camera = nullptr;
};
