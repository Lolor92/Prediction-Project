#include "Data/Databases/SI_SlotLayoutConfig.h"

#if WITH_EDITOR
// Needed for FDataValidationContext and EDataValidationResult.
#include "Misc/DataValidation.h"
#endif

TArray<FString> USI_SlotLayoutConfig::GetAvailableProfileIds() const
{
	// This array will hold the profile names shown in the editor dropdown.
	TArray<FString> ProfileIds;

	// If no registry is assigned, there are no profile ids to show.
	if (!SlotProfileRegistry)
	{
		return ProfileIds;
	}

	// Reserve memory up front based on how many profiles exist.
	ProfileIds.Reserve(SlotProfileRegistry->SlotProfiles.Num());

	// Check every profile stored in the registry.
	for (const TObjectPtr<USI_SlotProfile>& Profile : SlotProfileRegistry->SlotProfiles)
	{
		// Skip empty profile references.
		if (!Profile) continue;

		// Skip profiles that do not have an id.
		if (Profile->ProfileId.IsNone()) continue;

		// Convert the FName id into a string for the editor dropdown.
		ProfileIds.Add(Profile->ProfileId.ToString());
	}

	// Sort makes the dropdown easier to read.
	ProfileIds.Sort();

	// Return the final dropdown list.
	return ProfileIds;
}

const FSI_SlotLayoutEntry* USI_SlotLayoutConfig::FindSlotEntryByIndex(int32 SlotIndex) const
{
	// Search SlotEntries for the first entry with the matching SlotIndex.
	return SlotEntries.FindByPredicate([SlotIndex](const FSI_SlotLayoutEntry& Entry)
	{
		return Entry.SlotIndex == SlotIndex;
	});
}

#if WITH_EDITOR
EDataValidationResult USI_SlotLayoutConfig::IsDataValid(FDataValidationContext& Context) const
{
	// Start with the parent class validation result.
	EDataValidationResult Result = Super::IsDataValid(Context);

	// A layout config needs a registry so profile ids can be checked.
	if (!SlotProfileRegistry)
	{
		Context.AddError(FText::FromString(TEXT("SlotProfileRegistry is not assigned.")));
		return EDataValidationResult::Invalid;
	}

	// Validate every slot entry in this layout.
	for (const FSI_SlotLayoutEntry& SlotEntry : SlotEntries)
	{
		// Empty profile ids are allowed.
		// This means the slot has no special profile yet.
		if (SlotEntry.SlotProfileId.IsNone()) continue;

		// Try to find the profile id in the registry.
		const USI_SlotProfile* Profile = SlotProfileRegistry->FindProfileById(SlotEntry.SlotProfileId);

		// If no profile was found, this layout points to a missing profile.
		if (!Profile)
		{
			Context.AddError(FText::Format(
				NSLOCTEXT("Inventory", "InvalidSlotProfileId", "Slot index {0} references missing profile id '{1}'."),
				FText::AsNumber(SlotEntry.SlotIndex),
				FText::FromName(SlotEntry.SlotProfileId)));

			// Mark the asset as invalid.
			Result = EDataValidationResult::Invalid;
		}
	}

	// Return whether the asset is valid or invalid.
	return Result;
}

void USI_SlotLayoutConfig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// Let the parent class handle its editor change behavior first.
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Without a registry, we cannot check whether profile ids are valid.
	if (!SlotProfileRegistry) return;

	// Check every slot entry after an editor property changes.
	for (FSI_SlotLayoutEntry& SlotEntry : SlotEntries)
	{
		// Empty profile ids are already fine.
		if (SlotEntry.SlotProfileId.IsNone()) continue;

		// If the selected profile id no longer exists, clear it.
		if (!SlotProfileRegistry->FindProfileById(SlotEntry.SlotProfileId))
		{
			SlotEntry.SlotProfileId = NAME_None;
		}
	}
}
#endif
