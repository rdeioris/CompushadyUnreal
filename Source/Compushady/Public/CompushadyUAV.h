// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "CompushadyTypes.h"
#include "UObject/NoExportTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "CompushadyUAV.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class COMPUSHADY_API UCompushadyUAV : public UObject, public ICompushadySignalable, public ICompushadyResource
{
	GENERATED_BODY()

public:
	bool InitializeFromTexture(FTextureRHIRef InTextureRHIRef);
	bool InitializeFromBuffer(FBufferRHIRef InBufferRHIRef, const EPixelFormat PixelFormat);
	bool InitializeFromStructuredBuffer(FBufferRHIRef InBufferRHIRef);

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void Readback();

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void CopyToRenderTarget2D(UTextureRenderTarget2D* RenderTarget, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void CopyToSRV(UCompushadySRV* SRV, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void CopyToStaticMeshPositions(UStaticMesh* StaticMesh, const int32 LOD, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void CopyToStaticMeshTexCoords(UStaticMesh* StaticMesh, const int32 LOD, const FCompushadySignaled& OnSignaled);

	FUnorderedAccessViewRHIRef GetRHI() const;

	bool CopyFromRHIBuffer(FBufferRHIRef SourceBufferRHIRef);

	void OnSignalReceived() override;
protected:
	FUnorderedAccessViewRHIRef UAVRHIRef;
};
