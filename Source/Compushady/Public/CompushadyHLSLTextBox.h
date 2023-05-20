// Copyright 2023 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextWidgetTypes.h"
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

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	
	
};
