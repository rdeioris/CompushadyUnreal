// Copyright 2024 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "CompushadyK2Node_MakeStringBase.h"
#include "CompushadyK2Node_MakeGLSLString.generated.h"

/**
 * 
 */
UCLASS()
class COMPUSHADYBLUEPRINTNODES_API UCompushadyK2Node_MakeGLSLString : public UCompushadyK2Node_MakeStringBase
{
	GENERATED_BODY()

public:

	TSharedPtr<ITextLayoutMarshaller> GetSyntaxHighlighter() const;

	const FString& GetShaderLanguage() const;
};
