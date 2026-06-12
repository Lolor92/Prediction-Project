#include "Data/SCP_ReactionData.h"

#include "Animation/AnimMontage.h"

UAnimMontage* USCP_ReactionData::FindReactionMontage(FGameplayTag ReactionTag) const
{
	const FSCP_ReactionMontageEntry* Reaction = FindReaction(ReactionTag);
	return Reaction ? Reaction->Montage : nullptr;
}

const FSCP_ReactionMontageEntry* USCP_ReactionData::FindReaction(FGameplayTag ReactionTag) const
{
	if (!ReactionTag.IsValid())
	{
		return nullptr;
	}

	for (const FSCP_ReactionMontageEntry& Entry : Reactions)
	{
		if (Entry.ReactionTag.IsValid() && ReactionTag.MatchesTag(Entry.ReactionTag))
		{
			return &Entry;
		}
	}

	return nullptr;
}
