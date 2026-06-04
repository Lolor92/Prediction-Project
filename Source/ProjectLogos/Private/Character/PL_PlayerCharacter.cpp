#include "Character/PL_PlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/PL_PlayerState.h"


APL_PlayerCharacter::APL_PlayerCharacter()
{
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("Spring Arm");
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->TargetArmLength = 750.f;
	SpringArm->SetRelativeLocation(FVector(0.f, 25.f, 50.f));
	SpringArm->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(SpringArm);
	Camera->bUsePawnControlRotation = false;
}

void APL_PlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	// Server-side ASC initialization.
	InitializeAbilitySystem();
}

void APL_PlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	// Client-side ASC initialization.
	InitializeAbilitySystem();
}

void APL_PlayerCharacter::InitializeAbilitySystem()
{
	APL_PlayerState* TB_PlayerState = GetPlayerState<APL_PlayerState>();
	if (!TB_PlayerState) return;

	// Player characters use the PlayerState-owned ASC.
	AbilitySystemComponent = TB_PlayerState->GetAbilitySystemComponent();
	AttributeSet = TB_PlayerState->GetAttributeSet();

	if (!AbilitySystemComponent) return;

	AbilitySystemComponent->InitAbilityActorInfo(TB_PlayerState, this);
}
