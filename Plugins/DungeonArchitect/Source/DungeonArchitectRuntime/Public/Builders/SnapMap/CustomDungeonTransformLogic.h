// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Frameworks/ThemeEngine/Rules/DungeonTransformLogic.h"
#include "CustomDungeonTransformLogic.generated.h"


struct FCell;
class UCustomBoundsDungeonModel;
/**
 * 
 */
UCLASS(Blueprintable, HideDropDown)
class DUNGEONARCHITECTRUNTIME_API UCustomDungeonTransformLogic : public UDungeonTransformLogic
{
	GENERATED_BODY()

public:

   UFUNCTION(BlueprintNativeEvent, Category = "Dungeon")
      void GetNodeOffset(UCustomBoundsDungeonModel* Model, FTransform& Offset);
   virtual void GetNodeOffset_Implementation(UCustomBoundsDungeonModel* Model, FTransform& Offset);
	
};
