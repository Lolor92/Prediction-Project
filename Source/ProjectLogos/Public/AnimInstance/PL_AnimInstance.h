#pragma once

#include "CoreMinimal.h"
#include "AnimInstance/SyncAbilityMotionAnimInstance.h"
#include "PL_AnimInstance.generated.h"


UCLASS()
class PROJECTLOGOS_API UPL_AnimInstance : public USyncAbilityMotionAnimInstance
{
	GENERATED_BODY()
	
public:
	UPL_AnimInstance();
	
	// Anim instance lifecycle.
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|State");
	bool bIsBlocking = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|State");
	bool bIsKnockdown = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|State");
	bool bIsFlinching = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|State");
	bool bIsFrozen = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|State");
	bool bIsStunned = false;
};
