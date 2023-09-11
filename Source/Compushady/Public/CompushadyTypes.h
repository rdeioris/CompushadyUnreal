// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Compushady.h"
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
	float X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Y = 0;

};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyFloat3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Y = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Z = 0;
};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyFloat4
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Y = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float Z = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	float W = 0;
};

USTRUCT(BlueprintType)
struct COMPUSHADY_API FCompushadyTextureCopyInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FIntVector SourceOffset{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FIntVector SourceSize{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 SourceSlice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FIntVector DestinationOffset{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 DestinationSlice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	int32 NumSlices = 0;
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
	bool CopyTexture_Internal(FTextureRHIRef Destination, FTextureRHIRef Source, const FCompushadyTextureCopyInfo& CopyInfo, const FCompushadySignaled& OnSignaled);

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

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "CopyInfo", AutoCreateRefTerm = "OnSignaled,CopyInfo"), Category = "Compushady")
	void CopyToRenderTarget2D(UTextureRenderTarget2D* RenderTarget, const FCompushadySignaled& OnSignaled, const FCompushadyTextureCopyInfo& CopyInfo);

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "CopyInfo", AutoCreateRefTerm = "OnSignaled,CopyInfo"), Category = "Compushady")
	void CopyToRenderTarget2DArray(UTextureRenderTarget2DArray* RenderTargetArray, const FCompushadySignaled& OnSignaled, const FCompushadyTextureCopyInfo& CopyInfo);

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "CopyInfo", AutoCreateRefTerm = "OnSignaled,CopyInfo"), Category = "Compushady")
	void CopyFromMediaTexture(UMediaTexture* MediaTexture, const FCompushadySignaled& OnSignaled, const FCompushadyTextureCopyInfo& CopyInfo);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	FIntVector GetTextureThreadGroupSize(const FIntVector XYZ, const bool bUseNumSlicesForZ) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	FIntVector GetTextureSize() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int64 GetBufferSize() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	int32 GetTextureNumSlices() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	EPixelFormat GetTexturePixelFormat() const;

	FTextureRHIRef GetTextureRHI() const;
	FBufferRHIRef GetBufferRHI() const;

	const FRHITransitionInfo& GetRHITransitionInfo() const;

	FStagingBufferRHIRef GetStagingBuffer();
	FBufferRHIRef GetUploadBuffer(FRHICommandListImmediate& RHICmdList);
	FTextureRHIRef GetReadbackTexture();

	bool IsValidTexture() const;
	bool IsValidBuffer() const;

	void OnSignalReceived() override;

	void MapReadAndExecute(TFunction<void(const void*)> InFunction, const FCompushadySignaled& OnSignaled);
	void MapReadAndExecuteInGameThread(TFunction<void(const void*)> InFunction, const FCompushadySignaled& OnSignaled);
	bool MapReadAndExecuteSync(TFunction<void(const void*)> InFunction);

	void MapWriteAndExecute(TFunction<void(void*)> InFunction, const FCompushadySignaled& OnSignaled);
	void MapWriteAndExecuteInGameThread(TFunction<void(void*)> InFunction, const FCompushadySignaled& OnSignaled);
	bool MapWriteAndExecuteSync(TFunction<void(void*)> InFunction);

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "UpdateInfo", AutoCreateRefTerm = "UpdateInfo"), Category = "Compushady")
	bool UpdateTextureSliceSync(const TArray<uint8>& Pixels, const int32 Slice);

	bool UpdateTextureSliceSync(const uint8* Ptr, const int64 Size, const int32 Slice);

	bool MapTextureSliceAndExecuteSync(TFunction<void(const void*, const int32)> InFunction, const int32 Slice);

protected:
	FTextureRHIRef TextureRHIRef;
	FBufferRHIRef BufferRHIRef;
	FStagingBufferRHIRef StagingBufferRHIRef;
	FBufferRHIRef UploadBufferRHIRef;
	FRHITransitionInfo RHITransitionInfo;
	FTextureRHIRef ReadbackTextureRHIRef;
	TArray<uint8> ReadbackCache;
};