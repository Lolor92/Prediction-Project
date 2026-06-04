// Copyright ProjectLogos

#include "Runtime/SyncCombatTagReactionRuntime.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SyncCombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

FSyncCombatTagReactionRuntime::FSyncCombatTagReactionRuntime(USyncCombatComponent& InCombatComponent)
	: CombatComponent(&InCombatComponent)
{
}

FSyncCombatTagReactionRuntime::~FSyncCombatTagReactionRuntime()
{
	Deinitialize();
}

void FSyncCombatTagReactionRuntime::Initialize(AActor* InOwnerActor,
	UAbilitySystemComponent* InAbilitySystemComponent, USyncCombatTagReactionData* InTagReactionData,
	TArray<FSyncCombatAnimBoolBinding>& InAnimBoolBindings)
{
	Deinitialize();

	OwningActor = InOwnerActor;
	AbilitySystemComponent = InAbilitySystemComponent;
	TagReactionData = InTagReactionData;
	AnimBoolBindings = &InAnimBoolBindings;

	BindTagReactionEvents();
}

void FSyncCombatTagReactionRuntime::Deinitialize()
{
	ClearTagReactionEvents();

	AnimInstance = nullptr;
	OwningActor = nullptr;
	AbilitySystemComponent = nullptr;
	TagReactionData = nullptr;
	AnimBoolBindings = nullptr;
}

void FSyncCombatTagReactionRuntime::BindTagReactionEvents()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC || !AnimBoolBindings) return;

	ClearTagReactionEvents();
	CacheAnimBoolBindings();

	TSet<FGameplayTag> WatchedTags;

	if (TagReactionData)
	{
		for (const FSyncCombatTagReactionBinding& Reaction : TagReactionData->Reactions)
			if (Reaction.TriggerTag.IsValid()) WatchedTags.Add(Reaction.TriggerTag);
	}

	for (const FSyncCombatAnimBoolBinding& Binding : *AnimBoolBindings)
	{
		TArray<FGameplayTag> Tags;
		Binding.Tags.GetGameplayTagArray(Tags);

		for (const FGameplayTag& Tag : Tags)
		{
			if (!Tag.IsValid()) continue;
			WatchedTags.Add(Tag);
		}
	}

	for (const FGameplayTag& Tag : WatchedTags)
	{
		FDelegateHandle DelegateHandle = ASC
			->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::AnyCountChange)
			.AddRaw(this, &FSyncCombatTagReactionRuntime::OnReactionTagChanged);

		TagReactionDelegateHandles.Add(Tag, DelegateHandle);
	}

	for (const FSyncCombatAnimBoolBinding& Binding : *AnimBoolBindings)
	{
		SetAnimBool(Binding, IsAnimBoolActive(Binding));
	}
}

void FSyncCombatTagReactionRuntime::ClearTagReactionEvents()
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		for (const TPair<FGameplayTag, FDelegateHandle>& Entry : TagReactionDelegateHandles)
		{
			if (!Entry.Key.IsValid() || !Entry.Value.IsValid()) continue;

			ASC->RegisterGameplayTagEvent(Entry.Key, EGameplayTagEventType::AnyCountChange).Remove(Entry.Value);
		}
	}

	TagReactionDelegateHandles.Reset();

	if (USyncCombatComponent* Component = CombatComponent.Get())
	{
		if (UWorld* World = Component->GetWorld())
		{
			for (TPair<FGameplayTag, FTimerHandle>& Entry : AbilityReactionTimers)
			{
				World->GetTimerManager().ClearTimer(Entry.Value);
			}

			for (TPair<FGameplayTag, FTimerHandle>& Entry : ApplyEffectReactionTimers)
			{
				World->GetTimerManager().ClearTimer(Entry.Value);
			}

			for (TPair<FName, FTimerHandle>& Entry : RemoveEffectReactionTimers)
			{
				World->GetTimerManager().ClearTimer(Entry.Value);
			}
		}
	}

	AbilityReactionTimers.Reset();
	ApplyEffectReactionTimers.Reset();
	RemoveEffectReactionTimers.Reset();
}

