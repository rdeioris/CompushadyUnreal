// Copyright 2023-2024 - Roberto De Ioris.


#include "CompushadySyntaxHighlighter.h"
#include "Runtime/Slate/Public/Framework/Text/IRun.h"
#include "Runtime/Slate/Public/Framework/Text/TextLayout.h"
#include "Runtime/Slate/Public/Framework/Text/SlateTextRun.h"

#define ADD_RULE(rule) TokenizerRules.Add(FSyntaxTokenizer::FRule(TEXT(rule)))

TSharedRef<FCompushadySyntaxHighlighter> FCompushadySyntaxHighlighter::CreateWGSL()
{
	TArray<FSyntaxTokenizer::FRule> TokenizerRules;

	// https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-appendix-keywords

	// basic types
	ADD_RULE("float");
	ADD_RULE("float1");
	ADD_RULE("float2");
	ADD_RULE("float3");
	ADD_RULE("float4");
	ADD_RULE("float1x1");
	ADD_RULE("float1x2");
	ADD_RULE("float1x3");
	ADD_RULE("float1x4");
	ADD_RULE("float2x1");
	ADD_RULE("float2x2");
	ADD_RULE("float2x3");
	ADD_RULE("float2x4");
	ADD_RULE("float3x1");
	ADD_RULE("float3x2");
	ADD_RULE("float3x3");
	ADD_RULE("float3x4");
	ADD_RULE("float4x1");
	ADD_RULE("float4x2");
	ADD_RULE("float4x3");
	ADD_RULE("float4x4");
	ADD_RULE("half");
	ADD_RULE("int");
	ADD_RULE("int2");
	ADD_RULE("int3");
	ADD_RULE("int4");
	ADD_RULE("matrix");
	ADD_RULE("min16float");
	ADD_RULE("min10float");
	ADD_RULE("min16int");
	ADD_RULE("min12int");
	ADD_RULE("min16uint");
	ADD_RULE("uint");
	ADD_RULE("uint2");
	ADD_RULE("uint3");
	ADD_RULE("uint4");
	ADD_RULE("void");

	// templated types
	ADD_RULE("AppendStructuredBuffer");
	ADD_RULE("Buffer");
	ADD_RULE("ByteAddressBuffer");
	ADD_RULE("AppendStructuredBuffer");
	ADD_RULE("ConsumeStructuredBuffer");
	ADD_RULE("RWBuffer");
	ADD_RULE("RWByteAddressBuffer");
	ADD_RULE("RWStructuredBuffer");
	ADD_RULE("RWTexture1D");
	ADD_RULE("RWTexture1DArray");
	ADD_RULE("RWTexture2D");
	ADD_RULE("RWTexture2DArray");
	ADD_RULE("RWTexture3D");
	ADD_RULE("SamplerState");
	ADD_RULE("SamplerComparisonState");
	ADD_RULE("StructuredBuffer");
	ADD_RULE("Texture1D");
	ADD_RULE("Texture1DArray");
	ADD_RULE("Texture2D");
	ADD_RULE("Texture2DArray");
	ADD_RULE("Texture2DMS");
	ADD_RULE("Texture2DMSArray");
	ADD_RULE("Texture3D");
	ADD_RULE("TextureCube");
	ADD_RULE("TextureCubeArray");

	//keywords
	ADD_RULE("return");

	// preprocessor
	ADD_RULE("#define");

	// comments
	ADD_RULE("/*");
	ADD_RULE("*/");
	ADD_RULE("//");

	// strings
	ADD_RULE("\"");

	TokenizerRules.Sort([](const FSyntaxTokenizer::FRule& A, const FSyntaxTokenizer::FRule& B)
		{
			return A.MatchText.Len() > B.MatchText.Len();
		});


	return MakeShared<FCompushadySyntaxHighlighter>(FSyntaxTokenizer::Create(TokenizerRules));
}