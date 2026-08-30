// Copyright 2023-2026 - Roberto De Ioris.

#include "CompushadyNiagaraModule.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FCompushadyNiagaraModule"

void FCompushadyNiagaraModule::StartupModule()
{
	// Map /Plugin/CompushadyNiagara -> <PluginRoot>/Shaders so the Niagara Data Interface
	// template (.ush) can be resolved by the shader compiler.
	// NOTE: the .uplugin file is named "Compushady", so that is the name to look up here
	// (not the "CompushadyUnreal" folder name).
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Compushady"));
	if (Plugin.IsValid())
	{
		const FString PluginShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
		AddShaderSourceDirectoryMapping(TEXT("/Plugin/CompushadyNiagara"), PluginShaderDir);
	}
}

void FCompushadyNiagaraModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCompushadyNiagaraModule, CompushadyNiagara)
