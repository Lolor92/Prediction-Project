#include "UI/Widgets/SI_InventorySlotWidget.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Component/SI_InventoryComponent.h"
#include "Component/SI_InventoryUIManager.h"
#include "Replication/SI_InventoryFastArray.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Data/Definitions/SI_ItemDefinition.h"
#include "UI/DragDrop/SI_InventoryDragOperation.h"
#include "UI/DragDrop/SI_InventorySlotDragVisual.h"
#include "Data/Databases/SI_SlotLayoutConfig.h"
#include "Tags/SI_NativeGameplayTags.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "UI/ToolTips/SI_ItemToolTipWidget.h"


static FString GetShortInputTagText(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return FString();

	// Convert the full tag to text.
	// Example: Input.Inventory.1
	const FString FullTagString = InputTag.ToString();

	// Split the tag into pieces by periods.
	TArray<FString> TagParts;
	FullTagString.ParseIntoArray(TagParts, TEXT("."));

	// If split failed, fall back to the full tag.
	if (TagParts.IsEmpty())
	{
		return FullTagString;
	}

	// Use the last part for compact UI.
	// Example: Input.Inventory.1 becomes 1.
	return TagParts.Last();
}

void USI_InventorySlotWidget::InitSlotWidget(USI_InventoryUIManager* InInventoryUIManager,
                                             const FGameplayTag& InContainerId,int32 InSlotIndex)
{
	InventoryUIManager = InInventoryUIManager;
	ContainerId = InContainerId;
	SlotIndex = InSlotIndex;
	
	Refresh();
}

void USI_InventorySlotWidget::Refresh()
{
	auto InventoryComponent = InventoryUIManager->GetInventoryComponent();
	if (!InventoryComponent)
	{
		SetToDefault();
		return;
	}
	
	const FSI_InventoryEntry* Entry = InventoryComponent->FindEntryAtSlot(ContainerId, SlotIndex);
	if (!Entry)
	{
		SetToDefault();
		return;
	}
	
	FGameplayTag ItemTag = Entry->ItemTag;
	if (!ItemTag.IsValid())
	{
		SetToDefault();
		return;
	}
	
	const USI_ItemDatabase* ItemDatabase = InventoryComponent->GetItemDatabase();
	const USI_ItemDefinition* Def = ItemDatabase ? ItemDatabase->GetItemDefinitionByTag(ItemTag) : nullptr;
	if (!Def)
	{
		SetToDefault();
		return;
	}
	
	if (SlotBackgroundImage)
	{
		SlotBackgroundImage->SetBrush(GetBackgroundRarityBrush(Def->GetItemRarityTag()));
	}
	
	if (ItemIcon)
	{
		ItemIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ItemIcon->SetBrushFromTexture(Def->ItemIcon);
	}
	
	if (QuantityText)
	{
		// References show the amount carried in the main inventory.
		// Real inventory slots show their own stack quantity.
		const int32 DisplayQuantity = Entry->bIsReference
			? InventoryComponent->GetMainInventoryQuantityForItem(Entry->ItemTag)
			: Entry->Quantity;

		const bool bShowQuantity = Def->bStackable && DisplayQuantity > 0;

		QuantityText->SetVisibility(bShowQuantity
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);

		QuantityText->SetText(bShowQuantity
			? FText::AsNumber(DisplayQuantity) : FText::GetEmpty());
	}
	
	if (InputTagText)
	{
		InputTagText->SetVisibility(ESlateVisibility::Hidden);
		InputTagText->SetText(FText::GetEmpty());

		// Get this slot's layout entry, if one exists.
		const FSI_SlotLayoutEntry* SlotEntry = InventoryComponent
			? InventoryComponent->GetSlotLayoutEntry(ContainerId, SlotIndex)
			: nullptr;

		// Only show text when the slot has a valid input tag.
		if (SlotEntry && SlotEntry->InputTag.IsValid())
		{
			InputTagText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			InputTagText->SetText(FText::FromString(GetShortInputTagText(SlotEntry->InputTag)));
		}
	}
	
	if (HoverImage)
	{
		HoverImage->SetVisibility(ESlateVisibility::Hidden);
	}
	
	if (CooldownText)
	{
		CooldownText->SetVisibility(ESlateVisibility::Hidden);
		CooldownText->SetText(FText::GetEmpty());
	}
	
	if (CooldownOverlayImage)
	{
		CooldownOverlayImage->SetVisibility(ESlateVisibility::Hidden);
	}
	
	RefreshCooldownVisual();
}

