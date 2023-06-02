// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextWidgetTypes.h"
#include "Runtime/Slate/Public/Widgets/Input/SMultiLineEditableTextBox.h"
#include "CompushadyHLSLTextBox.generated.h"

/**
 * 
 */
UCLASS()
class COMPUSHADY_API UCompushadyHLSLTextBox : public UTextLayoutWidget
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

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	TSharedPtr<SMultiLineEditableTextBox> SourceWidget;
	
};