void FSyncCombatTagReactionRuntime::OnReactionTagChanged(const FGameplayTag Tag, const int32 NewCount)
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC || !AnimBoolBindings) return;

	const bool bAdded = NewCount > 0;
	const bool bAuthority = ASC->IsOwnerActorAuthoritative();

	if (TagReactionData && bAuthority)
	{
		for (const FSyncCombatTagReactionBinding& Reaction : TagReactionData->Reactions)
		{
			if (!Reaction.TriggerTag.IsValid() || !Tag.MatchesTag(Reaction.TriggerTag)) continue;

			const bool bShouldRun =
				Reaction.Policy == ESyncCombatTagReactionPolicy::Both ||
				(Reaction.Policy == ESyncCombatTagReactionPolicy::OnAdd && bAdded) ||
				(Reaction.Policy == ESyncCombatTagReactionPolicy::OnRemove && !bAdded);

			if (!bShouldRun) continue;

			QueueEffectRemove(Reaction, Tag);
			QueueEffectApply(Reaction, Tag);
			QueueAbilityActivation(Reaction, Tag);
		}
	}

	for (const FSyncCombatAnimBoolBinding& Binding : *AnimBoolBindings)
	{
		if (!Binding.Tags.HasTagExact(Tag)) continue;
		SetAnimBool(Binding, IsAnimBoolActive(Binding));
	}
}

void FSyncCombatTagReactionRuntime::CacheAnimBoolBindings()
{
	AActor* OwnerActor = OwningActor.Get();
	if (!OwnerActor || !AnimBoolBindings) return;

	if (USkeletalMeshComponent* MeshComp = OwnerActor->FindComponentByClass<USkeletalMeshComponent>())
	{
		AnimInstance = MeshComp->GetAnimInstance();
	}

	if (!AnimInstance.IsValid()) return;

	for (FSyncCombatAnimBoolBinding& Binding : *AnimBoolBindings)
	{
		Binding.CachedBoolProperty = nullptr;
		if (Binding.AnimBoolName.IsNone()) continue;

		if (FProperty* Property = AnimInstance->GetClass()->FindPropertyByName(Binding.AnimBoolName))
		{
			Binding.CachedBoolProperty = CastField<FBoolProperty>(Property);
		}
	}
}

void FSyncCombatTagReactionRuntime::SetAnimBool(const FSyncCombatAnimBoolBinding& Binding, const bool bValue) const
{
	UAnimInstance* CurrentAnimInstance = AnimInstance.Get();
	if (!CurrentAnimInstance || !Binding.CachedBoolProperty) return;

	void* ValuePtr = Binding.CachedBoolProperty->ContainerPtrToValuePtr<void>(CurrentAnimInstance);
	Binding.CachedBoolProperty->SetPropertyValue(ValuePtr, bValue);
}

bool FSyncCombatTagReactionRuntime::IsAnimBoolActive(const FSyncCombatAnimBoolBinding& Binding) const
{
	if (const UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		return ASC->HasAnyMatchingGameplayTags(Binding.Tags);
	}

	return false;
}

void FSyncCombatTagReactionRuntime::QueueAbilityActivation(const FSyncCombatTagReactionBinding& Binding,
	const FGameplayTag TriggeredTag)
{
	if (!Binding.Ability.AbilityTag.IsValid() || !AbilitySystemComponent.IsValid()) return;

	TWeakObjectPtr<UAbilitySystemComponent> WeakASC = AbilitySystemComponent;

	TFunction<void()> ActivateAbility = [WeakASC, AbilityTag = Binding.Ability.AbilityTag]()
	{
		UAbilitySystemComponent* ASC = WeakASC.Get();
		if (!ASC) return;

		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTag));
	};

	FTimerHandle& TimerHandle = AbilityReactionTimers.FindOrAdd(TriggeredTag);
	ExecuteDelayed(MoveTemp(ActivateAbility), Binding.Ability.DelaySeconds, TimerHandle);
}