void USI_InventorySlotWidget::SetToDefault()
{
	HideItemTooltip();

	if (SlotBackgroundImage)
	{
		SlotBackgroundImage->SetBrush(DefaultSlotBackgroundBrush);
	}
	
	if (ItemIcon)
	{
		ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		ItemIcon->SetBrushFromTexture(nullptr);
	}
	
	if (QuantityText)
	{
		QuantityText->SetVisibility(ESlateVisibility::Hidden);
		QuantityText->SetText(FText::GetEmpty());
	}
	
	if (InputTagText)
	{
		InputTagText->SetVisibility(ESlateVisibility::Hidden);
		InputTagText->SetText(FText::GetEmpty());

		const USI_InventoryComponent* InventoryComponent = InventoryUIManager 
		? InventoryUIManager->GetInventoryComponent() : nullptr;

		// Get this slot's layout entry, if one exists.
		const FSI_SlotLayoutEntry* SlotEntry = InventoryComponent
			? InventoryComponent->GetSlotLayoutEntry(ContainerId, SlotIndex)
			: nullptr;

		// Show the input label even when the slot is empty.
		if (SlotEntry && SlotEntry->InputTag.IsValid())
		{
			InputTagText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			InputTagText->SetText(FText::FromString(GetShortInputTagText(SlotEntry->InputTag)));
		}
	}
	
	if (HoverImage)
	{
		HoverImage->SetVisibility(ESlateVisibility::Hidden);
	}
	
	if (CooldownText)
	{
		CooldownText->SetVisibility(ESlateVisibility::Hidden);
		CooldownText->SetText(FText::GetEmpty());
	}
	
	if (CooldownOverlayImage)
	{
		CooldownOverlayImage->SetVisibility(ESlateVisibility::Hidden);
	}
	
	RefreshCooldownVisual();
}

const FSlateBrush& USI_InventorySlotWidget::GetBackgroundRarityBrush(const FGameplayTag& RarityTag) const
{
	if (const FSlateBrush* ExactBrush = RaritySlotBackgroundBrushes.Find(RarityTag))
	{
		return *ExactBrush;
	}

	for (const TPair<FGameplayTag, FSlateBrush>& Pair : RaritySlotBackgroundBrushes)
	{
		if (Pair.Key.IsValid() && RarityTag.MatchesTag(Pair.Key))
		{
			return Pair.Value;
		}
	}

	if (RarityTag.MatchesTagExact(TAG_SI_Item_Rarity_Common))
	{
		return CommonSlotBackgroundBrush;
	}

	if (RarityTag.MatchesTagExact(TAG_SI_Item_Rarity_Rare))
	{
		return RareSlotBackgroundBrush;
	}

	if (RarityTag.MatchesTagExact(TAG_SI_Item_Rarity_Epic))
	{
		return EpicSlotBackgroundBrush;
	}

	if (RarityTag.MatchesTagExact(TAG_SI_Item_Rarity_Legendary))
	{
		return LegendarySlotBackgroundBrush;
	}

	return DefaultSlotBackgroundBrush;
}

