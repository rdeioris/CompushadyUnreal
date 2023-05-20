// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Slate/Public/Framework/Text/SyntaxHighlighterTextLayoutMarshaller.h"

/**
 * 
 */
class COMPUSHADY_API FCompushadyHLSLSyntaxHighlighter : public FSyntaxHighlighterTextLayoutMarshaller
{
public:
	FCompushadyHLSLSyntaxHighlighter(TSharedPtr<FSyntaxTokenizer> InTokenizer);
	~FCompushadyHLSLSyntaxHighlighter();

	static TSharedRef<FCompushadyHLSLSyntaxHighlighter> Create();

protected:
	virtual void ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FSyntaxTokenizer::FTokenizedLine> TokenizedLines) override;
};
