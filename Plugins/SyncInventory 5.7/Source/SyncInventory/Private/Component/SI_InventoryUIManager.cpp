#include "Component/SI_InventoryUIManager.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "UObject/ConstructorHelpers.h"
#include "Component/SI_InventoryComponent.h"
#include "Tags/SI_NativeGameplayTags.h"
#include "UI/Popups/SI_DeleteItemWidget.h"
#include "UI/Popups/SI_SplitStackWidget.h"
#include "UI/Widgets/SI_PlayerInventoryWidget.h"
#include "Data/Definitions/SI_ItemDefinition.h"
#include "UI/Notifications/SI_ItemAddedNotificationHostWidget.h"


USI_InventoryUIManager::USI_InventoryUIManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FClassFinder<USI_DeleteItemWidget> DeleteItemWidgetFinder(
		TEXT("/SyncInventory/UI/Popup/WBP_DeleteItemWidget"));
	if (DeleteItemWidgetFinder.Succeeded())
	{
		DeleteItemWidgetClass = DeleteItemWidgetFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<USI_SplitStackWidget> SplitStackWidgetFinder(
		TEXT("/SyncInventory/UI/Popup/WBP_SplitStackWidget"));
	if (SplitStackWidgetFinder.Succeeded())
	{
		SplitStackWidgetClass = SplitStackWidgetFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<USI_ItemAddedNotificationHostWidget> NotificationHostWidgetFinder(
		TEXT("/SyncInventory/UI/Notification/WBP_ItemAddedNotificationHost"));
	if (NotificationHostWidgetFinder.Succeeded())
	{
		ItemAddedNotificationHostWidgetClass = NotificationHostWidgetFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<USI_ItemAddedNotificationWidget> NotificationWidgetFinder(
		TEXT("/SyncInventory/UI/Notification/WBP_ItemAddedNotification"));
	if (NotificationWidgetFinder.Succeeded())
	{
		InventoryMessageNotificationWidgetClass = NotificationWidgetFinder.Class;
	}
}

void USI_InventoryUIManager::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeUIManager();
	
	for (const FSI_ContainerConfig& Config : InventoryComponent->GetContainerConfigs())
	{
		if (!Config.bStartVisibleOnSpawn || !Config.ContainerId.IsValid()) continue;
		
		ToggleWidget(Config.ContainerId);
	}
	
}

void USI_InventoryUIManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InventoryComponent)
	{
		InventoryComponent->OnSlotChanged.RemoveAll(this);
		
		InventoryComponent->OnAddItemRequestCompleted.RemoveDynamic(
			this, &ThisClass::HandleAddItemRequestCompleted);

		InventoryComponent->OnMoveItemRequestCompleted.RemoveDynamic(
			this, &ThisClass::HandleMoveItemRequestCompleted);
	}

	if (AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->RegisterGenericGameplayTagEvent().RemoveAll(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

void USI_InventoryUIManager::InitializeUIManager()
{
	InventoryComponent = GetOwner() ? GetOwner()->FindComponentByClass<USI_InventoryComponent>() : nullptr;
	if (!InventoryComponent) return;
	
	InventoryComponent->OnSlotChanged.AddUObject(this, &USI_InventoryUIManager::HandleSlotsChanged);
	
	InventoryComponent->OnAddItemRequestCompleted.AddDynamic(
		this, &ThisClass::HandleAddItemRequestCompleted);

	InventoryComponent->OnMoveItemRequestCompleted.AddDynamic(
		this, &ThisClass::HandleMoveItemRequestCompleted);
	
	if (!AbilitySystemComponent.IsValid())
	{
		if (!GetAbilitySystemComponent()) return;
	}
	
	AbilitySystemComponent->RegisterGenericGameplayTagEvent().AddUObject(this, 
		&ThisClass::HandleGameplayTagChanged);
}

void USI_InventoryUIManager::HandleSlotsChanged(const FGameplayTag ContainerTag, const TArray<int32>& SlotIndices)
{
	if (!ContainerTag.IsValid() || SlotIndices.Num() == 0) return;
	
	TObjectPtr<USI_WidgetBase>* FoundWindow = ActiveWidget.Find(ContainerTag);
	if (!FoundWindow) return;
	
	(*FoundWindow)->RefreshSlots(SlotIndices);
}

void USI_InventoryUIManager::HandleAddItemRequestCompleted(const FSI_AddItemResponse& Response)
{
	if (!InventoryComponent) return;
	if (Response.Result == ESI_AddItemResult::Failed) return;
	if (Response.AddedQuantity <= 0) return;
	if (!Response.ContainerId.IsValid()) return;
	if (!Response.ItemTag.IsValid()) return;

	const FSI_ContainerConfig* Config = InventoryComponent->GetContainerConfig(Response.ContainerId);
	if (!Config) return;
	if (!Config->bShowItemAddedNotification) return;
	if (!Config->ItemAddedNotificationWidgetClass) return;

	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController || !PlayerController->IsLocalController() || !PlayerController->GetLocalPlayer()) return;

	const USI_ItemDatabase* ItemDatabase = InventoryComponent->GetItemDatabase();
	const USI_ItemDefinition* ItemDefinition = ItemDatabase
		? ItemDatabase->GetItemDefinitionByTag(Response.ItemTag)
		: nullptr;

	if (!ItemDefinition) return;

	EnsureItemAddedNotificationHost();

	if (!ActiveItemAddedNotificationHostWidget) return;

	FSI_ItemAddedNotificationData NotificationData;
	NotificationData.ItemTag = Response.ItemTag;
	NotificationData.ItemName = ItemDefinition->ItemName;
	NotificationData.ItemIcon = ItemDefinition->ItemIcon;
	NotificationData.ItemRarityTag = ItemDefinition->GetItemRarityTag();
	NotificationData.QuantityAdded = Response.AddedQuantity;

	ActiveItemAddedNotificationHostWidget->QueueNotification(NotificationData, Config->ItemAddedNotificationWidgetClass);
}

void USI_InventoryUIManager::HandleMoveItemRequestCompleted(const FSI_MoveItemResponse& Response)
{
	if (Response.Result != ESI_MoveItemResult::Failed) return;

	if (Response.FailureReason == ESI_InventoryFailureReason::LevelRequirementNotMet)
	{
		QueueInventoryMessageNotification(
			NSLOCTEXT("SyncInventory", "CannotEquipLevelRequirement", "Cannot equip: level requirement not met."));
	}
}

void USI_InventoryUIManager::ToggleWidget(const FGameplayTag& ContainerId)
{
	if (ActiveWidget.Find(ContainerId))
	{
		CloseDeleteItemWidget();
		CloseWidget(ContainerId);
		return;
	}
	
	OpenWidget(ContainerId);
}

void USI_InventoryUIManager::OpenWidget(const FGameplayTag& ContainerId)
{
	if (ActiveWidget.Contains(ContainerId)) return;
	
	if (!InventoryComponent) return;
	
	const FSI_ContainerConfig* Config = InventoryComponent->GetContainerConfig(ContainerId);
	if (!Config || !Config->WidgetClass) return;
	
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController || !PlayerController->IsLocalController() || !PlayerController->GetLocalPlayer()) return;
	
	USI_WidgetBase* NewWidget = CreateWidget<USI_WidgetBase>(PlayerController, Config->WidgetClass);
	if (!NewWidget) return;
	
	NewWidget->InitWidget(this, ContainerId, Config->MaxSlots, Config->Columns);
	NewWidget->AddToViewport();
	
	ActiveWidget.Add(ContainerId, NewWidget);
	
	if (!Config->bAffectsInputMode) return;
	
	RefreshInputMode();
}

