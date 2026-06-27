#include "Movement/SP_CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"

static int32 SP_GetWorldPIEInstanceId(const UWorld* World)
{
	if (!World)
	{
		return INDEX_NONE;
	}

	if (const UPackage* Package = World->GetPackage())
	{
		const int32 PIEInstance = Package->GetPIEInstanceID();
		if (PIEInstance != INDEX_NONE)
		{
			return PIEInstance;
		}
	}

	if (GEngine)
	{
		if (const FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(World))
		{
			return WorldContext->PIEInstance;
		}
	}

	return INDEX_NONE;
}

void USP_CharacterMovementComponent::SetRootMotionContactBlock(
	bool bBlocked,
	const FVector& InBlockedDirection,
	float Duration)
{
	const bool bWasBlocked = bSPRootMotionContactBlocked;
	const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SPRootMotionContactBlockTimerHandle);
	}

	bSPRootMotionContactBlocked = bBlocked;
	SPBlockedRootMotionDirection = InBlockedDirection.GetSafeNormal2D();
	SPRootMotionContactBlockStartTime = bBlocked ? CurrentTime : -1.0;

	if (bBlocked && Duration > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				SPRootMotionContactBlockTimerHandle,
				this,
				&USP_CharacterMovementComponent::ClearRootMotionContactBlock,
				Duration,
				false);
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("SP CMC RootMotionContactBlock SET PIE=%d Character=%s Blocked=%d WasBlocked=%d Direction=%s Duration=%.3f Time=%.3f Role=%d Local=%d Auth=%d"),
		SP_GetWorldPIEInstanceId(GetWorld()),
		*GetNameSafe(CharacterOwner),
		bSPRootMotionContactBlocked ? 1 : 0,
		bWasBlocked ? 1 : 0,
		*SPBlockedRootMotionDirection.ToString(),
		Duration,
		CurrentTime,
		CharacterOwner ? static_cast<int32>(CharacterOwner->GetLocalRole()) : -1,
		CharacterOwner ? CharacterOwner->IsLocallyControlled() : false,
		CharacterOwner ? CharacterOwner->HasAuthority() : false);
}

void USP_CharacterMovementComponent::ClearRootMotionContactBlock()
{
	const bool bWasBlocked = bSPRootMotionContactBlocked;
	const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0;
	const double ActiveFor = (bWasBlocked && SPRootMotionContactBlockStartTime >= 0.0 && CurrentTime >= 0.0)
		? CurrentTime - SPRootMotionContactBlockStartTime
		: -1.0;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SPRootMotionContactBlockTimerHandle);
	}

	bSPRootMotionContactBlocked = false;
	SPBlockedRootMotionDirection = FVector::ZeroVector;
	SPRootMotionContactBlockStartTime = -1.0;

	UE_LOG(LogTemp, Warning,
		TEXT("SP CMC RootMotionContactBlock CLEAR PIE=%d Character=%s WasBlocked=%d ActiveFor=%.3f Time=%.3f Role=%d Local=%d Auth=%d"),
		SP_GetWorldPIEInstanceId(GetWorld()),
		*GetNameSafe(CharacterOwner),
		bWasBlocked ? 1 : 0,
		ActiveFor,
		CurrentTime,
		CharacterOwner ? static_cast<int32>(CharacterOwner->GetLocalRole()) : -1,
		CharacterOwner ? CharacterOwner->IsLocallyControlled() : false,
		CharacterOwner ? CharacterOwner->HasAuthority() : false);
}

void USP_CharacterMovementComponent::ApplyRootMotionToVelocity(float DeltaTime)
{
	const bool bHadRootMotionVelocity =
		HasAnimRootMotion() ||
		CurrentRootMotion.HasOverrideVelocity() ||
		CurrentRootMotion.HasAdditiveVelocity();

	Super::ApplyRootMotionToVelocity(DeltaTime);

	if (bHadRootMotionVelocity)
	{
		Velocity = RemoveVelocityIntoRootMotionContactBlock(Velocity, TEXT("source"));
	}
}

FVector USP_CharacterMovementComponent::CalcAnimRootMotionVelocity(
	const FVector& RootMotionDeltaMove,
	float DeltaSeconds,
	const FVector& CurrentVelocity) const
{
	FVector RootMotionVelocity = Super::CalcAnimRootMotionVelocity(
		RootMotionDeltaMove,
		DeltaSeconds,
		CurrentVelocity);

	return RemoveVelocityIntoRootMotionContactBlock(RootMotionVelocity, TEXT("anim"));
}

FVector USP_CharacterMovementComponent::RemoveVelocityIntoRootMotionContactBlock(
	const FVector& InVelocity,
	const TCHAR* LogContext) const
{
	if (!bSPRootMotionContactBlocked || SPBlockedRootMotionDirection.IsNearlyZero())
	{
		return InVelocity;
	}

	const FVector BlockDir = SPBlockedRootMotionDirection.GetSafeNormal2D();
	FVector OutVelocity = InVelocity;

	FVector HorizontalVelocity(OutVelocity.X, OutVelocity.Y, 0.0f);
	const float IntoBlockerSpeed = FVector::DotProduct(HorizontalVelocity, BlockDir);
	const bool bStoppedRootMotion = IntoBlockerSpeed > 0.0f;

	if (bStoppedRootMotion)
	{
		OutVelocity.X = 0.0f;
		OutVelocity.Y = 0.0f;
	}

	const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0;
	if (CurrentTime < 0.0 || CurrentTime - LastSPRootMotionContactClampLogTime >= 0.05)
	{
		LastSPRootMotionContactClampLogTime = CurrentTime;
		const double ActiveFor = (SPRootMotionContactBlockStartTime >= 0.0 && CurrentTime >= 0.0)
			? CurrentTime - SPRootMotionContactBlockStartTime
			: -1.0;

		UE_LOG(LogTemp, Warning,
			TEXT("SP CMC RootMotionContactBlock ACTIVE %s PIE=%d Character=%s Action=%s ActiveFor=%.3f InVelocity=%s BlockDir=%s IntoBlockerSpeed=%.2f OutVelocity=%s Time=%.3f Role=%d Local=%d Auth=%d"),
			LogContext,
			SP_GetWorldPIEInstanceId(GetWorld()),
			*GetNameSafe(CharacterOwner),
			bStoppedRootMotion ? TEXT("Stopped") : TEXT("Allowed"),
			ActiveFor,
			*InVelocity.ToString(),
			*BlockDir.ToString(),
			IntoBlockerSpeed,
			*OutVelocity.ToString(),
			CurrentTime,
			CharacterOwner ? static_cast<int32>(CharacterOwner->GetLocalRole()) : -1,
			CharacterOwner ? CharacterOwner->IsLocallyControlled() : false,
			CharacterOwner ? CharacterOwner->HasAuthority() : false);
	}

	return OutVelocity;
}
