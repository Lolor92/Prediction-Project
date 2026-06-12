#pragma once

#include "CoreMinimal.h"
#include "SI_WidgetBase.h"
#include "SI_PlayerInventoryWidget.generated.h"


class USI_InventoryGridWidget;
class USI_InventoryUIManager;

UCLASS()
class SYNCINVENTORY_API USI_PlayerInventoryWidget : public USI_WidgetBase
{
	GENERATED_BODY()
	
public:
	USI_PlayerInventoryWidget();
};
