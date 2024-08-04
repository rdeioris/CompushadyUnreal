// Copyright 2023-2024 - Roberto De Ioris.

#include "Compushady.h"

#if WITH_EDITOR
#include "Interfaces/IPluginManager.h"
#endif

namespace Compushady
{
	namespace KHR
	{
		static void* LibHandle = nullptr;
		static const uint8* (*GLSLToSpirV)(const char* Glsl, const SIZE_T GlslSize, const char* ShaderModel, const char* EntryPoint, const uint32 Flags, SIZE_T* SpirVSize, char** ErrorPtr, SIZE_T* ErrorLen, void* (*Allocator)(const SIZE_T)) = nullptr;
		static const uint8* (*SpirVToHLSL)(const uint8* SpirV, const SIZE_T SpirVSize, const uint32 Flags, SIZE_T* HlslSize, char** EntryPointPtr, SIZE_T* EntryPointLen, char** ErrorPtr, SIZE_T* ErrorLen, void* (*Allocator)(const SIZE_T)) = nullptr;
		static const uint8* (*SpirVDisassemble)(const uint8* SpirV, const SIZE_T SpirVSize, const uint32 Flags, SIZE_T* AssemblySize, char** ErrorPtr, SIZE_T* ErrorLen, void* (*Allocator)(const SIZE_T)) = nullptr;
		static const uint8* (*SpirVToGLSL)(const uint8* SpirV, const SIZE_T SpirVSize, const uint32 Flags, SIZE_T* GlslSize, char** ErrorPtr, SIZE_T* ErrorLen, void* (*Allocator)(const SIZE_T)) = nullptr;
		static void* (*SpirVToMSL)(const uint32* Binary, const SIZE_T WordCount, void* (*Allocator)(SIZE_T), SIZE_T* OutputSize, char** Errors) = nullptr;

		static void* Malloc(const SIZE_T Size)
		{
			return FMemory::Malloc(Size);
		}

		static bool Setup()
		{
			if (!LibHandle)
			{
#if PLATFORM_WINDOWS
#if WITH_EDITOR
				LibHandle = FPlatformProcess::GetDllHandle(*(FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("Compushady"))->GetBaseDir(), TEXT("Binaries/Win64/compushady_khr.dll"))));
#else
				LibHandle = FPlatformProcess::GetDllHandle(*(FPaths::ProjectDir() / TEXT("Binaries/Win64/compushady_khr.dll")));
#endif
#elif PLATFORM_LINUX
#if WITH_EDITOR
				LibHandle = FPlatformProcess::GetDllHandle(*(FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("Compushady"))->GetBaseDir(), TEXT("Binaries/Linux/libcompushady_khr.so"))));
#else
				LibHandle = FPlatformProcess::GetDllHandle(*(FPaths::ProjectDir() / TEXT("Binaries/Linux/libcompushady_khr.so")));
#endif
#elif PLATFORM_ANDROID
				LibHandle = FPlatformProcess::GetDllHandle(TEXT("libcompushady_khr.so"));
