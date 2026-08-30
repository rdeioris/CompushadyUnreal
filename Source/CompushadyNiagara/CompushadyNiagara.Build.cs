// Copyright 2023-2026 - Roberto De Ioris.

using UnrealBuildTool;

public class CompushadyNiagara : ModuleRules
{
    public CompushadyNiagara(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Compushady",
                "Niagara",
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
                "NiagaraCore",
                "NiagaraShader",
                "Projects", // IPluginManager, for the virtual shader path mapping
            }
            );
    }
}