FReply USI_InventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FReply SuperReply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	
	bDragDetectThisClick = false;
	
	if (!InventoryUIManager || !ContainerId.IsValid() || SlotIndex < 0)
	{
		return SuperReply;
	}

	USI_InventoryComponent* InventoryComponent = InventoryUIManager->GetInventoryComponent();
	if (!InventoryComponent) return SuperReply;

	const FSI_InventoryEntry* Entry = InventoryComponent->FindEntryAtSlot(ContainerId, SlotIndex);
	
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (!Entry)
		{
			bDragDetectThisClick = false;
			return SuperReply;
		}

		PressedFeedback();
		
		return UWidgetBlueprintLibrary::DetectDragIfPressed(
			InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}
	
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (!Entry)
		{
			return SuperReply;
		}

		PressedFeedback();
		
		// References are shortcuts, not real stacks, so they cannot be split.
		if (Entry->bIsReference)
		{
			ResetPressedFeedback();
			return SuperReply;
		}
		
		const USI_ItemDatabase* ItemDatabase = InventoryComponent->GetItemDatabase();
		const USI_ItemDefinition* Def = ItemDatabase ? ItemDatabase->GetItemDefinitionByTag(Entry->ItemTag) : nullptr;
		if (!Def)
		{
			ResetPressedFeedback();
			return SuperReply;
		}

		if (Def->bEquippable)
		{
			const FGameplayTag EquipmentContainerId = InventoryComponent->GetEquipmentContainerId();
			if (!EquipmentContainerId.IsValid())
			{
				ResetPressedFeedback();
				return SuperReply;
			}

			FSI_MoveItemRequest Request;
			Request.FromContainerId = ContainerId;
			Request.FromSlotIndex = SlotIndex;

			if (ContainerId == EquipmentContainerId)
			{
				const FGameplayTag MainContainerId = InventoryComponent->GetMainContainerId();
				if (!MainContainerId.IsValid())
				{
					ResetPressedFeedback();
					return SuperReply;
				}

				const int32 MainInventorySlotIndex = InventoryComponent->FindFirstFreeAllowedSlot(MainContainerId, Entry->ItemTag);
				if (MainInventorySlotIndex == INDEX_NONE)
				{
					ResetPressedFeedback();
					return SuperReply;
				}

				Request.ToContainerId = MainContainerId;
				Request.ToSlotIndex = MainInventorySlotIndex;
			}
			else
			{
				const int32 EquipmentSlotIndex = InventoryComponent->FindBestEquipmentSlotForItem(Entry->ItemTag);
				if (EquipmentSlotIndex == INDEX_NONE)
				{
					ResetPressedFeedback();
					return SuperReply;
				}

				Request.ToContainerId = EquipmentContainerId;
				Request.ToSlotIndex = EquipmentSlotIndex;
			}

			HideItemTooltip();
			InventoryComponent->RequestMoveItem(Request);
			ResetPressedFeedback();
			return FReply::Handled();
		}
		
		const bool bCanSplit = Def->bStackable && Entry->Quantity > 1;
		if (bCanSplit)
		{
			InventoryUIManager->OpenSplitStackWidget(ContainerId, SlotIndex, InMouseEvent.GetScreenSpacePosition());
			ResetPressedFeedback();
			return FReply::Handled();
		}

		ResetPressedFeedback();
	}
	
	return SuperReply;
}

FReply USI_InventorySlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FReply SuperReply = Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	
	if (!InventoryUIManager->GetInventoryComponent() || !ContainerId.IsValid() || SlotIndex < 0)
	{
		bDragDetectThisClick = false;
		return SuperReply;
	}
	
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		bDragDetectThisClick = false;
		return SuperReply;
	}
	
	const FSI_InventoryEntry* Entry = InventoryUIManager->GetInventoryComponent()->FindEntryAtSlot(ContainerId, SlotIndex);
	if (!Entry)
	{
		bDragDetectThisClick = false;
		ResetPressedFeedback();
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}
	
	if (!bDragDetectThisClick)
	{
		ResetPressedFeedback();
		InventoryUIManager->GetInventoryComponent()->RequestUseItem(ContainerId, SlotIndex, true);
		return FReply::Handled();
	}

	ResetPressedFeedback();
	
	return SuperReply;
}

void USI_InventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
	
	HideItemTooltip();
	
	if (!InventoryUIManager || !ContainerId.IsValid() || SlotIndex < 0) return;
	
	const FSI_InventoryEntry* Entry = InventoryUIManager->GetInventoryComponent()->FindEntryAtSlot(ContainerId, SlotIndex);
	if (!Entry) return;
	
	USI_InventoryDragOperation* DragOperation = NewObject<USI_InventoryDragOperation>();
	if (!DragOperation) return;
	DragOperation->InventoryComponent = InventoryUIManager->GetInventoryComponent();
	DragOperation->ContainerId = ContainerId;
	DragOperation->SlotIndex = SlotIndex;
	
	bDragDetectThisClick = true;
	ResetPressedFeedback();
	
	const FSI_ContainerConfig* Config = InventoryUIManager->GetInventoryComponent()->GetContainerConfig(ContainerId);
	DragOperation->bAllowDeleteOnEmptyDrop = Config ? Config->bAllowDeleteOnEmptyDrop : false;
	
	if (DragVisualClass)
	{
		if (USI_InventorySlotDragVisual* DragVisual = CreateWidget<USI_InventorySlotDragVisual>(this, DragVisualClass))
		{
			DragVisual->InitDragVisual(InventoryUIManager->GetInventoryComponent(), ContainerId, SlotIndex);
			DragOperation->DefaultDragVisual = DragVisual;
		}
	}
	
	OutOperation = DragOperation;
}