void USI_InventoryUIManager::CloseWidget(const FGameplayTag& ContainerId)
{
	if (auto FoundWidget = ActiveWidget.Find(ContainerId))
	{
		if (*FoundWidget)
		{
			(*FoundWidget)->RemoveFromParent();
		}
		
		ActiveWidget.Remove(ContainerId);
		
		const FSI_ContainerConfig* Config = InventoryComponent->GetContainerConfig(ContainerId);
		if (!Config || !Config->bAffectsInputMode) return;
		
		RefreshInputMode();
	}
}

void USI_InventoryUIManager::OpenDeleteItemWidget(const FGameplayTag& ContainerId, const int32 SlotIndex,
	const FVector2D& ScreenPosition)
{
	if (!InventoryComponent || !DeleteItemWidgetClass) return;
	if (!ContainerId.IsValid() || SlotIndex < 0) return;
	
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController() || !PC->GetLocalPlayer()) return;
	
	const FSI_InventoryEntry* Entry = InventoryComponent->FindEntryAtSlot(ContainerId, SlotIndex);
	if (!Entry || Entry->Quantity <= 0) return;
	
	CloseDeleteItemWidget();
	
	USI_DeleteItemWidget* DeleteWidget = CreateWidget<USI_DeleteItemWidget>(PC, DeleteItemWidgetClass);
	if (!DeleteWidget) return;
	
	DeleteWidget->AddToViewport(100);
	DeleteWidget->InitDeleteItemWidget(this, InventoryComponent, ContainerId, SlotIndex);
	DeleteWidget->SetPopScreenPosition(ScreenPosition);
	
	ActiveDeleteItemWidget = DeleteWidget;
}

void USI_InventoryUIManager::CloseDeleteItemWidget()
{
	if (ActiveDeleteItemWidget)
	{
		ActiveDeleteItemWidget->RemoveFromParent();
		ActiveDeleteItemWidget = nullptr;
	}
}

