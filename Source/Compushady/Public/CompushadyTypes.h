// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureRenderTarget2DArray.h"
#include "MediaTexture.h"
#include "CompushadyTypes.generated.h"

/**
 *
 */
USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyFloat2
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Y;

};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyFloat3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Z;
};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyFloat4
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float W;
};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyCopyInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FIntVector SourceOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FIntVector SourceSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FIntVector DestinationOffset;
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCompushadySignaled, bool, bSuccess, const FString&, ErrorMessage);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FCompushadySignaledWithFloatPayload, bool, bSuccess, float&, Payload, const FString&, ErrorMessage);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FCompushadySignaledWithFloatArrayPayload, bool, bSuccess, const TArray<float>&, Payload, const FString&, ErrorMessage);


class COMPUSHADY_API ICompushadySignalable
{
public:
	virtual ~ICompushadySignalable() = default;
	bool InitFence(UObject* InOwningObject);
	void ClearFence();
	void CheckFence(FCompushadySignaled OnSignal);
	void CheckFence(FCompushadySignaledWithFloatPayload OnSignal, TArray<uint8>& Data);
	void CheckFence(FCompushadySignaledWithFloatArrayPayload OnSignal, TArray<uint8>& Data);
	void WriteFence(FRHICommandListImmediate& RHICmdList);
	virtual void OnSignalReceived() = 0;
protected:
	bool CopyTexture_Internal(FTextureRHIRef Destination, FTextureRHIRef Source, const FCompushadyCopyInfo& CopyInfo, const FCompushadySignaled& OnSignaled);

	bool bRunning;
	FGPUFenceRHIRef FenceRHIRef;
	TWeakObjectPtr<UObject> OwningObject;
};

UCLASS(Abstract)
class COMPUSHADY_API UCompushadyResource : public UObject, public ICompushadySignalable
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void ReadbackToFloatArray(const int32 Offset, const int32 Elements, const FCompushadySignaledWithFloatArrayPayload& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void ReadbackAllToFloatArray(const FCompushadySignaledWithFloatArrayPayload& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void ReadbackAllToFile(const FString& Filename, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void ReadbackTextureToPngFile(const FString& Filename, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void CopyToRenderTarget2D(UTextureRenderTarget2D* RenderTarget, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "OnSignaled"), Category = "Compushady")
	void CopyToRenderTarget2DArray(UTextureRenderTarget2DArray* RenderTargetArray, const int32 Slice, const FCompushadySignaled& OnSignaled);

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "CopyInfo", AutoCreateRefTerm = "OnSignaled,CopyInfo"), Category = "Compushady")
	void CopyFromMediaTexture(UMediaTexture* MediaTexture, const FCompushadySignaled& OnSignaled, const FCompushadyCopyInfo& CopyInfo);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	FIntVector GetTextureThreadGroupSize(const FIntVector XYZ);

	FTextureRHIRef GetTextureRHI() const;
	FBufferRHIRef GetBufferRHI() const;

	const FRHITransitionInfo& GetRHITransitionInfo() const;

	FStagingBufferRHIRef GetStagingBuffer();
	FTextureRHIRef GetReadbackTexture();

	void OnSignalReceived() override;
protected:
	FTextureRHIRef TextureRHIRef;
	FBufferRHIRef BufferRHIRef;
	FStagingBufferRHIRef StagingBufferRHIRef;
	FRHITransitionInfo RHITransitionInfo;
	TArray<uint8> ReadbackCache;
	FTextureRHIRef ReadbackTextureRHIRef;
};