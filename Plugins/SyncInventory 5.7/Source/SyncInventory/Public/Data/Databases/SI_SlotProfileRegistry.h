#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/Definitions/SI_SlotProfile.h"
#include "SI_SlotProfileRegistry.generated.h"

/*
 * Stores all reusable slot profiles in one data asset.
 *
 * Layout configs will reference profiles by ProfileId,
 * so you do not need to duplicate slot rules everywhere.
 */
UCLASS(BlueprintType)
class SYNCINVENTORY_API USI_SlotProfileRegistry : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// Instanced means profiles can be created directly inside this registry asset.
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Inventory|Profiles", meta = (TitleProperty = "ProfileId"))
	TArray<TObjectPtr<USI_SlotProfile>> SlotProfiles;
	
	// Finds a profile by its ProfileId.
	// Returns nullptr if the id is empty or no matching profile exists.
	const USI_SlotProfile* FindProfileById(FName ProfileId) const;
};
