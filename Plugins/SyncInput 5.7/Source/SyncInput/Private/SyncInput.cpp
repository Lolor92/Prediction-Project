// Copyright Epic Games, Inc. All Rights Reserved.

#include "SyncInput.h"
#include "Component/SyncInputComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagsManager.h"
#include "HAL/IConsoleManager.h"
#include "UObject/CoreRedirects.h"

#define LOCTEXT_NAMESPACE "FSyncInputModule"

namespace
{
	bool TryParseFloatArgument(const FString& Argument, float& OutValue)
	{
		return LexTryParseString(OutValue, *Argument);
	}

	USyncInputComponent* FindLocalSyncInputComponent(UWorld* World)
	{
		if (!World) return nullptr;

		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PlayerController->GetPawn())
			{
				return Pawn->FindComponentByClass<USyncInputComponent>();
			}
		}

		return nullptr;
	}

	void ParseSimulationArguments(
		const TArray<FString>& Args,
		TArray<FGameplayTag>& OutTags,
		TArray<float>& OutTimingValues)
	{
		for (const FString& Arg : Args)
		{
			float NumericValue = 0.f;
			if (TryParseFloatArgument(Arg, NumericValue))
			{
				OutTimingValues.Add(NumericValue);
				continue;
			}

			TArray<FString> TagNames;
			Arg.ParseIntoArray(TagNames, TEXT(","), true);
			for (const FString& TagName : TagNames)
			{
				const FString TrimmedTagName = TagName.TrimStartAndEnd();
				if (TrimmedTagName.IsEmpty()) continue;

				const FGameplayTag InputTag =
					UGameplayTagsManager::Get().RequestGameplayTag(FName(*TrimmedTagName), false);
				if (InputTag.IsValid())
				{
					OutTags.AddUnique(InputTag);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("SyncInput: Ignoring unknown simulation tag '%s'."), *TrimmedTagName);
				}
			}
		}
	}

	void StartInputSimulationCommand(const TArray<FString>& Args, UWorld* World)
	{
		USyncInputComponent* SyncInputComponent = FindLocalSyncInputComponent(World);
		if (!SyncInputComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("SyncInput: No local SyncInputComponent found for input simulation."));
			return;
		}

		TArray<FGameplayTag> InputTags;
		TArray<float> TimingValues;
		ParseSimulationArguments(Args, InputTags, TimingValues);

		if (TimingValues.Num() > 0)
		{
			SyncInputComponent->SimulatedInputMinHoldDuration = TimingValues[0];
		}
		if (TimingValues.Num() > 1)
		{
			SyncInputComponent->SimulatedInputMaxHoldDuration = TimingValues[1];
		}
		if (TimingValues.Num() > 2)
		{
			SyncInputComponent->SimulatedInputMinPauseDuration = TimingValues[2];
		}
		if (TimingValues.Num() > 3)
		{
			SyncInputComponent->SimulatedInputMaxPauseDuration = TimingValues[3];
		}

		SyncInputComponent->StartInputSimulation(InputTags);
		UE_LOG(LogTemp, Display, TEXT("SyncInput: Started local input simulation with %d explicit tag(s)."), InputTags.Num());
	}

	void StopInputSimulationCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (USyncInputComponent* SyncInputComponent = FindLocalSyncInputComponent(World))
		{
			SyncInputComponent->StopInputSimulation();
			UE_LOG(LogTemp, Display, TEXT("SyncInput: Stopped local input simulation."));
		}
	}

	static FAutoConsoleCommandWithWorldAndArgs GStartInputSimulationCommand(
		TEXT("SyncInput.Sim.Start"),
		TEXT("Starts local random SyncInput simulation. Usage: SyncInput.Sim.Start [SyncInput.Tag,SyncInput.OtherTag] [MinHold] [MaxHold] [MinPause] [MaxPause]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&StartInputSimulationCommand));

	static FAutoConsoleCommandWithWorldAndArgs GStopInputSimulationCommand(
		TEXT("SyncInput.Sim.Stop"),
		TEXT("Stops local random SyncInput simulation."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&StopInputSimulationCommand));
}

void FSyncInputModule::StartupModule()
{
}

void FSyncInputModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSyncInputModule, SyncInput)
