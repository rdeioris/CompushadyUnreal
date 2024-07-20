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
	const FString VSCode = "float4 main(const uint vid : SV_VertexID) : SV_Position { if (vid == 0) { return float4(-1, -1, 0, 1); } else if (vid == 1) { return float4(0, 1, 0, 1); } else if (vid == 2) { return float4(1, -1, 0, 1); } return float4(0, 0, 0, 0); }";
	const FString PSCode = "float4 main() : SV_Target0 { return float4(1, 0, 0, 1); }";
	UCompushadyRasterizer* Rasterizer = UCompushadyFunctionLibrary::CreateCompushadyVSPSRasterizerFromHLSLString(VSCode, PSCode, FCompushadyRasterizerConfig(), ErrorMessages, "main", "main");

	UCompushadyRTV* RTV = UCompushadyFunctionLibrary::CreateCompushadyRTVTexture2D(TestName, 8, 8, EPixelFormat::PF_R8G8B8A8, FLinearColor::Black);

	FCompushadySignaled Signal;
	Signal.BindUFunction(Rasterizer, TEXT("StoreLastSignal"));

	Rasterizer->Draw({}, {}, { RTV }, nullptr, 3, 1, FCompushadyRasterizeConfig(), Signal);

	ADD_LATENT_AUTOMATION_COMMAND(FCompushadyWaitRasterizer(this, Rasterizer, [this, Rasterizer, RTV]()
		{
			TestTrue("Rasterizer->bLastSuccess", Rasterizer->bLastSuccess);

			TArray<uint32> Output;
			Output.AddZeroed(8 * 8);

			RTV->MapTextureSliceAndExecuteSync([&Output](const void* Data, const int32 RowPitch)
				{
					CopyTextureData2D(Data, Output.GetData(), 8, EPixelFormat::PF_R8G8B8A8, RowPitch, 8 * sizeof(uint32));
				}, 0);

			TestEqual(TEXT("Output[7 * 8]"), Output[7 * 8], 0xff0000ff);
			TestEqual(TEXT("Output[7 * 8 + 7]"), Output[7 * 8 + 7], 0xff0000ff);
			TestEqual(TEXT("Output[3 * 8 + 4]"), Output[3 * 8 + 4], 0xff0000ff);

		}));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyRasterizerTest_ClearDepthStencil, "Compushady.Rasterizer.ClearDepthStencil", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyRasterizerTest_ClearDepthStencil::RunTest(const FString& Parameters)
{
	FString ErrorMessages;
	const FString VSCode = "float4 main(const uint vid : SV_VertexID) : SV_Position { if (vid == 0) { return float4(-1, -1, 0, 1); } else if (vid == 1) { return float4(0, 1, 0, 1); } else if (vid == 2) { return float4(1, -1, 0, 1); } return float4(0, 0, 0, 0); }";
	const FString PSCode = "float4 main() : SV_Target0 { return float4(1, 0, 0, 1); }";
	UCompushadyRasterizer* Rasterizer = UCompushadyFunctionLibrary::CreateCompushadyVSPSRasterizerFromHLSLString(VSCode, PSCode, FCompushadyRasterizerConfig(), ErrorMessages, "main", "main");

	UCompushadyDSV* DSV = UCompushadyFunctionLibrary::CreateCompushadyDSVTexture2D(TestName, 8, 8, EPixelFormat::PF_DepthStencil, 1, 2);

	FCompushadySignaled Signal;
	Signal.BindUFunction(Rasterizer, TEXT("StoreLastSignal"));

	Rasterizer->Clear({}, DSV, Signal);

	ADD_LATENT_AUTOMATION_COMMAND(FCompushadyWaitRasterizer(this, Rasterizer, [this, Rasterizer, DSV]()
		{
			TestTrue("Rasterizer->bLastSuccess", Rasterizer->bLastSuccess);

			TArray<uint64> Output;
			Output.AddZeroed(8 * 8);

			// depth and stencil are organized in two planes D32, S32, D32, S32, ...
			DSV->MapTextureSliceAndExecuteSync([&Output](const void* Data, const int32 RowPitch)
				{
					CopyTextureData2D(Data, Output.GetData(), 8, EPixelFormat::PF_DepthStencil, RowPitch * 2, 8 * sizeof(uint64));
				}, 0);

			TestEqual(TEXT("Output[0]"), Output[0], 0x23f800000LLU);
			TestEqual(TEXT("Output[32]"), Output[32], 0x23f800000LLU);
			TestEqual(TEXT("Output[63]"), Output[63], 0x23f800000LLU);

		}));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompushadyRasterizerTest_StencilOnly, "Compushady.Rasterizer.StencilOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompushadyRasterizerTest_StencilOnly::RunTest(const FString& Parameters)
{
	FString ErrorMessages;
	const FString VSCode = "float4 main(const uint vid : SV_VertexID) : SV_Position { if (vid == 0) { return float4(-1, -1, 0, 1); } else if (vid == 1) { return float4(0, 1, 0, 1); } else if (vid == 2) { return float4(1, -1, 0, 1); } return float4(0, 0, 0, 0); }";
	const FString PSCode = "float4 main() : SV_Target0 { return float4(1, 0, 0, 1); }";
	UCompushadyRasterizer* Rasterizer = UCompushadyFunctionLibrary::CreateCompushadyVSPSRasterizerFromHLSLString(VSCode, PSCode, FCompushadyRasterizerConfig(), ErrorMessages, "main", "main");

	//UCompushadyRTV* RTV = UCompushadyFunctionLibrary::CreateCompushadyRTVTexture2D(TestName, 8, 8, EPixelFormat::PF_R8G8B8A8, FLinearColor::Black);
	UCompushadyDSV* DSV = UCompushadyFunctionLibrary::CreateCompushadyDSVTexture2D(TestName, 8, 8, EPixelFormat::PF_DepthStencil, 1, 2);

	FCompushadySignaled Signal;
	Signal.BindUFunction(Rasterizer, TEXT("StoreLastSignal"));

	FCompushadyRasterizeConfig RasterizeConfig;
	RasterizeConfig.StencilValue = 0xde;

	Rasterizer->Draw({}, {}, {}, DSV, 3, 1, RasterizeConfig, Signal);

	ADD_LATENT_AUTOMATION_COMMAND(FCompushadyWaitRasterizer(this, Rasterizer, [this, Rasterizer, DSV]()
		{
			TestTrue("Rasterizer->bLastSuccess", Rasterizer->bLastSuccess);

			TArray<uint64> Output;
			Output.AddZeroed(8 * 8);

			// depth and stencil are organized in two planes D32, S32, D32, S32, ...
			DSV->MapTextureSliceAndExecuteSync([&Output](const void* Data, const int32 RowPitch)
				{
					CopyTextureData2D(Data, Output.GetData(), 8, EPixelFormat::PF_DepthStencil, RowPitch * 2, 8 * sizeof(uint64));
				}, 0);

			TestEqual(TEXT("Output[56]"), (Output[56] & 0xff00000000LLU) >> 32, 0xdeLLU);
			TestEqual(TEXT("Output[63]"), (Output[63] & 0xff00000000LLU) >> 32, 0xdeLLU);

		}));

	return true;
}

#endif
