#include "Data/SCP_ReactionData.h"

#include "Animation/AnimMontage.h"

UAnimMontage* USCP_ReactionData::FindReactionMontage(FGameplayTag ReactionTag) const
{
	if (ReactionTag.IsValid())
	{
		for (const FSCP_ReactionMontageEntry& Entry : Reactions)
		{
			if (Entry.ReactionTag.IsValid() &&
				ReactionTag.MatchesTag(Entry.ReactionTag) &&
				Entry.Montage)
			{
				return Entry.Montage;
			}
		}
	}

	return DefaultReactionMontage;
}

