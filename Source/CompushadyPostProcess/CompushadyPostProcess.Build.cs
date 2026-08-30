// Copyright 2023-2026 - Roberto De Ioris.

using UnrealBuildTool;

public class CompushadyPostProcess : ModuleRules
{
    public CompushadyPostProcess(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Compushady",
                "CompushadyBlitter",
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "RHI",
                "RenderCore",
                "Renderer",
            }
            );
    }
}