void USI_InventoryUIManager::OpenSplitStackWidget(const FGameplayTag& ContainerId, const int32 SlotIndex,
	const FVector2D& ScreenPosition)
{
	if (!InventoryComponent || !SplitStackWidgetClass) return;
	if (!ContainerId.IsValid() || SlotIndex < 0) return;
	
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController() || !PC->GetLocalPlayer()) return;
	
	const FSI_InventoryEntry* Entry = InventoryComponent->FindEntryAtSlot(ContainerId, SlotIndex);
	if (!Entry || Entry->Quantity <= 1) return;
	
	const USI_ItemDatabase* ItemDatabase = InventoryComponent->GetItemDatabase();
	if (!ItemDatabase) return;
	
	const USI_ItemDefinition* Def = ItemDatabase->GetItemDefinitionByTag(Entry->ItemTag);
	if (!Def || !Def->bStackable) return;
	
	CloseSplitStackWidget();
	CloseDeleteItemWidget();
	
	USI_SplitStackWidget* SplitWidget = CreateWidget<USI_SplitStackWidget>(PC, SplitStackWidgetClass);
	if (!SplitWidget) return;
	
	SplitWidget->AddToViewport(100);
	SplitWidget->InitSplitStackWidget(this, InventoryComponent, ContainerId, SlotIndex, Entry->Quantity);
	SplitWidget->SetPopupScreenPosition(ScreenPosition);
	
	ActiveSplitStackWidget = SplitWidget;
}

void USI_InventoryUIManager::CloseSplitStackWidget()
{
	if (ActiveSplitStackWidget)
	{
		ActiveSplitStackWidget->RemoveFromParent();
		ActiveSplitStackWidget = nullptr;
	}
}

void USI_InventoryUIManager::HandleInputPressed(FGameplayTag InputTag)
{
	if (!InventoryComponent) return;
	if (!InputTag.IsValid()) return;
	
	for (const FSI_ContainerConfig& Config : InventoryComponent->GetContainerConfigs())
	{
		if (!Config.ContainerId.IsValid()) continue;
		if (!Config.bCanReceiveInput) continue;
		
		TObjectPtr<USI_WidgetBase>* FoundWindow = ActiveWidget.Find(Config.ContainerId);
		if (!FoundWindow && !Config.bReceiveInputWhenWindowClosed) continue;
		
		const USI_SlotLayoutConfig* SlotConfig = InventoryComponent->GetSlotLayoutConfig(Config.ContainerId);
		if (!SlotConfig) continue;
		
		for (auto LayoutEntry : SlotConfig->SlotEntries)
		{
			if (!LayoutEntry.InputTag.IsValid()) continue;
			if (!LayoutEntry.InputTag.MatchesTagExact(InputTag)) continue;
			
			if (FoundWindow && *FoundWindow)
			{
				(*FoundWindow)->PressedFeedback(LayoutEntry.SlotIndex);
			}
			
			InventoryComponent->RequestUseItem(Config.ContainerId, LayoutEntry.SlotIndex, true);
			return;
		}
	}
}

void USI_InventoryUIManager::HandleInputReleased(FGameplayTag InputTag)
{
	if (!InventoryComponent) return;
	if (!InputTag.IsValid()) return;
	
	for (const FSI_ContainerConfig& Config : InventoryComponent->GetContainerConfigs())
	{
		if (!Config.ContainerId.IsValid()) continue;
		if (!Config.bCanReceiveInput) continue;
		
		TObjectPtr<USI_WidgetBase>* FoundWindow = ActiveWidget.Find(Config.ContainerId);
		if (!FoundWindow && !Config.bReceiveInputWhenWindowClosed) continue;
		
		const USI_SlotLayoutConfig* SlotConfig = InventoryComponent->GetSlotLayoutConfig(Config.ContainerId);
		if (!SlotConfig) continue;
		
		for (auto LayoutEntry : SlotConfig->SlotEntries)
		{
			if (!LayoutEntry.InputTag.IsValid()) continue;
			if (!LayoutEntry.InputTag.MatchesTagExact(InputTag)) continue;
			
			if (FoundWindow && *FoundWindow)
			{
				(*FoundWindow)->ResetPressedFeedback(LayoutEntry.SlotIndex);
			}
			
			InventoryComponent->RequestUseItem(Config.ContainerId, LayoutEntry.SlotIndex, false);
			return;
		}
	}
}

void USI_InventoryUIManager::UpdateInputMode(bool bInventoryOpen)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController) return;
	
	if (bInventoryOpen)
	{
		PlayerController->bShowMouseCursor = true;
		
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockInFullscreen);
		
		PlayerController->SetInputMode(InputMode);
		return;
	}
	
	PlayerController->bShowMouseCursor = false;
	
	FInputModeGameOnly InputMode;
	PlayerController->SetInputMode(InputMode);
}

