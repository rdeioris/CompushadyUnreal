// Copyright 2023 - Roberto De Ioris.

#if WITH_DEV_AUTOMATION_TESTS
#include "CompushadyFunctionLibrary.h"
#include "Misc/AutomationTest.h"

class FCompushadyWaitRasterizer : public IAutomationLatentCommand
{
public:
	FCompushadyWaitRasterizer(FAutomationTestBase* InTest, UCompushadyRasterizer* InRasterizer, TFunction<void()> InTestsFunction) : Test(InTest), Rasterizer(InRasterizer), TestsFunction(InTestsFunction)
	{

	}

	bool Update() override
	{
		if (!Rasterizer->IsRunning())
		{
			TestsFunction();
		}
		return !Rasterizer->IsRunning();
	}

private:
	FAutomationTestBase* Test;
	TStrongObjectPtr<UCompushadyRasterizer> Rasterizer;
	TFunction<void()> TestsFunction;
};


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyRasterizerTest_Simple, "Compushady.Rasterizer.Simple", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyRasterizerTest_Simple::RunTest(const FString& Parameters)
{
	FString ErrorMessages;
	const FString VSCode = "float4 main(uint vid : SV_VertexID, out uint c : COLOR) : SV_Position { if (vid == 0) { return float4(-1, -1, 0, 1); } else if (vid == 1) { return float4(0, 1, 0, 1); } else if (vid == 2) { return float4(1, -1, 0, 1); } return float4(0, 0, 0, 0); }";
	const FString PSCode = "float4 main(float2 hello : OPS, uint ops : AAAA, float4 pos : SV_position, float4x4 color : COLOR, uint4 foo : BBB) : SV_Target0 { return float4(1, 0, 0, 1); }";
	UCompushadyRasterizer* Rasterizer = UCompushadyFunctionLibrary::CreateCompushadyVSPSRasterizerFromHLSLString(VSCode, PSCode, ErrorMessages, "main", "main");

	UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMessages);

	UCompushadyRTV* RTV = UCompushadyFunctionLibrary::CreateCompushadyRTVTexture2D(TestName, 8, 8, EPixelFormat::PF_R8G8B8A8);

	FCompushadySignaled Signal;
	Signal.BindUFunction(Rasterizer, TEXT("StoreLastSignal"));
	Rasterizer->Draw({}, {}, { RTV }, 1, Signal);

	ADD_LATENT_AUTOMATION_COMMAND(FCompushadyWaitRasterizer(this, Rasterizer, [this, Rasterizer, RTV]()
		{
			TestTrue("Rasterizer->bLastSuccess", Rasterizer->bLastSuccess);

			TArray<uint32> Output;
			Output.AddZeroed(8 * 8);

			bool bSuccess = RTV->MapTextureSliceAndExecuteSync([&Output](const void* Data, const int32 RowPitch)
				{
					CopyTextureData2D(Data, Output.GetData(), 8, EPixelFormat::PF_R8G8B8A8, RowPitch, 8 * sizeof(uint32));
				}, 0);

			UE_LOG(LogTemp, Error, TEXT("Mapped: %d"), bSuccess);

			for (int32 Index = 0; Index < Output.Num(); Index++)
			{
				UE_LOG(LogTemp, Warning, TEXT("%d 0x%x"), Index, Output[Index]);
			}

			TestEqual(TEXT("Output[0]"), Output[0], 0xdead0000);
			TestEqual(TEXT("Output[1]"), Output[1], 0xdead0001);
			TestEqual(TEXT("Output[2]"), Output[2], 0xdead0002);
			TestEqual(TEXT("Output[3]"), Output[3], 0xdead0003);

		}));

	return true;
}

#endif
