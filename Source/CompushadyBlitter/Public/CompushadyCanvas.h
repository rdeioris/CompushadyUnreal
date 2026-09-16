// Copyright 2023-2026 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CompushadyCanvas.generated.h"

class UFont;

// One retained text item on a canvas. Position is normalized 0..1 viewport
// space (multiplied by the view rect at draw time, resolution-independent).
USTRUCT(BlueprintType)
struct FCompushadyCanvasText
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Compushady|Canvas")
	FGuid Handle;

	UPROPERTY(BlueprintReadWrite, Category = "Compushady|Canvas")
	FVector2D Position = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Compushady|Canvas")
	FString Text;

	UPROPERTY(BlueprintReadWrite, Category = "Compushady|Canvas")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(BlueprintReadWrite, Category = "Compushady|Canvas")
	float Scale = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Compushady|Canvas")
	bool bShadow = true;

	// null -> GEngine->GetSmallFont()
	UPROPERTY(BlueprintReadWrite, Category = "Compushady|Canvas")
	TObjectPtr<UFont> Font = nullptr;
};

// A UObject drawing surface: code / Blueprints / Python add primitives; the
// CompushadyBlitter snapshots them each frame and replays them in its
// post-scene canvas pass. v1 = text only (line/rect/tile can follow as extra
// arrays). Register an instance with the blitter subsystem's AddCanvas(); the
// returned handle removes it via RemoveDrawable().
UCLASS(BlueprintType)
class COMPUSHADYBLITTER_API UCompushadyCanvas : public UObject
{
	GENERATED_BODY()

public:
	// BP-friendly factory: creates a new (transient) canvas. Register it with the
	// blitter via the subsystem's AddCanvas() to draw it. Provided because a
	// Blueprint cannot spawn a Construct-Object-from-Class node for this type
	// through the headless graph-authoring API.
	UFUNCTION(BlueprintCallable, Category = "Compushady|Canvas")
	static UCompushadyCanvas* CreateCompushadyCanvas();

	// Adds a text item; returns a handle for UpdateText / RemoveText.
	UFUNCTION(BlueprintCallable, Category = "Compushady|Canvas")
	FGuid AddText(FVector2D Position, const FString& Text,
		FLinearColor Color = FLinearColor::White, float Scale = 1.0f,
		bool bShadow = true, UFont* Font = nullptr);

	// Updates an existing item in place (all fields); returns true if the handle was found.
	UFUNCTION(BlueprintCallable, Category = "Compushady|Canvas")
	bool UpdateText(const FGuid& Handle, FVector2D Position, const FString& Text,
		FLinearColor Color = FLinearColor::White, float Scale = 1.0f,
		bool bShadow = true, UFont* Font = nullptr);

	// Removes a single item by handle; returns true if found.
	UFUNCTION(BlueprintCallable, Category = "Compushady|Canvas")
	bool RemoveText(const FGuid& Handle);

	// Removes every item.
	UFUNCTION(BlueprintCallable, Category = "Compushady|Canvas")
	void Clear();

	// Render snapshot source: the blitter reads this on the game thread each
	// frame and copies it by value to the render thread. Not reflected.
	const TArray<FCompushadyCanvasText>& GetTextItems() const { return TextItems; }

private:
	UPROPERTY()
	TArray<FCompushadyCanvasText> TextItems;
};
