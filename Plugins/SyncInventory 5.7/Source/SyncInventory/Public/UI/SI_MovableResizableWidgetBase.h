#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "SI_MovableResizableWidgetBase.generated.h"

class UCanvasPanelSlot;
class UWidget;

UENUM(BlueprintType)
enum class ESI_WidgetDragMode : uint8
{
	Disabled UMETA(DisplayName="Disabled"),
	Anywhere UMETA(DisplayName="Anywhere"),
	TopBarOnly UMETA(DisplayName="Top Bar Only")
};

UCLASS(Abstract, BlueprintType, Blueprintable)
class SYNCINVENTORY_API USI_MovableResizableWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="SyncInventory|UI|Window")
	void StartDragAtScreenPosition(const FVector2D& ScreenPosition);

	UFUNCTION(BlueprintCallable, Category="SyncInventory|UI|Window")
	void StartResizeAtScreenPosition(const FVector2D& ScreenPosition);

	UFUNCTION(BlueprintCallable, Category="SyncInventory|UI|Window")
	void StopManipulation();

	UFUNCTION(BlueprintCallable, Category="SyncInventory|UI|Window")
	bool GetWindowLayout(FVector2D& OutPosition, FVector2D& OutSize) const;

	UFUNCTION(BlueprintCallable, Category="SyncInventory|UI|Window")
	void SetWindowPosition(const FVector2D& InPosition);

	UFUNCTION(BlueprintCallable, Category="SyncInventory|UI|Window")
	void ResetToDefaultLayout();

	void CaptureDefaultLayout();

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	virtual UWidget* GetManipulationTarget() const { return nullptr; }
	virtual FKey GetDragMouseButton() const;
	virtual bool ShouldStartDragAtScreenPosition(const FVector2D& ScreenPosition) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInventory|UI|Window|Drag")
	ESI_WidgetDragMode DragMode = ESI_WidgetDragMode::Anywhere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInventory|UI|Window|Drag",
		meta=(EditCondition="DragMode == ESI_WidgetDragMode::TopBarOnly", ClampMin="1.0"))
	float TitleBarDragHeight = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInventory|UI|Window|Resize")
	bool bResizeAnywhereWithRightMouse = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInventory|UI|Window|Resize")
	FVector2D MinWindowSize = FVector2D(200.f, 200.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SyncInventory|UI|Window|Resize")
	FVector2D MaxWindowSize = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	bool bIsDragging = false;

	UPROPERTY(Transient)
	bool bIsResizing = false;

	UPROPERTY(Transient)
	FVector2D DragOffset = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	FVector2D ResizeStartMouseViewport = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	FVector2D ResizeStartWindowSize = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	bool bHasDefaultLayout = false;

	UPROPERTY(Transient)
	FVector2D DefaultWindowPosition = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	FVector2D DefaultWindowSize = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	bool bDefaultAutoSize = false;

private:
	bool ResolveCanvasSlot(UCanvasPanelSlot*& OutCanvasSlot) const;
	bool ScreenToViewportPosition(const FVector2D& ScreenPosition, FVector2D& OutViewportPosition) const;
	FVector2D GetCurrentTargetSize(const UCanvasPanelSlot* CanvasSlot) const;
};