bool USI_InventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	
	if (!InventoryUIManager || !ContainerId.IsValid() || SlotIndex < 0) return false;
	
	USI_InventoryDragOperation* DragOperation = Cast<USI_InventoryDragOperation>(InOperation);
	if (!DragOperation) return false;
	if (!DragOperation->InventoryComponent) return false;
	if (DragOperation->InventoryComponent != InventoryUIManager->GetInventoryComponent()) return false;
	if (!DragOperation->ContainerId.IsValid()) return false;
	if (DragOperation->ContainerId == ContainerId && DragOperation->SlotIndex == SlotIndex) return false;
	if (DragOperation->SlotIndex < 0) return false;
	
	USI_InventoryComponent* InventoryComponent = InventoryUIManager->GetInventoryComponent();
	if (!InventoryComponent) return false;
	
	FSI_MoveItemRequest Request;
	Request.FromContainerId = DragOperation->ContainerId;
	Request.FromSlotIndex = DragOperation->SlotIndex;
	Request.ToContainerId = ContainerId;
	Request.ToSlotIndex = SlotIndex;
	
	InventoryComponent->RequestMoveItem(Request);
	
	return true;
}

void USI_InventorySlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	
	if (HoverImage)
	{
		HoverImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	
	ShowItemTooltip(InGeometry);
}

void USI_InventorySlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	
	if (HoverImage)
	{
		HoverImage->SetVisibility(ESlateVisibility::Hidden);
	}
	
	HideItemTooltip();
}

void USI_InventorySlotWidget::RefreshCooldownVisual()
{
	FSI_SlotCooldownState CooldownState{};
	
	StopCooldownRefreshTimer();
	ApplyCooldownVisual(CooldownState);
	
	USI_InventoryComponent* InventoryComponent = InventoryUIManager->GetInventoryComponent();
	
	if (!InventoryComponent || !ContainerId.IsValid() || SlotIndex < 0) return;
	
	const FSI_InventoryEntry* Entry = InventoryComponent->FindEntryAtSlot(ContainerId, SlotIndex);
	if (!Entry) return;
	
	const USI_ItemDatabase* ItemDatabase = InventoryComponent->GetItemDatabase();
	if (!ItemDatabase) return;
	
	const USI_ItemDefinition* Def = ItemDatabase->GetItemDefinitionByTag(Entry->ItemTag);
	if (!Def || !Def->CooldownGameplayEffect) return;
	
	UAbilitySystemComponent* ASC = InventoryComponent->GetAbilitySystemComponent();
	if (!ASC) return;
	
	FGameplayEffectQuery Query;
	Query.EffectDefinition = Def->CooldownGameplayEffect;

	const TArray<TPair<float, float>> TimePairs = ASC->GetActiveEffectsTimeRemainingAndDuration(Query);
	if (TimePairs.IsEmpty()) return;
	
	for (const TPair<float, float>& TimePair : TimePairs)
	{
		const float Remaining = FMath::Max(0.f, TimePair.Key);
		const float Duration = FMath::Max(0, TimePair.Value);
		
		if (Remaining > CooldownState.TimeRemaining)
		{
			CooldownState.TimeRemaining = Remaining;
			CooldownState.TotalDuration = Duration;
		}
	}
	
	CooldownState.bIsOnCooldown = CooldownState.TimeRemaining > 0.f;
	ApplyCooldownVisual(CooldownState);
	
	if (CooldownState.bIsOnCooldown)
	{
		StartCooldownRefreshTimer();
	}
}

void USI_InventorySlotWidget::StartCooldownRefreshTimer()
{
	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(CooldownRefreshTimer))
		{
			World->GetTimerManager().SetTimer(
				CooldownRefreshTimer, this, &ThisClass::RefreshCooldownVisual, 0.03f, true);
		}
	}
}

void USI_InventorySlotWidget::StopCooldownRefreshTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CooldownRefreshTimer);
	}
}

