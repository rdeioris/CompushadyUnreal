// Copyright 2023-2024 - Roberto De Ioris.

using UnrealBuildTool;

public class CompushadyBlueprintNodes : ModuleRules
{
    public CompushadyBlueprintNodes(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Compushady",
                "BlueprintGraph",
                "SlateCore",
                "Slate",
                "GraphEditor",
                "UnrealEd",
                "KismetCompiler"
            }
            );
    }
}
