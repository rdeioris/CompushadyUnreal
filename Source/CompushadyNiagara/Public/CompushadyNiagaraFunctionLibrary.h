// Copyright 2023-2026 - Roberto De Ioris.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CompushadyNiagaraFunctionLibrary.generated.h"

class UNiagaraComponent;
class UCompushadySRV;
class UNiagaraDataInterfaceCompushady;

/**
 * Blueprint helpers for binding a Compushady buffer to a Niagara "Compushady Buffer" Data
 * Interface. The engine's UNiagaraFunctionLibrary::GetDataInterface is C++-only (no UFUNCTION),
 * so these wrap it for Blueprint use.
 */
UCLASS()
class COMPUSHADYNIAGARA_API UCompushadyNiagaraFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Binds a Compushady SRV to a Compushady Buffer Data Interface that is exposed as a Niagara
	 * User Parameter on the given component. The buffer is layout-agnostic: how many floats each
	 * particle occupies and where position/color live inside it are decided in the Niagara graph
	 * (via the GetFloat / GetInt / GetPosition / GetColor accessors and their float indices).
	 *
	 * @param ParameterName  The User Parameter name. Either "CompushadyBuffer" or
	 *                       "User.CompushadyBuffer" works - both are tried.
	 * @return true if the Data Interface was found and updated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Compushady|Niagara")
	static bool SetCompushadyBuffer(UNiagaraComponent* NiagaraComponent, FName ParameterName, UCompushadySRV* SourceSRV);

	/** Returns the Compushady Buffer Data Interface instance for a Niagara User Parameter (or null). */
	UFUNCTION(BlueprintCallable, Category = "Compushady|Niagara")
	static UNiagaraDataInterfaceCompushady* GetCompushadyDataInterface(UNiagaraComponent* NiagaraComponent, FName ParameterName);
};
