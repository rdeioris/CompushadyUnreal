// Copyright 2023 - Roberto De Ioris.

using UnrealBuildTool;

public class Compushady : ModuleRules
{
    public Compushady(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "RHI",
                "RenderCore",
                "SlateCore",
                "Slate",
                "VulkanRHI",
                "UMG"
            }
            );

        AddEngineThirdPartyPrivateStaticDependencies(Target, "Vulkan");

        string ThirdPartyDirectory = System.IO.Path.Combine(ModuleDirectory, "..", "ThirdParty");

        string ThirdPartyDirectoryIncludePath = ThirdPartyDirectory;

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string ThirdPartyDirectoryWin64 = System.IO.Path.Combine(ThirdPartyDirectory, "dxc_2023_03_01");
            string ThirdPartyDirectoryWin64Libs = System.IO.Path.Combine(ThirdPartyDirectoryWin64, "bin", "x64");
            ThirdPartyDirectoryIncludePath = System.IO.Path.Combine(ThirdPartyDirectoryWin64, "inc");
            RuntimeDependencies.Add("$(BinaryOutputDir)/dxcompiler.dll", System.IO.Path.Combine(ThirdPartyDirectoryWin64Libs, "dxcompiler.dll"));
            RuntimeDependencies.Add("$(BinaryOutputDir)/dxil.dll", System.IO.Path.Combine(ThirdPartyDirectoryWin64Libs, "dxil.dll"));

        }

        PrivateIncludePaths.Add(ThirdPartyDirectoryIncludePath);
    }
}