void USI_InventorySlotWidget::ApplyCooldownVisual(const FSI_SlotCooldownState& CooldownState)
{
	const bool bShowCooldown = CooldownState.bIsOnCooldown;
	
	if (CooldownText)
	{
		CooldownText->SetText(bShowCooldown
		? FText::AsNumber(FMath::Max(1, FMath::CeilToInt(CooldownState.TimeRemaining)))
		: FText::GetEmpty());
		
		CooldownText->SetVisibility(bShowCooldown
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Hidden);
	}
	
	if (DeactivatedImageOverlay)
	{
		DeactivatedImageOverlay->SetVisibility(bShowCooldown ?
			ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
	
	if (!CooldownOverlayImage) return;
	
	if (bShowCooldown && !CooldownMaterialInstance && CooldownMaterial)
	{
		CooldownMaterialInstance = UMaterialInstanceDynamic::Create(CooldownMaterial, this);
		if (CooldownMaterialInstance)
		{
			CooldownOverlayImage->SetBrushFromMaterial(CooldownMaterialInstance.Get());
		}
	}
	
	if (!bShowCooldown ||!CooldownMaterialInstance)
	{
		CooldownOverlayImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	
	CooldownMaterialInstance->SetScalarParameterValue(
		CooldownProgressParameterName,
		CooldownState.GetNormalizedRemaining());
	
	CooldownOverlayImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void USI_InventorySlotWidget::PressedFeedback()
{
	SetRenderScale(FVector2D(PressedScale, PressedScale));
}

void USI_InventorySlotWidget::ResetPressedFeedback()
{
	SetRenderScale(FVector2D(1.f, 1.f));
}

void USI_InventorySlotWidget::ShowItemTooltip(const FGeometry& InGeometry)
{
	HideItemTooltip();

	if (!ItemTooltipWidgetClass || !InventoryUIManager || !ContainerId.IsValid() || SlotIndex < 0)
	{
		return;
	}

	USI_InventoryComponent* InventoryComponent = InventoryUIManager->GetInventoryComponent();
	if (!InventoryComponent) return;

	const FSI_InventoryEntry* Entry = InventoryComponent->FindEntryAtSlot(ContainerId, SlotIndex);
	if (!Entry || !Entry->IsValidEntry()) return;

	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController) return;

	ActiveItemTooltipWidget = CreateWidget<UUserWidget>(PlayerController, ItemTooltipWidgetClass);
	if (!ActiveItemTooltipWidget) return;
	
	if (USI_ItemToolTipWidget* ItemTooltipWidget = Cast<USI_ItemToolTipWidget>(ActiveItemTooltipWidget))
	{
		ItemTooltipWidget->InitTooltip(InventoryComponent, ContainerId, SlotIndex);
	}

	ActiveItemTooltipWidget->AddToViewport(200);
	ActiveItemTooltipWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

	const FVector2D SlotAbsolutePosition = InGeometry.GetAbsolutePosition();
	const FVector2D SlotAbsoluteBottomRight = InGeometry.LocalToAbsolute(InGeometry.GetLocalSize());

	FVector2D PixelPosition;
	FVector2D ViewportPosition;
	USlateBlueprintLibrary::AbsoluteToViewport(GetWorld(), SlotAbsolutePosition,
		PixelPosition, ViewportPosition);

	FVector2D SlotBottomRightPixelPosition;
	FVector2D SlotBottomRightViewportPosition;
	USlateBlueprintLibrary::AbsoluteToViewport(GetWorld(), SlotAbsoluteBottomRight,
		SlotBottomRightPixelPosition, SlotBottomRightViewportPosition);

	const FVector2D SlotViewportSize = SlotBottomRightViewportPosition - ViewportPosition;

	ActiveItemTooltipWidget->ForceLayoutPrepass();

	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this) / FMath::Max(ViewportScale, KINDA_SMALL_NUMBER);

	FVector2D TooltipSize = ActiveItemTooltipWidget->GetDesiredSize();
	if (TooltipSize.IsNearlyZero())
	{
		TooltipSize = FVector2D(320.f, 240.f);
	}

	FVector2D TooltipPosition = FVector2D(
		ViewportPosition.X + SlotViewportSize.X + TooltipOffset.X,
		ViewportPosition.Y);

	if (TooltipPosition.X + TooltipSize.X + TooltipPadding.X > ViewportSize.X)
	{
		TooltipPosition.X = ViewportPosition.X - TooltipSize.X - TooltipOffset.X;
	}

	if (TooltipPosition.Y + TooltipSize.Y + TooltipPadding.Y > ViewportSize.Y)
	{
		TooltipPosition.Y = ViewportSize.Y - TooltipSize.Y - TooltipPadding.Y;
	}

	const FVector2D MaxTooltipPosition(
		FMath::Max(TooltipPadding.X, ViewportSize.X - TooltipSize.X - TooltipPadding.X),
		FMath::Max(TooltipPadding.Y, ViewportSize.Y - TooltipSize.Y - TooltipPadding.Y));

	TooltipPosition.X = FMath::Clamp(TooltipPosition.X, TooltipPadding.X, MaxTooltipPosition.X);
	TooltipPosition.Y = FMath::Clamp(TooltipPosition.Y, TooltipPadding.Y, MaxTooltipPosition.Y);

	ActiveItemTooltipWidget->SetPositionInViewport(TooltipPosition, false);
}

void USI_InventorySlotWidget::HideItemTooltip()
{
	if (ActiveItemTooltipWidget)
	{
		ActiveItemTooltipWidget->RemoveFromParent();
		ActiveItemTooltipWidget = nullptr;
	}
}
