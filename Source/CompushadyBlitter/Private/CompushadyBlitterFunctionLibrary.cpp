// Copyright 2023-2026 - Roberto De Ioris.

#include "CompushadyBlitterFunctionLibrary.h"
#include "CompushadyBlitterSubsystem.h"
#include "CompushadyCBV.h"
#include "CompushadyFunctionLibrary.h"
#include "Compushady.h"
#include "Engine/World.h"

FGuid UCompushadyBlitterFunctionLibrary::Draw(UCompushadyResource* Resource, UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	return WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddDrawable(Resource, Quad, KeepAspectRatio);
}

FGuid UCompushadyBlitterFunctionLibrary::DrawBeforePostProcessing(UCompushadyResource* Resource, UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	return WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddBeforePostProcessingDrawable(Resource, Quad, KeepAspectRatio);
}

FGuid UCompushadyBlitterFunctionLibrary::DrawAfterMotionBlur(UCompushadyResource* Resource, UObject* WorldContextObject, const FVector4 Quad, const ECompushadyKeepAspectRatio KeepAspectRatio)
{
	return WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->AddAfterMotionBlurDrawable(Resource, Quad, KeepAspectRatio);
}

bool UCompushadyBlitterFunctionLibrary::SetProjectionMatrixFromViewport(UCompushadyCBV* CBV, UObject* WorldContextObject, const int64 Offset, const bool bTranspose, const bool bInverse)
{
	if (!CBV || !WorldContextObject)
	{
		return false;
	}
	return CBV->SetMatrixFloat(Offset, WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->GetProjectionMatrix(), bTranspose, bInverse);
}

bool UCompushadyBlitterFunctionLibrary::SetViewMatrixFromViewport(UCompushadyCBV* CBV, UObject* WorldContextObject, const int64 Offset, const bool bTranspose, const bool bInverse)
{
	if (!CBV || !WorldContextObject)
	{
		return false;
	}
	return CBV->SetMatrixFloat(Offset, WorldContextObject->GetWorld()->GetSubsystem<UCompushadyBlitterSubsystem>()->GetViewMatrix(), bTranspose, bInverse);
}

