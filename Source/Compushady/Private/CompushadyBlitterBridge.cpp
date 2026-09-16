// Copyright 2023-2026 - Roberto De Ioris.
//
// UCompushadyResource::Draw* — the reflection-callable entry points for
// on-screen blitting (with optional text label) — forwarding through the
// Compushady::Blitter draw sink. See the seam comment in CompushadyTypes.h:
// the implementations cannot live in the CompushadyBlitter module (their
// UHT thunks link into this DLL) and this module cannot depend on the
// blitter (it depends on us), so the blitter registers a sink at startup.

#include "Compushady.h"
#include "CompushadyTypes.h"

namespace Compushady::Blitter
{
	static FDrawSink GDrawSink;

	void RegisterDrawSink(FDrawSink InSink)
	{
		GDrawSink = MoveTemp(InSink);
	}

	void UnregisterDrawSink()
	{
		GDrawSink = nullptr;
	}

	static FGuid Dispatch(UCompushadyResource* Resource, UObject* WorldContextObject,
		const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio,
		const FString& Label, const EDrawPhase Phase)
	{
		if (!GDrawSink)
		{
			UE_LOG(LogCompushady, Warning,
				TEXT("UCompushadyResource::Draw called but the CompushadyBlitter module has not registered its draw sink"));
			return FGuid();
		}
		return GDrawSink(Resource, WorldContextObject, Quad, KeepAspectRatio, Label, Phase);
	}
}

FGuid UCompushadyResource::Draw(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio, const FString& Label)
{
	return Compushady::Blitter::Dispatch(this, WorldContextObject, Quad, KeepAspectRatio, Label, Compushady::Blitter::EDrawPhase::Default);
}

FGuid UCompushadyResource::DrawBeforePostProcessing(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	return Compushady::Blitter::Dispatch(this, WorldContextObject, Quad, KeepAspectRatio, FString(), Compushady::Blitter::EDrawPhase::BeforePostProcessing);
}

FGuid UCompushadyResource::DrawAfterMotionBlur(UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	return Compushady::Blitter::Dispatch(this, WorldContextObject, Quad, KeepAspectRatio, FString(), Compushady::Blitter::EDrawPhase::AfterMotionBlur);
}
