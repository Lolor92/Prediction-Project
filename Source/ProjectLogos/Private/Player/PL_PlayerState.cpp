#include "Player/PL_PlayerState.h"
#include "GAS/ASC/PL_AbilitySystemComponent.h"
#include "GAS/Attribute/PL_AttributeSet.h"

APL_PlayerState::APL_PlayerState()
{
	SetNetUpdateFrequency(100.f);

	AbilitySystemComponent = CreateDefaultSubobject<UPL_AbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UPL_AttributeSet>("AttributeSet");
}
