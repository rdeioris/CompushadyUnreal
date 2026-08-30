// Copyright 2023-2026 - Roberto De Ioris.

#include "NiagaraDataInterfaceCompushady.h"

#include "CompushadySRV.h"

#include "NiagaraShaderParametersBuilder.h"
#include "NiagaraRenderer.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraCompileHashVisitor.h" // full definition of FNiagaraCompileHashVisitor (only fwd-declared in NiagaraTypes.h)
#include "ShaderCore.h"
#include "ShaderParameterMacros.h"

#define LOCTEXT_NAMESPACE "UNiagaraDataInterfaceCompushady"

//////////////////////////////////////////////////////////////////////////
// Shader parameters bound to the Niagara GPU simulation.
// The whole buffer is exposed as a single flat Buffer<float> SRV; there is no
// layout metadata - the Niagara graph decides what every float index means.

BEGIN_SHADER_PARAMETER_STRUCT(FNDICompushadyShaderParameters, )
	SHADER_PARAMETER_SRV(Buffer<float>, FloatBuffer)
END_SHADER_PARAMETER_STRUCT()

// Virtual path mapped in FCompushadyNiagaraModule::StartupModule().
static const TCHAR* GCompushadyNiagaraTemplateShader = TEXT("/Plugin/CompushadyNiagara/NiagaraDataInterfaceCompushady.ush");

//////////////////////////////////////////////////////////////////////////
// Function names (must match the definitions in the .ush template)

static const FName GetPositionName(TEXT("GetPosition"));
static const FName GetColorName(TEXT("GetColor"));
static const FName GetFloatName(TEXT("GetFloat"));
static const FName GetIntName(TEXT("GetInt"));
static const FName GetFloat2Name(TEXT("GetFloat2"));
static const FName GetFloat3Name(TEXT("GetFloat3"));
static const FName GetFloat4Name(TEXT("GetFloat4"));

//////////////////////////////////////////////////////////////////////////
// Render-thread proxy + per-instance data plumbing

// Data snapshot handed from the game thread to the render thread every tick.
struct FNDICompushadyGameToRenderData
{
	FShaderResourceViewRHIRef SRV;
};

// Live render-thread state per system instance.
struct FNDICompushadyInstanceData_RT
{
	FShaderResourceViewRHIRef SRV;
};

struct FNDICompushadyProxy : public FNiagaraDataInterfaceProxy
{
	virtual int32 PerInstanceDataPassedToRenderThreadSize() const override
	{
		return sizeof(FNDICompushadyGameToRenderData);
	}

	virtual void ConsumePerInstanceDataFromGameThread(void* PerInstanceData, const FNiagaraSystemInstanceID& Instance) override
	{
		FNDICompushadyGameToRenderData* Incoming = static_cast<FNDICompushadyGameToRenderData*>(PerInstanceData);

		FNDICompushadyInstanceData_RT& Data = SystemInstancesToProxyData_RT.FindOrAdd(Instance);
		Data.SRV = Incoming->SRV;

		// Release the ref-counted SRV that was placement-new'd on the game thread.
		Incoming->~FNDICompushadyGameToRenderData();
	}

	TMap<FNiagaraSystemInstanceID, FNDICompushadyInstanceData_RT> SystemInstancesToProxyData_RT;
};

//////////////////////////////////////////////////////////////////////////
// Game-thread per-instance data (nothing to keep, but its presence drives the
// per-tick ProvidePerInstanceDataForRenderThread pump).

struct FNDICompushadyInstanceData_GT
{
	uint8 Dummy = 0;
};

//////////////////////////////////////////////////////////////////////////

UNiagaraDataInterfaceCompushady::UNiagaraDataInterfaceCompushady(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Proxy.Reset(new FNDICompushadyProxy());
}

void UNiagaraDataInterfaceCompushady::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		ENiagaraTypeRegistryFlags Flags = ENiagaraTypeRegistryFlags::AllowAnyVariable | ENiagaraTypeRegistryFlags::AllowParameter;
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), Flags);
	}

	MarkRenderDataDirty();
}

bool UNiagaraDataInterfaceCompushady::Equals(const UNiagaraDataInterface* Other) const
{
	if (!Super::Equals(Other))
	{
		return false;
	}

	const UNiagaraDataInterfaceCompushady* OtherTyped = CastChecked<const UNiagaraDataInterfaceCompushady>(Other);
	return OtherTyped->SourceSRV == SourceSRV;
}

bool UNiagaraDataInterfaceCompushady::CopyToInternal(UNiagaraDataInterface* Destination) const
{
	if (!Super::CopyToInternal(Destination))
	{
		return false;
	}

	UNiagaraDataInterfaceCompushady* DestTyped = CastChecked<UNiagaraDataInterfaceCompushady>(Destination);
	DestTyped->SourceSRV = SourceSRV;

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Per-instance data (game thread)

int32 UNiagaraDataInterfaceCompushady::PerInstanceDataSize() const
{
	return sizeof(FNDICompushadyInstanceData_GT);
}

bool UNiagaraDataInterfaceCompushady::InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	new(PerInstanceData) FNDICompushadyInstanceData_GT();
	return true;
}

void UNiagaraDataInterfaceCompushady::DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	FNDICompushadyInstanceData_GT* InstanceData = static_cast<FNDICompushadyInstanceData_GT*>(PerInstanceData);
	InstanceData->~FNDICompushadyInstanceData_GT();
}

