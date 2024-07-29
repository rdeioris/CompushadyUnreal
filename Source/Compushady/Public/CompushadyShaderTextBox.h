// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextWidgetTypes.h"
#include "Runtime/Slate/Public/Widgets/Input/SMultiLineEditableTextBox.h"
#include "CompushadyTypes.h"
#include "CompushadyShaderTextBox.generated.h"

/**
 * 
 */
UCLASS()
class COMPUSHADY_API UCompushadyShaderTextBox : public UTextLayoutWidget
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif
	virtual void SynchronizeProperties() override;

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Compushady")
	FString GetSource() const;

	UFUNCTION(BlueprintCallable, Category = "Compushady")
	void SetSource(const FString& Source);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	ECompushadyShaderLanguage ShaderLanguage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FTextBlockStyle SyntaxTypeStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FTextBlockStyle SyntaxKeywordStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FTextBlockStyle SyntaxCommentStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FTextBlockStyle SyntaxStringStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compushady")
	FTextBlockStyle SyntaxPreprocessorStyle;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	TSharedPtr<SMultiLineEditableTextBox> SourceWidget;
	
};
