// Copyright 2023 - Roberto De Ioris.

using UnrealBuildTool;

public class Compushady : ModuleRules
{
    public Compushady(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "MediaAssets",
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
                "SlateCore",
                "Slate",
                "UMG",
                "AudioExtensions",
            }
            );

        if (Target.Type == TargetType.Editor)
        {
            PrivateDependencyModuleNames.Add("Projects");
        }

        string ThirdPartyDirectory = System.IO.Path.Combine(ModuleDirectory, "..", "ThirdParty");

        string ThirdPartyDirectoryIncludePath = ThirdPartyDirectory;

        // DirectXCompiler
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicDependencyModuleNames.Add("AVCodecsCore");
            PrivateDependencyModuleNames.Add("AVCodecsCoreRHI");
            PrivateDependencyModuleNames.Add("VulkanRHI");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "Vulkan");
            string ThirdPartyDirectoryWin64 = System.IO.Path.Combine(ThirdPartyDirectory, "dxc_2023_03_01_windows");
            string ThirdPartyDirectoryWin64Libs = System.IO.Path.Combine(ThirdPartyDirectoryWin64, "bin", "x64");
            ThirdPartyDirectoryIncludePath = System.IO.Path.Combine(ThirdPartyDirectoryWin64, "inc");
            RuntimeDependencies.Add("$(BinaryOutputDir)/dxcompiler.dll", System.IO.Path.Combine(ThirdPartyDirectoryWin64Libs, "dxcompiler.dll"));
            RuntimeDependencies.Add("$(BinaryOutputDir)/dxil.dll", System.IO.Path.Combine(ThirdPartyDirectoryWin64Libs, "dxil.dll"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicDependencyModuleNames.Add("AVCodecsCore");
            PrivateDependencyModuleNames.Add("AVCodecsCoreRHI");
            PrivateDependencyModuleNames.Add("VulkanRHI");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "Vulkan");
            string ThirdPartyDirectoryLinux = System.IO.Path.Combine(ThirdPartyDirectory, "dxc_2023_03_01_linux");
            string ThirdPartyDirectoryLinuxLibs = System.IO.Path.Combine(ThirdPartyDirectoryLinux, "lib");
            ThirdPartyDirectoryIncludePath = System.IO.Path.Combine(ThirdPartyDirectoryLinux, "include", "dxc");
            RuntimeDependencies.Add("$(BinaryOutputDir)/libdxcompiler.so", System.IO.Path.Combine(ThirdPartyDirectoryLinuxLibs, "libdxcompiler.so"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PrivateDependencyModuleNames.Add("VulkanRHI");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "Vulkan");
            string ThirdPartyDirectoryAndroid = System.IO.Path.Combine(ThirdPartyDirectory, "dxc_2023_03_01_android");
            ThirdPartyDirectoryIncludePath = System.IO.Path.Combine(ThirdPartyDirectoryAndroid, "include", "dxc");
            string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
            AdditionalPropertiesForReceipt.Add("AndroidPlugin", System.IO.Path.Combine(PluginPath, "Compushady_APL.xml"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicDependencyModuleNames.Add("AVCodecsCore");
            PrivateDependencyModuleNames.Add("AVCodecsCoreRHI");
            string ThirdPartyDirectoryMac = System.IO.Path.Combine(ThirdPartyDirectory, "dxc_2023_03_01_mac");
            string ThirdPartyDirectoryMacLibs = System.IO.Path.Combine(ThirdPartyDirectoryMac, "lib");
            ThirdPartyDirectoryIncludePath = System.IO.Path.Combine(ThirdPartyDirectoryMac, "include", "dxc");
            RuntimeDependencies.Add("$(BinaryOutputDir)/libdxcompiler.dylib", System.IO.Path.Combine(ThirdPartyDirectoryMacLibs, "libdxcompiler.dylib"));
        }

        PrivateIncludePaths.Add(ThirdPartyDirectoryIncludePath);

        // compushady_khr
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string ThirdPartyDirectoryWin64 = System.IO.Path.Combine(ThirdPartyDirectory, "compushady_khr");
            RuntimeDependencies.Add("$(BinaryOutputDir)/compushady_khr.dll", System.IO.Path.Combine(ThirdPartyDirectoryWin64, "compushady_khr.dll"));
        }
    }
}