#endif
				if (!LibHandle)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to load compushady_khr shared library"));
					return false;
				}
			}

			if (!GLSLToSpirV)
			{
				GLSLToSpirV = reinterpret_cast<const uint8 * (*)(const char*, const SIZE_T, const char*, const char*, const uint32, SIZE_T*, char**, SIZE_T*, void* (*)(const SIZE_T))>(FPlatformProcess::GetDllExport(LibHandle, TEXT("compushady_khr_glsl_to_spv")));

				if (!GLSLToSpirV)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to find compushady_khr_glsl_to_spv symbol in compushady_khr"));
					return false;
				}
			}

			if (!SpirVToHLSL)
			{
				SpirVToHLSL = reinterpret_cast<const uint8 * (*)(const uint8 * SpirV, const SIZE_T SpirVSize, const uint32 Flags, SIZE_T * HlslSize, char** EntryPointPtr, SIZE_T * EntryPointLen, char** ErrorPtr, SIZE_T * ErrorLen, void* (*Allocator)(const SIZE_T))> (FPlatformProcess::GetDllExport(LibHandle, TEXT("compushady_khr_spv_to_hlsl")));

				if (!SpirVToHLSL)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to find compushady_khr_spv_to_hlsl symbol in compushady_khr"));
					return false;
				}
			}

			if (!SpirVDisassemble)
			{
				SpirVDisassemble = reinterpret_cast<const uint8 * (*)(const uint8 * SpirV, const SIZE_T SpirVSize, const uint32 Flags, SIZE_T * AssemblySize, char** ErrorPtr, SIZE_T * ErrorLen, void* (*Allocator)(const SIZE_T))>(FPlatformProcess::GetDllExport(LibHandle, TEXT("compushady_khr_spv_disassemble")));

				if (!SpirVDisassemble)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to find compushady_khr_spv_disassemble symbol in compushady_khr"));
					return false;
				}
			}

			if (!SpirVToGLSL)
			{
				SpirVToGLSL = reinterpret_cast<const uint8 * (*)(const uint8 * SpirV, const SIZE_T SpirVSize, const uint32 Flags, SIZE_T * GlslSize, char** ErrorPtr, SIZE_T * ErrorLen, void* (*Allocator)(const SIZE_T))> (FPlatformProcess::GetDllExport(LibHandle, TEXT("compushady_khr_spv_to_glsl")));

				if (!SpirVToGLSL)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to find compushady_khr_spv_to_glsl symbol in compushady_khr"));
					return false;
				}
			}

			/*

			if (!SpirVToMSL)
			{
				SpirVToMSL = reinterpret_cast<void* (*)(const uint32*, const SIZE_T, void* (*)(SIZE_T), SIZE_T*, char**)>(FPlatformProcess::GetDllExport(LibHandle, TEXT("compushady_khr_spirv_to_msl")));

				if (!SpirVToMSL)
				{
					UE_LOG(LogCompushady, Error, TEXT("Unable to find compushady_khr_spirv_to_msl symbol in libcompushady_khr"));
					return false;
				}
			}*/

			return true;
		}
	}
}

bool Compushady::CompileGLSL(const TArray<uint8>& ShaderCode, const FString& EntryPoint, const FString& TargetProfile, TArray<uint8>& ByteCode, FString& ErrorMessages)
{
	if (!KHR::Setup())
	{
		return false;
	}

	SIZE_T SpvSize = 0;
	char* Errors = nullptr;
	SIZE_T ErrorsLen = 0;

	TArray<uint8> TargetProfileC;
	StringToCString(TargetProfile, TargetProfileC);
	TArray<uint8> EntryPointC;
	StringToCString(EntryPoint, EntryPointC);

	const uint8* Data = KHR::GLSLToSpirV(reinterpret_cast<const char*>(ShaderCode.GetData()), ShaderCode.Num(), reinterpret_cast<const char*>(TargetProfileC.GetData()), reinterpret_cast<const char*>(EntryPointC.GetData()), 0, &SpvSize, &Errors, &ErrorsLen, KHR::Malloc);
	if (!Data)
	{
		if (Errors)
		{
			FUTF8ToTCHAR Converter(reinterpret_cast<UTF8CHAR*>(Errors), ErrorsLen);
			ErrorMessages = FString(Converter.Length(), Converter.Get());
			FMemory::Free(Errors);
		}
		else
		{
			ErrorMessages = "Unable to compile GLSL sources";
		}
		return false;
	}

	ByteCode.Append(reinterpret_cast<const uint8*>(Data), SpvSize);
	FMemory::Free(const_cast<uint8*>(Data));

	return true;
}

