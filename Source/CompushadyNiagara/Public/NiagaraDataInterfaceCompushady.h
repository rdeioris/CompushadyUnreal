// Copyright 2023-2026 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraDataInterface.h"
#include "NiagaraCommon.h"
#include "RHIResources.h"
#include "NiagaraDataInterfaceCompushady.generated.h"

class UCompushadySRV;
class UCompushadyResource;

/**
 * Zero-copy bridge between a Compushady GPU buffer and a Niagara GPUCompute simulation.
 *
 * A Compushady compute dispatch fills a UAV (typed RWBuffer<float> or an equivalent) with
 * per-particle data. An SRV that aliases the *same* FRHIBuffer (e.g. created with
 * UCompushadyFunctionLibrary::CreateCompushadySRVFromResource) is handed to this Data
 * Interface. Niagara particles then read straight from that SRV - no CPU readback and no
 * GPU->GPU copy.
 *
 * The interface is deliberately layout-agnostic: the buffer is exposed as a flat
 * Buffer<float>, and every accessor takes a raw float index into that buffer. Any notion of
 * stride, particle count, or where position/color live inside an element is entirely the
 * caller's business, expressed in the Niagara graph. GetFloat/GetFloat2/GetFloat3/GetFloat4
 * read N consecutive floats at the given index; GetPosition/GetColor are the same reads typed
 * as a Position / LinearColor so their pins connect straight to Particles.Position / .Color.
 *
 * GPU only: the accessor functions are not implemented for the CPU VM, so any emitter using
 * this interface must have its SimTarget set to GPUCompute.
 */
UCLASS(EditInlineNew, Category = "Compushady", meta = (DisplayName = "Compushady Buffer"))
class COMPUSHADYNIAGARA_API UNiagaraDataInterfaceCompushady : public UNiagaraDataInterface
{
	GENERATED_BODY()

public:
	UNiagaraDataInterfaceCompushady(const FObjectInitializer& ObjectInitializer);

	/** The read-only view (Buffer<float>) aliasing the Compushady buffer to read from. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	TObjectPtr<UCompushadySRV> SourceSRV = nullptr;

	//~ UObject interface
	virtual void PostInitProperties() override;

	//~ UNiagaraDataInterface interface
	virtual bool CanExecuteOnTarget(ENiagaraSimTarget Target) const override { return Target == ENiagaraSimTarget::GPUComputeSim; }

	virtual bool Equals(const UNiagaraDataInterface* Other) const override;

	// Per-instance (game thread) plumbing so we can push the SRV to the render thread each tick.
	virtual int32 PerInstanceDataSize() const override;
	virtual bool InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
	virtual void DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;

	virtual void ProvidePerInstanceDataForRenderThread(void* DataForRenderThread, void* PerInstanceData, const FNiagaraSystemInstanceID& SystemInstance) override;

	// GPU shader integration
	virtual void BuildShaderParameters(FNiagaraShaderParametersBuilder& ShaderParametersBuilder) const override;
	virtual void SetShaderParameters(const FNiagaraDataInterfaceSetShaderParametersContext& Context) const override;

#if WITH_EDITORONLY_DATA
	virtual void GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL) override;
	virtual bool GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, int FunctionInstanceIndex, FString& OutHLSL) override;
	virtual bool AppendCompileHash(FNiagaraCompileHashVisitor* InVisitor) const override;
#endif

protected:
#if WITH_EDITORONLY_DATA
	// NOTE (5.6/5.7): the function-signature provider is GetFunctionsInternal. On some 5.2/5.3
	// builds this virtual is the public GetFunctions(TArray<FNiagaraFunctionSignature>&). If it
	// fails to compile, rename this to GetFunctions and drop the "Internal" suffix.
	virtual void GetFunctionsInternal(TArray<FNiagaraFunctionSignature>& OutFunctions) const override;
#endif

	virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;
};