void UNiagaraDataInterfaceCompushady::ProvidePerInstanceDataForRenderThread(void* DataForRenderThread, void* PerInstanceData, const FNiagaraSystemInstanceID& SystemInstance)
{
	FNDICompushadyGameToRenderData* Data = new(DataForRenderThread) FNDICompushadyGameToRenderData();

	if (SourceSRV)
	{
		Data->SRV = SourceSRV->GetRHI();
	}
}

//////////////////////////////////////////////////////////////////////////
// GPU shader integration

void UNiagaraDataInterfaceCompushady::BuildShaderParameters(FNiagaraShaderParametersBuilder& ShaderParametersBuilder) const
{
	ShaderParametersBuilder.AddNestedStruct<FNDICompushadyShaderParameters>();
}

void UNiagaraDataInterfaceCompushady::SetShaderParameters(const FNiagaraDataInterfaceSetShaderParametersContext& Context) const
{
	FNDICompushadyProxy& DIProxy = Context.GetProxy<FNDICompushadyProxy>();
	FNDICompushadyInstanceData_RT* InstanceData = DIProxy.SystemInstancesToProxyData_RT.Find(Context.GetSystemInstanceID());

	FNDICompushadyShaderParameters* Parameters = Context.GetParameterNestedStruct<FNDICompushadyShaderParameters>();

	if (InstanceData && InstanceData->SRV.IsValid())
	{
		Parameters->FloatBuffer = InstanceData->SRV;
	}
	else
	{
		// Dummy SRV so the binding is always valid even before a buffer is assigned.
		Parameters->FloatBuffer = FNiagaraRenderer::GetDummyFloatBuffer();
	}
}

#if WITH_EDITORONLY_DATA
void UNiagaraDataInterfaceCompushady::GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL)
{
	// AppendTemplateHLSL loads the .ush and substitutes {ParameterName} (token replace, so the
	// HLSL braces in the template survive). This declares both the parameters and the functions.
	const TMap<FString, FStringFormatArg> TemplateArgs =
	{
		{TEXT("ParameterName"), ParamInfo.DataInterfaceHLSLSymbol},
	};
	AppendTemplateHLSL(OutHLSL, GCompushadyNiagaraTemplateShader, TemplateArgs);
}

bool UNiagaraDataInterfaceCompushady::GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, int FunctionInstanceIndex, FString& OutHLSL)
{
	// All function bodies live in the template appended by GetParameterDefinitionHLSL, so we
	// only need to acknowledge that we own these functions.
	if (FunctionInfo.DefinitionName == GetPositionName ||
		FunctionInfo.DefinitionName == GetColorName ||
		FunctionInfo.DefinitionName == GetFloatName ||
		FunctionInfo.DefinitionName == GetIntName ||
		FunctionInfo.DefinitionName == GetFloat2Name ||
		FunctionInfo.DefinitionName == GetFloat3Name ||
		FunctionInfo.DefinitionName == GetFloat4Name)
	{
		return true;
	}

	return false;
}

bool UNiagaraDataInterfaceCompushady::AppendCompileHash(FNiagaraCompileHashVisitor* InVisitor) const
{
	bool bSuccess = Super::AppendCompileHash(InVisitor);
	bSuccess &= InVisitor->UpdateShaderFile(GCompushadyNiagaraTemplateShader);
	bSuccess &= InVisitor->UpdateShaderParameters<FNDICompushadyShaderParameters>();
	return bSuccess;
}

void UNiagaraDataInterfaceCompushady::GetFunctionsInternal(TArray<FNiagaraFunctionSignature>& OutFunctions) const
{
	const FNiagaraTypeDefinition DIType(GetClass());

	auto MakeBaseSig = [&DIType](const FName InName) -> FNiagaraFunctionSignature
	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = InName;
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		Sig.bSupportsCPU = false;   // GPU only - reads a GPU SRV
		Sig.bSupportsGPU = true;
		Sig.Inputs.Add(FNiagaraVariable(DIType, TEXT("Compushady")));
		return Sig;
	};

	// Position: 3 floats at FloatIndex, typed as a Position so the pin drives Particles.Position.
	{
		FNiagaraFunctionSignature Sig = MakeBaseSig(GetPositionName);
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("FloatIndex")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetPositionDef(), TEXT("Position")));
		OutFunctions.Add(Sig);
	}

	// Color: 4 floats at FloatIndex, typed as a LinearColor.
	{
		FNiagaraFunctionSignature Sig = MakeBaseSig(GetColorName);
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("FloatIndex")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(), TEXT("Color")));
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig = MakeBaseSig(GetFloatName);
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("FloatIndex")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Value")));
		OutFunctions.Add(Sig);
	}

	// Integer scalar: raw 32 bits at FloatIndex reinterpreted as int (Niagara has no uint type,
	// so unsigned values ride on this same int output bit-for-bit).
	{
		FNiagaraFunctionSignature Sig = MakeBaseSig(GetIntName);
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("FloatIndex")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Value")));
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig = MakeBaseSig(GetFloat2Name);
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("FloatIndex")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), TEXT("Value")));
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig = MakeBaseSig(GetFloat3Name);
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("FloatIndex")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Value")));
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig = MakeBaseSig(GetFloat4Name);
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("FloatIndex")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("Value")));
		OutFunctions.Add(Sig);
	}
}
#endif // WITH_EDITORONLY_DATA

#undef LOCTEXT_NAMESPACE
