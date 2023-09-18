// Copyright 2023 - Roberto De Ioris.

#if WITH_DEV_AUTOMATION_TESTS
#include "CompushadyFunctionLibrary.h"
#include "Misc/AutomationTest.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_Empty, "Compushady.CBV.Empty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_Empty::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	const bool bSuccess = CBV->Initialize(TestName, nullptr, 0);

	TestFalse(TEXT("bSuccess"), bSuccess);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_Unaligned, "Compushady.CBV.Unaligned", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_Unaligned::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	const bool bSuccess = CBV->Initialize(TestName, nullptr, 3);

	const int64 Size = CBV->GetBufferSize();

	TestEqual(TEXT("Size"), Size, 16LL);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_Float, "Compushady.CBV.Float", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_Float::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	const bool bSuccess = CBV->Initialize(TestName, nullptr, 4);

	CBV->SetFloat(0, 0.1);

	float Value = 0;
	CBV->GetFloat(0, Value);

	TestEqual(TEXT("Value"), Value, 0.1f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_Int, "Compushady.CBV.Int", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_Int::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	const bool bSuccess = CBV->Initialize(TestName, nullptr, 4);

	CBV->SetInt(0, 0xdeadbeef);

	int32 Value = 0;
	CBV->GetInt(0, Value);

	TestEqual(TEXT("Value"), Value, 0xdeadbeef);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_UInt, "Compushady.CBV.UInt", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_UInt::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	const bool bSuccess = CBV->Initialize(TestName, nullptr, 4);

	CBV->SetUInt(0, 100U);

	uint32 Value = 0;
	CBV->GetUInt(0, Value);

	TestEqual(TEXT("Value"), Value, 100);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_UIntFromInt64, "Compushady.CBV.UIntFromInt64", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_UIntFromInt64::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	const bool bSuccess = CBV->Initialize(TestName, nullptr, 4);

	CBV->SetUInt(0, 100LL);

	int64 Value = 0;
	CBV->GetUInt(0, Value);

	TestEqual(TEXT("Value"), static_cast<uint32>(Value), 100);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_UIntNegative, "Compushady.CBV.UIntNegative", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_UIntNegative::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	const bool bSuccess = CBV->Initialize(TestName, nullptr, 4);

	CBV->SetUInt(0, static_cast<int64>(-17));

	uint32 Value = 0;
	CBV->GetUInt(0, Value);

	TestEqual(TEXT("Value"), Value, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_Zero, "Compushady.CBV.Zero", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_Zero::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	const bool bSuccess = CBV->Initialize(TestName, nullptr, 4);

	int32 Value = 0;
	CBV->GetInt(0, Value);

	TestEqual(TEXT("Value"), Value, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_ZeroOutOfBounds, "Compushady.CBV.ZeroOutOfBounds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_ZeroOutOfBounds::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	const bool bSuccess = CBV->Initialize(TestName, nullptr, 4);

	int32 Value;
	const bool bGetIntSuccess = CBV->GetInt(64, Value);

	TestFalse(TEXT("bGetIntSuccess"), bGetIntSuccess);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_InitialDirty, "Compushady.CBV.InitialDirty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_InitialDirty::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	bool bSuccess = CBV->Initialize(TestName, nullptr, 4);

	const bool bDirty = CBV->BufferDataIsDirty();

	TestTrue(TEXT("bDirty"), bDirty);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_Dirty, "Compushady.CBV.Dirty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_Dirty::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	bool bSuccess = CBV->Initialize(TestName, nullptr, 4);

	CBV->SetInt(0, 0);

	const bool bDirty = CBV->BufferDataIsDirty();

	TestTrue(TEXT("bDirty"), bDirty);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_Cleaned, "Compushady.CBV.Cleaned", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_Cleaned::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	bool bSuccess = CBV->Initialize(TestName, nullptr, 4);

	CBV->BufferDataClean();

	const bool bDirty = CBV->BufferDataIsDirty();

	TestFalse(TEXT("bDirty"), bDirty);

	return true;
}
#endif
