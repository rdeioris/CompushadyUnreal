// Copyright 2023-2026 - Roberto De Ioris.

using UnrealBuildTool;

public class CompushadyShared : ModuleRules
{
    public CompushadyShared(ReadOnlyTargetRules Target) : base(Target)
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
                "Renderer",
            }
            );

        // D3D12 zero-copy backend (Windows only). Depending on the D3D12RHI module gives us
        // ID3D12DynamicRHI.h and the DX12 third-party headers (d3d12.h) needed to export
        // ID3D12Resource / ID3D12Fence NT shared handles.
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PrivateDependencyModuleNames.Add("D3D12RHI");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
            PublicDefinitions.Add("COMPUSHADY_SHARED_WITH_D3D12=1");
        }
        else
        {
            PublicDefinitions.Add("COMPUSHADY_SHARED_WITH_D3D12=0");
        }

        // Vulkan external-memory / external-semaphore backend.
        if (Target.Platform == UnrealTargetPlatform.Win64 ||
            Target.Platform == UnrealTargetPlatform.Linux ||
            Target.Platform == UnrealTargetPlatform.Android)
        {
            PrivateDependencyModuleNames.Add("VulkanRHI");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "Vulkan");
            PublicDefinitions.Add("COMPUSHADY_SHARED_WITH_VULKAN=1");
        }
        else
        {
            PublicDefinitions.Add("COMPUSHADY_SHARED_WITH_VULKAN=0");
        }
    }
}