void FSyncCombatTagReactionRuntime::QueueEffectApply(const FSyncCombatTagReactionBinding& Binding, const FGameplayTag TriggeredTag)
{
	if (Binding.Effects.Apply.IsEmpty() || !AbilitySystemComponent.IsValid()) return;

	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply = Binding.Effects.Apply;

	TFunction<void()> ApplyEffects = [this, EffectsToApply]()
	{
		for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectsToApply)
		{
			ApplyEffectToSelf(EffectClass, 1.f);
		}
	};

	FTimerHandle& TimerHandle = ApplyEffectReactionTimers.FindOrAdd(TriggeredTag);
	ExecuteDelayed(MoveTemp(ApplyEffects), Binding.Effects.ApplyDelaySeconds, TimerHandle);
}

void FSyncCombatTagReactionRuntime::QueueEffectRemove(const FSyncCombatTagReactionBinding& Binding, const FGameplayTag TriggeredTag)
{
	if (Binding.Effects.Remove.IsEmpty() || !AbilitySystemComponent.IsValid()) return;

	TWeakObjectPtr<UAbilitySystemComponent> WeakASC = AbilitySystemComponent;
	TArray<TSubclassOf<UGameplayEffect>> EffectsToRemove = Binding.Effects.Remove;

	TFunction<void()> RemoveEffects = [WeakASC, EffectsToRemove]()
	{
		UAbilitySystemComponent* ASC = WeakASC.Get();
		if (!ASC) return;

		for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectsToRemove)
		{
			if (!EffectClass) continue;

			FGameplayEffectQuery Query;
			Query.EffectDefinition = EffectClass;
			ASC->RemoveActiveEffects(Query);
		}
	};

	FTimerHandle& TimerHandle = RemoveEffectReactionTimers.FindOrAdd(GetRemoveTimerKey(Binding, TriggeredTag));
	ExecuteDelayed(MoveTemp(RemoveEffects), Binding.Effects.RemoveDelaySeconds, TimerHandle);
}

FName FSyncCombatTagReactionRuntime::GetRemoveTimerKey(const FSyncCombatTagReactionBinding& Binding,
	const FGameplayTag& TriggeredTag) const
{
	return Binding.Effects.RemoveTimerKey.IsNone()
		? TriggeredTag.GetTagName()
		: Binding.Effects.RemoveTimerKey;
}

void FSyncCombatTagReactionRuntime::ExecuteDelayed(TFunction<void()> Function, const float DelaySeconds,
	FTimerHandle& TimerHandle)
{
	if (DelaySeconds <= 0.f) { Function(); return; }

	USyncCombatComponent* Component = CombatComponent.Get();
	if (!Component) return;

	UWorld* World = Component->GetWorld();
	if (!World) return;

	World->GetTimerManager().ClearTimer(TimerHandle);

	FTimerDelegate TimerDelegate;
	TimerDelegate.BindLambda(MoveTemp(Function));
	World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, DelaySeconds, false);
}

FActiveGameplayEffectHandle FSyncCombatTagReactionRuntime::ApplyEffectToSelf(
	const TSubclassOf<UGameplayEffect>& GameplayEffectClass, const float Level) const
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	USyncCombatComponent* Component = CombatComponent.Get();
	if (!GameplayEffectClass || !ASC || !Component || !ASC->IsOwnerActorAuthoritative())
	{
		return FActiveGameplayEffectHandle();
	}

	AActor* OwnerActor = Component->GetOwner();

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(OwnerActor ? static_cast<UObject*>(OwnerActor) : static_cast<UObject*>(Component));

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GameplayEffectClass, Level, ContextHandle);

	return SpecHandle.IsValid()
		? ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get())
		: FActiveGameplayEffectHandle();
}
