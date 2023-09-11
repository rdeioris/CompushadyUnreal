// Copyright 2023 - Roberto De Ioris.

#if WITH_DEV_AUTOMATION_TESTS
#include "CompushadyFunctionLibrary.h"
#include "Misc/AutomationTest.h"

class FCompushadyWaitCompute : public IAutomationLatentCommand
{
public:
	FCompushadyWaitCompute(FAutomationTestBase* InTest, UCompushadyCompute* InCompute, TFunction<void()> InTestsFunction) : Test(InTest), Compute(InCompute), TestsFunction(InTestsFunction)
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
	FAutomationTestBase* Test;
	TStrongObjectPtr<UCompushadyCompute> Compute;
	TFunction<void()> TestsFunction;
};


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyHLSLTest_Texture2D, "Compushady.HLSL.Texture2D", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyHLSLTest_Texture2D::RunTest(const FString& Parameters)
{
	FString ErrorMessages;
	const FString Code = "RWTexture2D<uint> Output; [numthreads(1,1,1)] void main(uint3 tid : SV_DispatchThreadID) { Output[tid.xy] = tid.x + 0xdead0000; }";
	UCompushadyCompute* Compute = UCompushadyFunctionLibrary::CreateCompushadyComputeFromHLSLString(Code, ErrorMessages, "main");

	UCompushadyUAV* UAV = UCompushadyFunctionLibrary::CreateCompushadyUAVTexture2D(TestName, 4, 1, EPixelFormat::PF_R32_UINT);

	FCompushadySignaled Signal;
	Signal.BindUFunction(Compute, TEXT("StoreLastSignal"));
	Compute->DispatchByMap({ {"Output", UAV} }, UAV->GetTextureThreadGroupSize(Compute->GetThreadGroupSize(), false), Signal);

	ADD_LATENT_AUTOMATION_COMMAND(FCompushadyWaitCompute(this, Compute, [this, Compute, UAV]()
		{
			TestTrue("Compute->bLastSuccess", Compute->bLastSuccess);

			TArray<uint32> Output;
			Output.AddZeroed(4);

			UAV->MapTextureSliceAndExecuteSync([&Output](const void* Data, const int32 RowPitch)
				{
					CopyTextureData2D(Data, Output.GetData(), 1, EPixelFormat::PF_R32_UINT, RowPitch, 4 * sizeof(uint32));
				}, 0);

			TestEqual(TEXT("Output[0]"), Output[0], 0xdead0000);
			TestEqual(TEXT("Output[1]"), Output[1], 0xdead0001);
			TestEqual(TEXT("Output[2]"), Output[2], 0xdead0002);
			TestEqual(TEXT("Output[3]"), Output[3], 0xdead0003);

		}));

	return true;
}

#endif
