// Copyright 2023-2026 - Roberto De Ioris.

using UnrealBuildTool;

public class CompushadyAudio : ModuleRules
{
    public CompushadyAudio(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Compushady",
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "RHI",
                "RenderCore",
                "AudioExtensions",
                "AudioMixer",
                "SignalProcessing",
            }
            );
    }
}
