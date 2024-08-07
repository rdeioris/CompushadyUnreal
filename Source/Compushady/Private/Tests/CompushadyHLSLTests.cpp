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


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyHLSLTest_RWTexture2D, "Compushady.HLSL.RWTexture2D", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyHLSLTest_RWTexture2D::RunTest(const FString& Parameters)
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyHLSLTest_RWBuffer, "Compushady.HLSL.RWBuffer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyHLSLTest_RWBuffer::RunTest(const FString& Parameters)
{
	FString ErrorMessages;
	const FString Code = "RWBuffer<uint> Output; [numthreads(1,1,1)] void main(uint3 tid : SV_DispatchThreadID) { Output[tid.x] = tid.x + 0xdead0000; }";
	UCompushadyCompute* Compute = UCompushadyFunctionLibrary::CreateCompushadyComputeFromHLSLString(Code, ErrorMessages, "main");

	UCompushadyUAV* UAV = UCompushadyFunctionLibrary::CreateCompushadyUAVBuffer(TestName, 32, EPixelFormat::PF_R32_UINT);

	FCompushadySignaled Signal;
	Signal.BindUFunction(Compute, TEXT("StoreLastSignal"));
	Compute->DispatchByMap({ {"Output", UAV} }, FIntVector(8, 1, 1), Signal);

	ADD_LATENT_AUTOMATION_COMMAND(FCompushadyWaitCompute(this, Compute, [this, Compute, UAV]()
		{
			TestTrue("Compute->bLastSuccess", Compute->bLastSuccess);

			TArray<uint32> Output;
			Output.AddZeroed(8);

			UAV->MapReadAndExecuteSync([&Output](const void* Data)
				{
					FMemory::Memcpy(Output.GetData(), Data, 8 * sizeof(uint32));
					return true;
				});

			TestEqual(TEXT("Output[0]"), Output[0], 0xdead0000);
			TestEqual(TEXT("Output[1]"), Output[1], 0xdead0001);
			TestEqual(TEXT("Output[2]"), Output[2], 0xdead0002);
			TestEqual(TEXT("Output[3]"), Output[3], 0xdead0003);
			TestEqual(TEXT("Output[4]"), Output[4], 0xdead0004);
			TestEqual(TEXT("Output[5]"), Output[5], 0xdead0005);
			TestEqual(TEXT("Output[6]"), Output[6], 0xdead0006);
			TestEqual(TEXT("Output[7]"), Output[7], 0xdead0007);

		}));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyHLSLTest_RWBufferFloats, "Compushady.HLSL.RWBufferFloats", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyHLSLTest_RWBufferFloats::RunTest(const FString& Parameters)
{
	FString ErrorMessages;
	const FString Code = "RWBuffer<float> Output; [numthreads(1,1,1)] void main(uint3 tid : SV_DispatchThreadID) { Output[tid.x] = tid.x; }";
	UCompushadyCompute* Compute = UCompushadyFunctionLibrary::CreateCompushadyComputeFromHLSLString(Code, ErrorMessages, "main");

	UCompushadyUAV* UAV = UCompushadyFunctionLibrary::CreateCompushadyUAVBuffer(TestName, 32, EPixelFormat::PF_R32_FLOAT);

	Compute->DispatchByMapSync({ {"Output", UAV} }, FIntVector(8, 1, 1), ErrorMessages);

	TArray<float> Output;
	Output.AddZeroed(8);

	UAV->ReadbackBufferToFloatArraySync(0, 8, Output, ErrorMessages);

	TestEqual(TEXT("Output[0]"), Output[0], 0.0f);
	TestEqual(TEXT("Output[1]"), Output[1], 1.0f);
	TestEqual(TEXT("Output[2]"), Output[2], 2.0f);
	TestEqual(TEXT("Output[3]"), Output[3], 3.0f);
	TestEqual(TEXT("Output[4]"), Output[4], 4.0f);
	TestEqual(TEXT("Output[5]"), Output[5], 5.0f);
	TestEqual(TEXT("Output[6]"), Output[6], 6.0f);
	TestEqual(TEXT("Output[7]"), Output[7], 7.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyHLSLTest_RWBufferFloatsOffset, "Compushady.HLSL.RWBufferFloatsOffset", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyHLSLTest_RWBufferFloatsOffset::RunTest(const FString& Parameters)
{
	FString ErrorMessages;
	const FString Code = "RWBuffer<float> Output; [numthreads(1,1,1)] void main(uint3 tid : SV_DispatchThreadID) { Output[tid.x] = tid.x; }";
	UCompushadyCompute* Compute = UCompushadyFunctionLibrary::CreateCompushadyComputeFromHLSLString(Code, ErrorMessages, "main");

	UCompushadyUAV* UAV = UCompushadyFunctionLibrary::CreateCompushadyUAVBuffer(TestName, 32, EPixelFormat::PF_R32_FLOAT);

	Compute->DispatchByMapSync({ {"Output", UAV} }, FIntVector(8, 1, 1), ErrorMessages);

	TArray<float> Output;
	Output.AddZeroed(5);

	UAV->ReadbackBufferToFloatArraySync(12, 5, Output, ErrorMessages);

	TestEqual(TEXT("Output[0]"), Output[0], 3.0f);
	TestEqual(TEXT("Output[1]"), Output[1], 4.0f);
	TestEqual(TEXT("Output[2]"), Output[2], 5.0f);
	TestEqual(TEXT("Output[3]"), Output[3], 6.0f);
	TestEqual(TEXT("Output[4]"), Output[4], 7.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyHLSLTest_RWBufferBytes, "Compushady.HLSL.RWBufferBytes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyHLSLTest_RWBufferBytes::RunTest(const FString& Parameters)
{
	FString ErrorMessages;
	const FString Code = "RWBuffer<uint> Output; [numthreads(1,1,1)] void main(uint3 tid : SV_DispatchThreadID) { Output[tid.x] = tid.x; }";
	UCompushadyCompute* Compute = UCompushadyFunctionLibrary::CreateCompushadyComputeFromHLSLString(Code, ErrorMessages, "main");

	UCompushadyUAV* UAV = UCompushadyFunctionLibrary::CreateCompushadyUAVBuffer(TestName, 8, EPixelFormat::PF_R8_UINT);

	Compute->DispatchByMapSync({ {"Output", UAV} }, FIntVector(8, 1, 1), ErrorMessages);

	TArray<uint8> Output;
	Output.AddZeroed(8);

	UAV->ReadbackBufferToByteArraySync(0, 8, Output, ErrorMessages);

	TestEqual(TEXT("Output[0]"), Output[0], 0);
	TestEqual(TEXT("Output[1]"), Output[1], 1);
	TestEqual(TEXT("Output[2]"), Output[2], 2);
	TestEqual(TEXT("Output[3]"), Output[3], 3);
	TestEqual(TEXT("Output[4]"), Output[4], 4);
	TestEqual(TEXT("Output[5]"), Output[5], 5);
	TestEqual(TEXT("Output[6]"), Output[6], 6);
	TestEqual(TEXT("Output[7]"), Output[7], 7);

	return true;
}

#endif
