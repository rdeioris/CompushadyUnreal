// Copyright 2023-2026 - Roberto De Ioris.

#include "Modules/ModuleManager.h"
#include "CompushadyBlitter.h"
#include "CompushadyBlitterSubsystem.h"
#include "CompushadyTypes.h"
#include "Engine/World.h"

// Registers the Compushady::Blitter draw sink so UCompushadyResource::Draw*
// (UFUNCTIONs in the Compushady module — see the seam comment in
// CompushadyTypes.h) can reach this module's blitter subsystem without a
// circular module dependency.
class FCompushadyBlitterModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		Compushady::Blitter::RegisterDrawSink(
			[](UCompushadyResource* Resource, UObject* WorldContextObject,
			   const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio,
			   const FString& Label, const Compushady::Blitter::EDrawPhase Phase) -> FGuid
			{
				UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
				UCompushadyBlitterSubsystem* Subsystem = World ? World->GetSubsystem<UCompushadyBlitterSubsystem>() : nullptr;
				if (!Subsystem)
				{
					return FGuid();
				}
				switch (Phase)
				{
				case Compushady::Blitter::EDrawPhase::BeforePostProcessing:
					return Subsystem->AddBeforePostProcessingDrawable(Resource, Quad, KeepAspectRatio);
				case Compushady::Blitter::EDrawPhase::AfterMotionBlur:
					return Subsystem->AddAfterMotionBlurDrawable(Resource, Quad, KeepAspectRatio);
				default:
					return Subsystem->AddDrawable(Resource, Quad, KeepAspectRatio, Label);
				}
			});
	}

	virtual void ShutdownModule() override
	{
		Compushady::Blitter::UnregisterDrawSink();
	}
};

IMPLEMENT_MODULE(FCompushadyBlitterModule, CompushadyBlitter)

DEFINE_LOG_CATEGORY(LogCompushadyBlitter);
