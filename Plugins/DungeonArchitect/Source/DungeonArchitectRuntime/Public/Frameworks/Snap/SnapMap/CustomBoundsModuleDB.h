// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionConstants.h"
#include "Frameworks/Snap/SnapMap/SnapMapModuleDatabase.h"
#include "CustomBoundsModuleDB.generated.h"



//USTRUCT()
//struct DUNGEONARCHITECTRUNTIME_API FSnapMapCustomBoundsModuleDBConnectionInfo {
//   GENERATED_USTRUCT_BODY()
//
//      UPROPERTY(VisibleAnywhere, Category = Module)
//      FGuid ConnectionId;
//
//   UPROPERTY(VisibleAnywhere, Category = Module)
//      FTransform Transform;
//
//   UPROPERTY(VisibleAnywhere, Category = Module)
//      class USnapConnectionInfo* ConnectionInfo;
//
//   UPROPERTY(VisibleAnywhere, Category = Module)
//      ESnapConnectionConstraint ConnectionConstraint;
//};
//
//USTRUCT()
//struct DUNGEONARCHITECTRUNTIME_API FSnapMapCustomBoundsModuleDBItem {
//   GENERATED_USTRUCT_BODY()
//
//      UPROPERTY(EditAnywhere, Category = Module)
//      TSoftObjectPtr<UWorld> Level;
//
//   UPROPERTY(EditAnywhere, Category = Module)
//      FName Category = "Room";
//
//   UPROPERTY(VisibleAnywhere, Category = Module)
//      TArray<FBox> ModuleBounds;   //Roberta
//
//   UPROPERTY(VisibleAnywhere, Category = Module)
//      TArray<FSnapMapModuleDatabaseConnectionInfo> Connections;
//
//   //Roberta
//   UPROPERTY(EditAnywhere, Category = Module)
//      bool IsOriginal = true;
//};
//
//uint32 GetTypeHash(const FSnapMapModuleDatabaseItem& A);

/**
 * 
 */
UCLASS()
class DUNGEONARCHITECTRUNTIME_API UCustomBoundsModuleDB : public UDataAsset
{
	GENERATED_BODY()

public:
		UPROPERTY(EditAnywhere, Category = Module)
		TArray<FSnapMapModuleDatabaseItem> Modules;
	
};
