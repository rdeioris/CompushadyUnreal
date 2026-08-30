// Copyright 2023-2026 - Roberto De Ioris.

#include "CompushadyGPUFencePoller.h"
#include "Misc/CoreDelegates.h"

FCompushadyGPUFencePoller& FCompushadyGPUFencePoller::Get()
{
	// Leaked on purpose: the instance may be referenced from ICompushadySignalable
	// destructors that run during engine/static teardown, so it must outlive them.
	static FCompushadyGPUFencePoller* Instance = new FCompushadyGPUFencePoller();
	return *Instance;
}

void FCompushadyGPUFencePoller::RegisterFence(const FGPUFenceRHIRef& Fence, TFunction<void()> OnCompleted, ICompushadySignalable* Owner)
{
	check(IsInGameThread());

	if (bShuttingDown)
	{
		return;
	}

	Pending.Add({ Fence, MoveTemp(OnCompleted), Owner });
	Arm();
}

void FCompushadyGPUFencePoller::Abandon(ICompushadySignalable* Owner)
{
	if (bShuttingDown || Owner == nullptr || Pending.Num() == 0)
	{
		return;
	}

	// Remove without firing the completion: the owner is being destroyed.
	Pending.RemoveAll([Owner](const FPendingFence& Entry)
		{
			return Entry.Owner == Owner;
		});
}

void FCompushadyGPUFencePoller::Arm()
{
	if (bArmed)
	{
		return;
	}

	bArmed = true;

	if (!bPreExitHooked)
	{
		bPreExitHooked = true;
		FCoreDelegates::OnEnginePreExit.AddRaw(this, &FCompushadyGPUFencePoller::Shutdown);
	}

	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FCompushadyGPUFencePoller::Tick));
}

bool FCompushadyGPUFencePoller::Tick(float DeltaTime)
{
	// Move the current set aside so that completion callbacks can safely RegisterFence()
	// re-dispatches back into Pending while we iterate.
	TArray<FPendingFence> ToProcess = MoveTemp(Pending);
	Pending.Reset();

	TArray<TFunction<void()>> Completed;
	Completed.Reserve(ToProcess.Num());

	for (FPendingFence& Entry : ToProcess)
	{
		// Mirrors FRHIGPUMemoryReadback::IsReady(): NumPendingWriteCommands guards the
		// game->render->RHI window where the WriteGPUFence has not been executed yet, and
		// Poll() reports whether the GPU has actually passed the fence.
		const bool bReady = !Entry.Fence.IsValid()
			|| (Entry.Fence->NumPendingWriteCommands.GetValue() == 0 && Entry.Fence->Poll());

		if (bReady)
		{
			Completed.Add(MoveTemp(Entry.OnCompleted));
		}
		else
		{
			Pending.Add(MoveTemp(Entry));
		}
	}

	// Fire completions only after the still-pending entries have been re-queued: a completion
	// may enqueue new GPU work (pushing into Pending) or destroy its owner (Abandon()).
	for (TFunction<void()>& Completion : Completed)
	{
		if (Completion)
		{
			Completion();
		}
	}

	if (Pending.Num() == 0)
	{
		// Nothing left in flight: disarm. Returning false removes this ticker; the next
		// RegisterFence() will re-arm it.
		bArmed = false;
		TickerHandle = FTSTicker::FDelegateHandle();
		return false;
	}

	return true;
}

void FCompushadyGPUFencePoller::Shutdown()
{
	bShuttingDown = true;

	if (bArmed)
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle = FTSTicker::FDelegateHandle();
		bArmed = false;
	}

	Pending.Empty();
}
