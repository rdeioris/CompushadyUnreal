// Copyright 2026 - Roberto De Ioris.

#if WITH_DEV_AUTOMATION_TESTS
#include "CompushadyFunctionLibrary.h"
#include "Misc/AutomationTest.h"

class FCompushadyWaitProfiledCompute : public IAutomationLatentCommand
{
public:
	FCompushadyWaitProfiledCompute(UCompushadyCompute* InCompute, TFunction<void()> InTestsFunction) : Compute(InCompute), TestsFunction(InTestsFunction)
	{

	}

	bool Update() override
	{
		if (!Compute->IsRunning())
		{
			TestsFunction();
		}
		return !Compute->IsRunning();
	}

private:
	TStrongObjectPtr<UCompushadyCompute> Compute;
	TFunction<void()> TestsFunction;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyProfileTest_DispatchByMap, "Compushady.Profile.DispatchByMap", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyProfileTest_DispatchByMap::RunTest(const FString& Parameters)
{
	FString ErrorMessages;
	const FString Code = "RWBuffer<uint> Output; [numthreads(1,1,1)] void main(uint3 tid : SV_DispatchThreadID) { Output[tid.x] = tid.x + 0xbeef0000; }";
	UCompushadyCompute* Compute = UCompushadyFunctionLibrary::CreateCompushadyComputeFromHLSLString(Code, ErrorMessages, "main");
	TestNotNull(TEXT("Compute"), Compute);
	if (!Compute)
	{
		return false;
	}

	UCompushadyUAV* UAV = UCompushadyFunctionLibrary::CreateCompushadyUAVBuffer(TestName, 32, EPixelFormat::PF_R32_UINT);

	FCompushadySignaledAndProfiled Signal;
	Signal.BindUFunction(Compute, TEXT("StoreLastSignalAndProfile"));
	Compute->DispatchByMapAndProfile({ {"Output", UAV} }, FIntVector(8, 1, 1), Signal);

	ADD_LATENT_AUTOMATION_COMMAND(FCompushadyWaitProfiledCompute(Compute, [this, Compute, UAV]()
		{
			TestTrue(TEXT("Compute->bLastSuccess"), Compute->bLastSuccess);
			TestTrue(TEXT("Compute->LastErrorMessages.IsEmpty()"), Compute->LastErrorMessages.IsEmpty());
			TestTrue(TEXT("Compute->LastMicroseconds >= 0"), Compute->LastMicroseconds >= 0);

			TArray<uint32> Output;
			Output.AddZeroed(8);

			UAV->MapReadAndExecuteSync([&Output](const void* Data)
				{
					FMemory::Memcpy(Output.GetData(), Data, 8 * sizeof(uint32));
					return true;
				});

			for (int32 Index = 0; Index < 8; Index++)
			{
				TestEqual(FString::Printf(TEXT("Output[%d]"), Index), Output[Index], 0xbeef0000 + Index);
			}
		}));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyProfileTest_InvalidThreadGroupCount, "Compushady.Profile.InvalidThreadGroupCount", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyProfileTest_InvalidThreadGroupCount::RunTest(const FString& Parameters)
{
	FString ErrorMessages;
	const FString Code = "RWBuffer<uint> Output; [numthreads(1,1,1)] void main(uint3 tid : SV_DispatchThreadID) { Output[tid.x] = 1; }";
	UCompushadyCompute* Compute = UCompushadyFunctionLibrary::CreateCompushadyComputeFromHLSLString(Code, ErrorMessages, "main");
	TestNotNull(TEXT("Compute"), Compute);
	if (!Compute)
	{
		return false;
	}

	UCompushadyUAV* UAV = UCompushadyFunctionLibrary::CreateCompushadyUAVBuffer(TestName, 32, EPixelFormat::PF_R32_UINT);

	FCompushadySignaledAndProfiled Signal;
	Signal.BindUFunction(Compute, TEXT("StoreLastSignalAndProfile"));
	Compute->bLastSuccess = true;
	Compute->DispatchByMapAndProfile({ {"Output", UAV} }, FIntVector(0, 1, 1), Signal);

	// rejected synchronously, before anything reaches the GPU
	TestFalse(TEXT("Compute->bLastSuccess"), Compute->bLastSuccess);
	TestFalse(TEXT("Compute->IsRunning()"), Compute->IsRunning());
	TestEqual(TEXT("Compute->LastMicroseconds"), Compute->LastMicroseconds, (int64)0);
	TestTrue(TEXT("Compute->LastErrorMessages mentions ThreadGroupCount"), Compute->LastErrorMessages.Contains(TEXT("ThreadGroupCount")));

	return true;
}

#if COMPUSHADY_UE_VERSION >= 59
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadySRVTest_WorldSceneAccelerationStructureUnsupported, "Compushady.SRV.WorldSceneAccelerationStructureUnsupported", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadySRVTest_WorldSceneAccelerationStructureUnsupported::RunTest(const FString& Parameters)
{
#if RHI_RAYTRACING
	AddExpectedError(TEXT("not supported on UE 5.9+"), EAutomationExpectedErrorFlags::Contains, 1);
#endif
	// 5.9 made the ray tracing scene per-view: the call must fail cleanly instead of asserting
	UCompushadySRV* SRV = UCompushadyFunctionLibrary::CreateCompushadySRVFromWorldSceneAccelerationStructure(GetTransientPackage());
	TestNull(TEXT("SRV"), SRV);

	return true;
}
#endif

#endif
