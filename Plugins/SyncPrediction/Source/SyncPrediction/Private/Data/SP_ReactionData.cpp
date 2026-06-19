#include "Data/SP_ReactionData.h"

const FSP_ReactionDataEntry& USP_ReactionData::FindReactionChecked(FGameplayTag ReactionTag) const
{
	const FSP_ReactionDataEntry* Reaction = FindReactionPtr(ReactionTag);
	check(Reaction);
	return *Reaction;
}

bool USP_ReactionData::FindReaction(FGameplayTag ReactionTag, FSP_ReactionDataEntry& OutReaction) const
{
	const FSP_ReactionDataEntry* Reaction = FindReactionPtr(ReactionTag);
	if (!Reaction) return false;

	OutReaction = *Reaction;
	return true;
}

const FSP_ReactionDataEntry* USP_ReactionData::FindReactionPtr(FGameplayTag ReactionTag) const
{
	if (!ReactionTag.IsValid()) return nullptr;

	for (const FSP_ReactionDataEntry& Entry : Reactions)
	{
		if (Entry.ReactionTag.IsValid() && ReactionTag.MatchesTag(Entry.ReactionTag))
		{
			return &Entry;
		}
	}

	return nullptr;
}
