// Copyright 2024 - Roberto De Ioris.


#include "CompushadyK2Node_MakeHLSLString.h"
#include "CompushadySyntaxHighlighter.h"

const FString& UCompushadyK2Node_MakeHLSLString::GetShaderLanguage() const
{
	static FString ShaderLanguage = "HLSL";
	return ShaderLanguage;
}

TSharedPtr<ITextLayoutMarshaller> UCompushadyK2Node_MakeHLSLString::GetSyntaxHighlighter() const
{
	return FCompushadySyntaxHighlighter::CreateHLSL();
}