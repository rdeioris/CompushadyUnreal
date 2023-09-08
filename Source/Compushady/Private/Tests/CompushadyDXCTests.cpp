// Copyright 2023 - Roberto De Ioris.

#if WITH_DEV_AUTOMATION_TESTS
#include "Compushady.h"
#include "Misc/AutomationTest.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyDXCTest_Empty, "Compushady.DXC.Empty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyDXCTest_Empty::RunTest(const FString& Parameters)
{
	TArray<uint8> ByteCode;
	Compushady::FCompushadyShaderResourceBindings Bindings;
	FIntVector ThreadGroupSize;
	FString ErrorMessages;
	const bool bSuccess = Compushady::CompileHLSL({}, "", "cs_6_0", ByteCode, Bindings, ThreadGroupSize, ErrorMessages);

	TestFalse(TEXT("bSuccess"), bSuccess);

	TestEqual(TEXT("ErrorMessages"), ErrorMessages, "Empty ShaderCode");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyDXCTest_EmptyEntryPoint, "Compushady.DXC.EmptyEntryPoint", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyDXCTest_EmptyEntryPoint::RunTest(const FString& Parameters)
{
	TArray<uint8> ByteCode;
	Compushady::FCompushadyShaderResourceBindings Bindings;
	FIntVector ThreadGroupSize;
	FString ErrorMessages;

	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode("[numthreads(1, 1, 1)] void main() {}", ShaderCode);

	const bool bSuccess = Compushady::CompileHLSL(ShaderCode, "", "cs_6_0", ByteCode, Bindings, ThreadGroupSize, ErrorMessages);

	TestFalse(TEXT("bSuccess"), bSuccess);

	TestEqual(TEXT("ErrorMessages"), ErrorMessages, "Empty EntryPoint");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyDXCTest_NoTarget, "Compushady.DXC.NoTarget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyDXCTest_NoTarget::RunTest(const FString& Parameters)
{
	TArray<uint8> ByteCode;
	Compushady::FCompushadyShaderResourceBindings Bindings;
	FIntVector ThreadGroupSize;
	FString ErrorMessages;

	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode("[numthreads(1, 1, 1)] void main() {}", ShaderCode);

	const bool bSuccess = Compushady::CompileHLSL(ShaderCode, "main", "", ByteCode, Bindings, ThreadGroupSize, ErrorMessages);

	TestFalse(TEXT("bSuccess"), bSuccess);

	TestEqual(TEXT("ErrorMessages"), ErrorMessages, "Empty TargetProfile");

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyDXCTest_SyntaxError, "Compushady.DXC.SyntaxError", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyDXCTest_SyntaxError::RunTest(const FString& Parameters)
{
	TArray<uint8> ByteCode;
	Compushady::FCompushadyShaderResourceBindings Bindings;
	FIntVector ThreadGroupSize;
	FString ErrorMessages;

	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode("bug!", ShaderCode);

	const bool bSuccess = Compushady::CompileHLSL(ShaderCode, "main", "cs_6_0", ByteCode, Bindings, ThreadGroupSize, ErrorMessages);

	TestFalse(TEXT("bSuccess"), bSuccess);

	TestFalse(TEXT("ErrorMessages"), ErrorMessages.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyDXCTest_Nop, "Compushady.DXC.Nop", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyDXCTest_Nop::RunTest(const FString& Parameters)
{
	TArray<uint8> ByteCode;
	Compushady::FCompushadyShaderResourceBindings Bindings;
	FIntVector ThreadGroupSize;
	FString ErrorMessages;

	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode("[numthreads(1, 1, 1)] void main() {}", ShaderCode);

	const bool bSuccess = Compushady::CompileHLSL(ShaderCode, "main", "cs_6_0", ByteCode, Bindings, ThreadGroupSize, ErrorMessages);

	TestTrue(TEXT("bSuccess"), bSuccess);

	TestTrue(TEXT("ErrorMessages"), ErrorMessages.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyDXCTest_ThreadGroupSize, "Compushady.DXC.ThreadGroupSize", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyDXCTest_ThreadGroupSize::RunTest(const FString& Parameters)
{
	TArray<uint8> ByteCode;
	Compushady::FCompushadyShaderResourceBindings Bindings;
	FIntVector ThreadGroupSize;
	FString ErrorMessages;

	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode("[numthreads(2, 4, 8)] void main() {}", ShaderCode);

	const bool bSuccess = Compushady::CompileHLSL(ShaderCode, "main", "cs_6_0", ByteCode, Bindings, ThreadGroupSize, ErrorMessages);

	TestTrue(TEXT("bSuccess"), bSuccess);

	TestEqual(TEXT("ThreadGroupSize.X"), ThreadGroupSize.X, 2);
	TestEqual(TEXT("ThreadGroupSize.Y"), ThreadGroupSize.Y, 4);
	TestEqual(TEXT("ThreadGroupSize.Z"), ThreadGroupSize.Z, 8);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyDXCTest_Bindings, "Compushady.DXC.Bindings", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyDXCTest_Bindings::RunTest(const FString& Parameters)
{
	TArray<uint8> ByteCode;
	Compushady::FCompushadyShaderResourceBindings Bindings;
	FIntVector ThreadGroupSize;
	FString ErrorMessages;

	TArray<uint8> ShaderCode;
	Compushady::StringToShaderCode("float value1; float value2; Buffer<float4> Input0; Texture1D<uint3> Input1; RWTexture1D<uint> Output0; RWBuffer<float> Output1; [numthreads(1, 1, 1)] void main() { float value3 = value1 + value2; Output0[0] = Input1[0].r + value3; Output1[0] = Input0[0].x; }", ShaderCode);

	const bool bSuccess = Compushady::CompileHLSL(ShaderCode, "main", "cs_6_0", ByteCode, Bindings, ThreadGroupSize, ErrorMessages);

	TestEqual(TEXT("Bindings.CBVs.Num()"), Bindings.CBVs.Num(), 1);
	TestEqual(TEXT("Bindings.SRVs.Num()"), Bindings.SRVs.Num(), 2);
	TestEqual(TEXT("Bindings.UAVs.Num()"), Bindings.UAVs.Num(), 2);

	TestEqual(TEXT("Bindings.CBVs[0].Type"), Bindings.CBVs[0].Type, Compushady::ECompushadySharedResourceType::UniformBuffer);
	TestEqual(TEXT("Bindings.SRVs[0].Type"), Bindings.SRVs[0].Type, Compushady::ECompushadySharedResourceType::Buffer);
	TestEqual(TEXT("Bindings.SRVs[1].Type"), Bindings.SRVs[1].Type, Compushady::ECompushadySharedResourceType::Texture);
	TestEqual(TEXT("Bindings.UAVs[0].Type"), Bindings.UAVs[0].Type, Compushady::ECompushadySharedResourceType::Texture);
	TestEqual(TEXT("Bindings.UAVs[1].Type"), Bindings.UAVs[1].Type, Compushady::ECompushadySharedResourceType::Buffer);

	return true;
}


#endif
