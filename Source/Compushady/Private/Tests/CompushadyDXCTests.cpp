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
	Compushady::StringToShaderCode("main() {}", ShaderCode);

	const bool bSuccess = Compushady::CompileHLSL(ShaderCode, "", "cs_6_0", ByteCode, Bindings, ThreadGroupSize, ErrorMessages);

	TestFalse(TEXT("bSuccess"), bSuccess);

	TestEqual(TEXT("ErrorMessages"), ErrorMessages, "Empty EntryPoint");

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
#endif
