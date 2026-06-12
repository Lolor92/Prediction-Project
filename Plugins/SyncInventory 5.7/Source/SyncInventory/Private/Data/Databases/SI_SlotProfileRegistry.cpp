#include "Data/Databases/SI_SlotProfileRegistry.h"

const USI_SlotProfile* USI_SlotProfileRegistry::FindProfileById(FName ProfileId) const
{
	if (ProfileId.IsNone()) return nullptr;
	
	const TObjectPtr<USI_SlotProfile>* Match =
	SlotProfiles.FindByPredicate([ProfileId](const TObjectPtr<USI_SlotProfile>& Profile)
	{
		return Profile && Profile->ProfileId == ProfileId;
	});

	return Match ? Match->Get() : nullptr;
}