void USI_InventoryUIManager::RefreshInputMode()
{
	if (!InventoryComponent)
	{
		UpdateInputMode(false);
		return;
	}

	for (const TPair<FGameplayTag, TObjectPtr<USI_WidgetBase>>& Pair : ActiveWidget)
	{
		if (!Pair.Value) continue;

		const FSI_ContainerConfig* Config = InventoryComponent->GetContainerConfig(Pair.Key);
		if (Config && Config->bAffectsInputMode)
		{
			UpdateInputMode(true);
			return;
		}
	}

	UpdateInputMode(false);
}

void USI_InventoryUIManager::EnsureItemAddedNotificationHost()
{
	if (ActiveItemAddedNotificationHostWidget) return;
	if (!ItemAddedNotificationHostWidgetClass) return;

	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController || !PlayerController->IsLocalController() || !PlayerController->GetLocalPlayer()) return;

	ActiveItemAddedNotificationHostWidget = CreateWidget<USI_ItemAddedNotificationHostWidget>(
		PlayerController, ItemAddedNotificationHostWidgetClass);

	if (!ActiveItemAddedNotificationHostWidget) return;

	ActiveItemAddedNotificationHostWidget->AddToViewport(ItemAddedNotificationZOrder);
	ActiveItemAddedNotificationHostWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void USI_InventoryUIManager::QueueInventoryMessageNotification(const FText& MessageText)
{
	if (MessageText.IsEmpty()) return;

	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController || !PlayerController->IsLocalController() || !PlayerController->GetLocalPlayer()) return;

	EnsureItemAddedNotificationHost();
	if (!ActiveItemAddedNotificationHostWidget) return;
	if (!InventoryComponent) return;

	TSubclassOf<USI_ItemAddedNotificationWidget> NotificationWidgetClass = InventoryMessageNotificationWidgetClass;
	if (!NotificationWidgetClass)
	{
		for (const FSI_ContainerConfig& Config : InventoryComponent->GetContainerConfigs())
		{
			if (Config.ItemAddedNotificationWidgetClass)
			{
				NotificationWidgetClass = Config.ItemAddedNotificationWidgetClass;
				break;
			}
		}
	}

	if (!NotificationWidgetClass) return;

	FSI_ItemAddedNotificationData NotificationData;
	NotificationData.MessageText = MessageText;
	NotificationData.bUseMessageText = true;

	ActiveItemAddedNotificationHostWidget->QueueNotification(NotificationData, NotificationWidgetClass);
}

void USI_InventoryUIManager::HandleGameplayTagChanged(FGameplayTag ChangedTag, int32 NewCount)
{
	if (!ChangedTag.IsValid()) return;
	if (!InventoryComponent) return;
	if (!ChangedTag.MatchesTag(TAG_SI_Cooldown)) return;
	if (ActiveWidget.IsEmpty()) return;

	const USI_ItemDatabase* ItemDatabase = InventoryComponent->GetItemDatabase();
	if (!ItemDatabase) return;

	for (TPair<FGameplayTag, TObjectPtr<USI_WidgetBase>>& Pair : ActiveWidget)
	{
		USI_WidgetBase* Widget = Pair.Value;
		if (!Widget || !Pair.Key.IsValid()) continue;

		const FSI_ContainerConfig* Config = InventoryComponent->GetContainerConfig(Pair.Key);
		const int32 MaxSlots = Config ? Config->MaxSlots : 0;
		if (MaxSlots <= 0) continue;

		TArray<int32> MatchingSlotIndices;

		for (int32 SlotIndex = 0; SlotIndex < MaxSlots; SlotIndex++)
		{
			const FSI_InventoryEntry* Entry = InventoryComponent->FindEntryAtSlot(Pair.Key, SlotIndex);
			if (!Entry) continue;

			const USI_ItemDefinition* Def = ItemDatabase->GetItemDefinitionByTag(Entry->ItemTag);
			if (!Def || !Def->CooldownGameplayEffect) continue;

			MatchingSlotIndices.Add(SlotIndex);
		}

		if (MatchingSlotIndices.Num() > 0)
		{
			Widget->RefreshSlots(MatchingSlotIndices);
		}
	}
}

UAbilitySystemComponent* USI_InventoryUIManager::GetAbilitySystemComponent()
{
	if (AbilitySystemComponent.IsValid())
	{
		return AbilitySystemComponent.Get();
	}
	
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return nullptr;
	
	APlayerController* PC = Cast<APlayerController>(OwnerActor);
	if (!PC) return nullptr;
	
	if (APawn* Pawn = PC->GetPawn<APawn>())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
		{
			AbilitySystemComponent = ASC;
			return ASC;
		}
	}
	
	if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS))
		{
			AbilitySystemComponent = ASC;
			return ASC;
		}
	}
	
	return nullptr;
}
