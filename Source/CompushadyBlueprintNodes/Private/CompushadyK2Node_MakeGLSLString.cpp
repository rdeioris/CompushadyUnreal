// Copyright 2024 - Roberto De Ioris.


#include "CompushadyK2Node_MakeGLSLString.h"
#include "CompushadySyntaxHighlighter.h"

const FString& UCompushadyK2Node_MakeGLSLString::GetShaderLanguage() const
{
	static FString ShaderLanguage = "GLSL";
	return ShaderLanguage;
}

TSharedPtr<ITextLayoutMarshaller> UCompushadyK2Node_MakeGLSLString::GetSyntaxHighlighter() const
{
	return FCompushadySyntaxHighlighter::CreateGLSL();
}