bool Compushady::DisassembleSPIRV(const TArray<uint8>& ByteCode, TArray<uint8>& Disassembled, FString& ErrorMessages)
{
	if (!KHR::Setup())
	{
		return false;
	}

	SIZE_T DisassembledSize = 0;
	char* Errors = nullptr;
	SIZE_T ErrorsLen = 0;
	const uint8* Data = KHR::SpirVDisassemble(ByteCode.GetData(), ByteCode.Num(), 0, &DisassembledSize, &Errors, &ErrorsLen, KHR::Malloc);
	if (!Data)
	{
		if (Errors)
		{
			FUTF8ToTCHAR Converter(reinterpret_cast<UTF8CHAR*>(Errors), ErrorsLen);
			ErrorMessages = FString(Converter.Length(), Converter.Get());
			FMemory::Free(Errors);
		}
		else
		{
			ErrorMessages = "Unable to disassemble SPIRV";
		}
		return false;
	}

	Disassembled.Append(Data, DisassembledSize);
	FMemory::Free(const_cast<uint8*>(Data));

	return true;
}

bool Compushady::SPIRVToHLSL(const TArray<uint8>& ByteCode, TArray<uint8>& HLSL, FString& EntryPoint, FString& ErrorMessages)
{
	if (!KHR::Setup())
	{
		return false;
	}

	SIZE_T OutputSize = 0;
	char* EntryPointPtr = nullptr;
	SIZE_T EntryPointLen = 0;
	char* Errors = nullptr;
	SIZE_T ErrorsLen = 0;
	const uint8* Data = KHR::SpirVToHLSL(ByteCode.GetData(), ByteCode.Num(), 0, &OutputSize, &EntryPointPtr, &EntryPointLen, &Errors, &ErrorsLen, KHR::Malloc);
	if (!Data)
	{
		if (Errors)
		{
			FUTF8ToTCHAR Converter(reinterpret_cast<UTF8CHAR*>(Errors), ErrorsLen);
			ErrorMessages = FString(Converter.Length(), Converter.Get());
			FMemory::Free(Errors);
		}
		else
		{
			ErrorMessages = "Unable to convert SPIRV to HLSL";
		}
		return false;
	}

	if (EntryPointPtr)
	{
		FUTF8ToTCHAR Converter(reinterpret_cast<UTF8CHAR*>(EntryPointPtr), EntryPointLen);
		EntryPoint = FString(Converter.Length(), Converter.Get());
		FMemory::Free(EntryPointPtr);
	}

	HLSL.Append(Data, OutputSize);
	FMemory::Free(const_cast<uint8*>(Data));

	return true;
}

bool Compushady::SPIRVToGLSL(const TArray<uint8>& ByteCode, TArray<uint8>& HLSL, FString& ErrorMessages)
{
	if (!KHR::Setup())
	{
		return false;
	}

	SIZE_T OutputSize = 0;
	char* Errors = nullptr;
	SIZE_T ErrorsLen = 0;
	const uint8* Data = KHR::SpirVToGLSL(ByteCode.GetData(), ByteCode.Num(), 0, &OutputSize, &Errors, &ErrorsLen, KHR::Malloc);
	if (!Data)
	{
		if (Errors)
		{
			FUTF8ToTCHAR Converter(reinterpret_cast<UTF8CHAR*>(Errors), ErrorsLen);
			ErrorMessages = FString(Converter.Length(), Converter.Get());
			FMemory::Free(Errors);
		}
		else
		{
			ErrorMessages = "Unable to convert SPIRV to GLSL";
		}
		return false;
	}

	HLSL.Append(Data, OutputSize);
	FMemory::Free(const_cast<uint8*>(Data));

	return true;
}

bool Compushady::SPIRVToMSL(const TArray<uint8>& ByteCode, FString& MSL, FString& ErrorMessages)
{
	if (!KHR::Setup())
	{
		return false;
	}

	SIZE_T OutputSize = 0;
	char* Errors = nullptr;
	void* Data = KHR::SpirVToMSL(reinterpret_cast<const uint32*>(ByteCode.GetData()), ByteCode.Num() / 4, KHR::Malloc, &OutputSize, &Errors);
	if (!Data)
	{
		if (Errors)
		{
			ErrorMessages = UTF8_TO_TCHAR(Errors);
			FMemory::Free(Errors);
		}
		else
		{
			ErrorMessages = "Unable to convert SPIRV to MSL";
		}
		return false;
	}

	MSL = UTF8_TO_TCHAR(Data);
	FMemory::Free(Data);

	return true;
}