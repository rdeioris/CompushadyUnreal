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
                "UMG",
                "AudioExtensions",
                "MediaAssets"
            }
            );

        AddEngineThirdPartyPrivateStaticDependencies(Target, "Vulkan");

        string ThirdPartyDirectory = System.IO.Path.Combine(ModuleDirectory, "..", "ThirdParty");

        string ThirdPartyDirectoryIncludePath = ThirdPartyDirectory;

        // DirectXCompiler
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string ThirdPartyDirectoryWin64 = System.IO.Path.Combine(ThirdPartyDirectory, "dxc_2023_03_01_windows");
            string ThirdPartyDirectoryWin64Libs = System.IO.Path.Combine(ThirdPartyDirectoryWin64, "bin", "x64");
            ThirdPartyDirectoryIncludePath = System.IO.Path.Combine(ThirdPartyDirectoryWin64, "inc");
            RuntimeDependencies.Add("$(BinaryOutputDir)/dxcompiler.dll", System.IO.Path.Combine(ThirdPartyDirectoryWin64Libs, "dxcompiler.dll"));
            RuntimeDependencies.Add("$(BinaryOutputDir)/dxil.dll", System.IO.Path.Combine(ThirdPartyDirectoryWin64Libs, "dxil.dll"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            string ThirdPartyDirectoryLinux = System.IO.Path.Combine(ThirdPartyDirectory, "dxc_2023_03_01_linux");
            string ThirdPartyDirectoryLinuxLibs = System.IO.Path.Combine(ThirdPartyDirectoryLinux, "lib");
            ThirdPartyDirectoryIncludePath = System.IO.Path.Combine(ThirdPartyDirectoryLinux, "include", "dxc");
            RuntimeDependencies.Add("$(BinaryOutputDir)/libdxcompiler.so", System.IO.Path.Combine(ThirdPartyDirectoryLinuxLibs, "libdxcompiler.so"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            string ThirdPartyDirectoryAndroid = System.IO.Path.Combine(ThirdPartyDirectory, "dxc_2023_03_01_android");
            ThirdPartyDirectoryIncludePath = System.IO.Path.Combine(ThirdPartyDirectoryAndroid, "include", "dxc");
            string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
            AdditionalPropertiesForReceipt.Add("AndroidPlugin", System.IO.Path.Combine(PluginPath, "Compushady_APL.xml"));
        }

        PrivateIncludePaths.Add(ThirdPartyDirectoryIncludePath);

        // libcompushady_khr
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string ThirdPartyDirectoryWin64 = System.IO.Path.Combine(ThirdPartyDirectory, "compushady_khr");
            RuntimeDependencies.Add("$(BinaryOutputDir)/libcompushady_khr.dll", System.IO.Path.Combine(ThirdPartyDirectoryWin64, "libcompushady_khr.dll"));
        }
    }
}