UCompushadyBlendable* UCompushadyBlitterFunctionLibrary::CreateCompushadyBlendableFromHLSLString(const FString& PixelShaderSource, const FCompushadyResourceArray& PSResourceArray, FString& ErrorMessages, const FString& PixelShaderEntryPoint, const ECompushadyPostProcessLocation PostProcessLocation)
{
	UCompushadyBlendable* CompushadyBlendable = NewObject<UCompushadyBlendable>();

	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode(PixelShaderSource, ShaderCode);

	if (!CompushadyBlendable->InitFromHLSL(ShaderCode, PixelShaderEntryPoint, PostProcessLocation, ErrorMessages))
	{
		return nullptr;
	}

	if (!CompushadyBlendable->UpdateResources(PSResourceArray, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyBlendable;
}

UCompushadyBlendable* UCompushadyBlitterFunctionLibrary::CreateCompushadyAdvancedBlendableFromHLSLString(const FString& VertexShaderSource, const FCompushadyResourceArray& VSResourceArray, const FString& PixelShaderSource, const FCompushadyResourceArray& PSResourceArray, FString& ErrorMessages, const FCompushadyBlendableRasterizerConfig& BlendableRasterizerConfig, const int32 NumVertices, const int32 NumInstances, const FString& VertexShaderEntryPoint, const FString& PixelShaderEntryPoint, const ECompushadyPostProcessLocation PostProcessLocation)
{
	TArray<uint8> VertexShaderCode;
	Compushady::StringToShaderCode(VertexShaderSource, VertexShaderCode);

	TArray<uint8> PixelShaderCode;
	Compushady::StringToShaderCode(PixelShaderSource, PixelShaderCode);

	UCompushadyBlendable* CompushadyBlendable = NewObject<UCompushadyBlendable>();

	if (!CompushadyBlendable->InitFromHLSLAdvanced(VertexShaderCode, VertexShaderEntryPoint, PixelShaderCode, PixelShaderEntryPoint, PostProcessLocation, ErrorMessages))
	{
		return nullptr;
	}

	if (!CompushadyBlendable->UpdateResourcesAdvanced(VSResourceArray, PSResourceArray, NumVertices, NumInstances, BlendableRasterizerConfig, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyBlendable;
}

UCompushadyBlendable* UCompushadyBlitterFunctionLibrary::CreateCompushadyAdvancedBlendableByMapFromHLSLString(const FString& VertexShaderSource, const TMap<FString, TScriptInterface<ICompushadyBindable>>& VSResourceMap, const FString& PixelShaderSource, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, FString& ErrorMessages, const FCompushadyBlendableRasterizerConfig& BlendableRasterizerConfig, const int32 NumVertices, const int32 NumInstances, const FString& VertexShaderEntryPoint, const FString& PixelShaderEntryPoint, const ECompushadyPostProcessLocation PostProcessLocation)
{
	TArray<uint8> VertexShaderCode;
	Compushady::StringToShaderCode(VertexShaderSource, VertexShaderCode);

	TArray<uint8> PixelShaderCode;
	Compushady::StringToShaderCode(PixelShaderSource, PixelShaderCode);

	UCompushadyBlendable* CompushadyBlendable = NewObject<UCompushadyBlendable>();

	if (!CompushadyBlendable->InitFromHLSLAdvanced(VertexShaderCode, VertexShaderEntryPoint, PixelShaderCode, PixelShaderEntryPoint, PostProcessLocation, ErrorMessages))
	{
		return nullptr;
	}

	if (!CompushadyBlendable->UpdateResourcesByMapAdvanced(VSResourceMap, PSResourceMap, NumVertices, NumInstances, BlendableRasterizerConfig, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyBlendable;
}

UCompushadyBlendable* UCompushadyBlitterFunctionLibrary::CreateCompushadyComputeBlendableByMapFromHLSLString(const FString& ShaderSource, const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, FString& ErrorMessages, const FCompushadyBlendableMatricesConfig& BlendableMatricesConfig, const FIntVector& XYZ, const FString& ShaderEntryPoint, const FString& TargetProfile, const ECompushadyPostProcessLocation PostProcessLocation)
{
	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode(ShaderSource, ShaderCode);

	UCompushadyBlendable* CompushadyBlendable = NewObject<UCompushadyBlendable>();

	if (!CompushadyBlendable->InitFromHLSLCompute(ShaderCode, ShaderEntryPoint, TargetProfile, PostProcessLocation, ErrorMessages))
	{
		return nullptr;
	}

	if (!CompushadyBlendable->UpdateComputeResourcesByMapAdvanced(ResourceMap, XYZ, BlendableMatricesConfig, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyBlendable;
}

UCompushadyBlendable* UCompushadyBlitterFunctionLibrary::CreateCompushadyComputeBlendableByMapFromHLSLFile(const FString& Filename, const TMap<FString, TScriptInterface<ICompushadyBindable>>& ResourceMap, FString& ErrorMessages, const FCompushadyBlendableMatricesConfig& BlendableMatricesConfig, const FIntVector& XYZ, const FString& ShaderEntryPoint, const FString& TargetProfile, const ECompushadyPostProcessLocation PostProcessLocation, const FCompushadyFileLoaderConfig& FileLoaderConfig)
{
	TArray<uint8> ShaderCode;
	if (!UCompushadyFunctionLibrary::LoadFileWithLoaderConfig(Filename, ShaderCode, FileLoaderConfig))
	{
		return nullptr;
	}

	UCompushadyBlendable* CompushadyBlendable = NewObject<UCompushadyBlendable>();

	if (!CompushadyBlendable->InitFromHLSLCompute(ShaderCode, ShaderEntryPoint, TargetProfile, PostProcessLocation, ErrorMessages))
	{
		return nullptr;
	}

	if (!CompushadyBlendable->UpdateComputeResourcesByMapAdvanced(ResourceMap, XYZ, BlendableMatricesConfig, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyBlendable;
}

UCompushadyBlendable* UCompushadyBlitterFunctionLibrary::CreateCompushadyAdvancedBlendableByMapFromHLSLFile(const FString& VSFilename, const TMap<FString, TScriptInterface<ICompushadyBindable>>& VSResourceMap, const FString& PSFilename, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, FString& ErrorMessages, const FCompushadyBlendableRasterizerConfig& BlendableRasterizerConfig, const int32 NumVertices, const int32 NumInstances, const FString& VertexShaderEntryPoint, const FString& PixelShaderEntryPoint, const ECompushadyPostProcessLocation PostProcessLocation, const FCompushadyFileLoaderConfig& VSFileLoaderConfig, const FCompushadyFileLoaderConfig& PSFileLoaderConfig)
{
	TArray<uint8> VSShaderCode;
	if (!UCompushadyFunctionLibrary::LoadFileWithLoaderConfig(VSFilename, VSShaderCode, VSFileLoaderConfig))
	{
		return nullptr;
	}

	TArray<uint8> PSShaderCode;
	if (!UCompushadyFunctionLibrary::LoadFileWithLoaderConfig(PSFilename, PSShaderCode, PSFileLoaderConfig))
	{
		return nullptr;
	}

	UCompushadyBlendable* CompushadyBlendable = NewObject<UCompushadyBlendable>();

	if (!CompushadyBlendable->InitFromHLSLAdvanced(VSShaderCode, VertexShaderEntryPoint, PSShaderCode, PixelShaderEntryPoint, PostProcessLocation, ErrorMessages))
	{
		return nullptr;
	}

	if (!CompushadyBlendable->UpdateResourcesByMapAdvanced(VSResourceMap, PSResourceMap, NumVertices, NumInstances, BlendableRasterizerConfig, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyBlendable;
}

UCompushadyBlendable* UCompushadyBlitterFunctionLibrary::CreateCompushadyBlendableFromGLSLString(const FString& PixelShaderSource, const FCompushadyResourceArray& PSResourceArray, FString& ErrorMessages, const FString& PixelShaderEntryPoint, const ECompushadyPostProcessLocation PostProcessLocation)
{
	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode(PixelShaderSource, ShaderCode);

	UCompushadyBlendable* CompushadyBlendable = NewObject<UCompushadyBlendable>();

	if (!CompushadyBlendable->InitFromGLSL(ShaderCode, PixelShaderEntryPoint, PostProcessLocation, ErrorMessages))
	{
		return nullptr;
	}

	if (!CompushadyBlendable->UpdateResources(PSResourceArray, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyBlendable;
}

UCompushadyBlendable* UCompushadyBlitterFunctionLibrary::CreateCompushadyBlendableByMapFromHLSLString(const FString& PixelShaderSource, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, FString& ErrorMessages, const FString& PixelShaderEntryPoint, const ECompushadyPostProcessLocation PostProcessLocation)
{
	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode(PixelShaderSource, ShaderCode);

	UCompushadyBlendable* CompushadyBlendable = NewObject<UCompushadyBlendable>();

	if (!CompushadyBlendable->InitFromHLSL(ShaderCode, PixelShaderEntryPoint, PostProcessLocation, ErrorMessages))
	{
		return nullptr;
	}

	if (!CompushadyBlendable->UpdateResourcesByMap(PSResourceMap, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyBlendable;
}

UCompushadyBlendable* UCompushadyBlitterFunctionLibrary::CreateCompushadyBlendableByMapFromGLSLString(const FString& PixelShaderSource, const TMap<FString, TScriptInterface<ICompushadyBindable>>& PSResourceMap, FString& ErrorMessages, const FString& PixelShaderEntryPoint, const ECompushadyPostProcessLocation PostProcessLocation)
{
	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode(PixelShaderSource, ShaderCode);

	UCompushadyBlendable* CompushadyBlendable = NewObject<UCompushadyBlendable>();

	if (!CompushadyBlendable->InitFromGLSL(ShaderCode, PixelShaderEntryPoint, PostProcessLocation, ErrorMessages))
	{
		return nullptr;
	}

	if (!CompushadyBlendable->UpdateResourcesByMap(PSResourceMap, ErrorMessages))
	{
		return nullptr;
	}

	return CompushadyBlendable;
}
