// Copyright 2024 - Roberto De Ioris.


#include "CompushadyK2Node_MakeSPIRVString.h"
#include "CompushadySyntaxHighlighter.h"

const FString& UCompushadyK2Node_MakeSPIRVString::GetShaderLanguage() const
{
	static FString ShaderLanguage = "SPIRV";
	return ShaderLanguage;
}

TSharedPtr<ITextLayoutMarshaller> UCompushadyK2Node_MakeSPIRVString::GetSyntaxHighlighter() const
{
	return FCompushadySyntaxHighlighter::CreateSPIRV();
}