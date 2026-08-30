// Copyright 2023-2026 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "RHIResources.h"        // FGPUFenceRHIRef, FRHIGPUFence
#include "Containers/Ticker.h"   // FTSTicker
#include "Compushady.h"

class ICompushadySignalable;

/**
 * Non-stalling GPU-completion dispatcher.
 *
 * Replaces the previous "dispatch an empty render-thread task and treat *its* completion
 * as GPU completion" mechanism (ICompushadySignalable::BeginFence) with a real
 * FRHIGPUFence that is polled once per frame from the game thread through FTSTicker.
 *
 * Why:
 *  - The old BeginFence fired the OnSignaled delegate as soon as the render thread had
 *    finished *recording* the commands, not when the GPU had finished *executing* them.
 *    For readbacks this was papered over with a full BlockUntilGPUIdle() stall.
 *  - Polling a fence lets us know when the GPU is actually done without stalling either
 *    the game thread or the render thread (see FRHIGPUFence::Poll() docs).
 *
 * Lifetime / threading:
 *  - Single leaked-on-exit instance (Get()). It is intentionally never destroyed so it can
 *    be safely touched from ICompushadySignalable destructors that run during engine/static
 *    teardown.
 *  - The core ticker is armed lazily on the first RegisterFence() and removed automatically
 *    as soon as no fences remain in flight, so there is zero per-frame cost while idle.
 *  - Game-thread only. RegisterFence()/Abandon() are called while enqueuing GPU work from
 *    the game thread; Tick() runs on the game thread.
 */
class COMPUSHADY_API FCompushadyGPUFencePoller
{
public:
	static FCompushadyGPUFencePoller& Get();

	/**
	 * Register a GPU fence to be polled. When the GPU signals it, OnCompleted is invoked on
	 * the game thread during the core ticker tick.
	 *
	 * @param Fence        The fence written on the render thread right after the tracked GPU work.
	 * @param OnCompleted  Game-thread callback fired once the fence is signalled.
	 * @param Owner        Optional. When non-null, Abandon(Owner) can drop still-pending entries
	 *                     if the owner is destroyed before the GPU signals (avoids use-after-free).
	 */
	void RegisterFence(const FGPUFenceRHIRef& Fence, TFunction<void()> OnCompleted, ICompushadySignalable* Owner = nullptr);

	/** Drop (without firing) every pending entry owned by Owner. Call from the owner's destructor. */
	void Abandon(ICompushadySignalable* Owner);

	/** Number of fences currently in flight (mostly for tests/diagnostics). */
	int32 NumPending() const { return Pending.Num(); }

private:
	FCompushadyGPUFencePoller() = default;
	FCompushadyGPUFencePoller(const FCompushadyGPUFencePoller&) = delete;
	FCompushadyGPUFencePoller& operator=(const FCompushadyGPUFencePoller&) = delete;

	bool Tick(float DeltaTime);
	void Arm();
	void Shutdown();

	struct FPendingFence
	{
		FGPUFenceRHIRef Fence;
		TFunction<void()> OnCompleted;
		ICompushadySignalable* Owner = nullptr;
	};

	TArray<FPendingFence> Pending;
	FTSTicker::FDelegateHandle TickerHandle;
	bool bArmed = false;
	bool bShuttingDown = false;
	bool bPreExitHooked = false;
};
