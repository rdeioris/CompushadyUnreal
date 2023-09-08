// Copyright 2023 - Roberto De Ioris.

#if WITH_DEV_AUTOMATION_TESTS
#include "CompushadyCBV.h"
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

	const float Value = CBV->GetFloat(0);

	TestEqual(TEXT("Value"), Value, 0.1f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_Int, "Compushady.CBV.Int", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_Int::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	const bool bSuccess = CBV->Initialize(TestName, nullptr, 4);

	CBV->SetInt(0, 0xdeadbeef);

	const int32 Value = CBV->GetInt(0);

	TestEqual(TEXT("Value"), Value, 0xdeadbeef);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_UInt, "Compushady.CBV.UInt", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_UInt::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	const bool bSuccess = CBV->Initialize(TestName, nullptr, 4);

	CBV->SetUInt(0, 100);

	const uint32 Value = static_cast<uint32>(CBV->GetUInt(0));

	TestEqual(TEXT("Value"), Value, 100);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_UIntNegative, "Compushady.CBV.UIntNegative", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_UIntNegative::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	const bool bSuccess = CBV->Initialize(TestName, nullptr, 4);

	CBV->SetUInt(0, -17);

	const uint32 Value = static_cast<uint32>(CBV->GetUInt(0));

	TestEqual(TEXT("Value"), Value, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_Zero, "Compushady.CBV.Zero", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_Zero::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	const bool bSuccess = CBV->Initialize(TestName, nullptr, 4);

	const int32 Value = CBV->GetInt(0);

	TestEqual(TEXT("Value"), Value, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyCBVTest_ZeroOutOfBounds, "Compushady.CBV.ZeroOutOfBounds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyCBVTest_ZeroOutOfBounds::RunTest(const FString& Parameters)
{
	UCompushadyCBV* CBV = NewObject<UCompushadyCBV>();
	const bool bSuccess = CBV->Initialize(TestName, nullptr, 4);

	const int32 Value = CBV->GetInt(64);

	TestEqual(TEXT("Value"), Value, 0);

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
