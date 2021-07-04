// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/DungeonModel.h"
#include "Frameworks/Snap/Lib/Serialization/ConnectionSerialization.h"
#include "Frameworks/Snap/SnapMap/SnapMapGraphSerialization.h"
#include "CustomBoundsDungeonModel.generated.h"

struct FSnapConnectionInstance;
class UGrammarScriptGraph;
class USnapMapDungeonLevelLoadHandler;

UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UCustomBoundsDungeonModel : public UDungeonModel
{
	GENERATED_BODY()

public:
   virtual void Reset() override;
   virtual void GenerateLayoutData(class FDungeonLayoutData& OutLayout) override;
   virtual bool ShouldAutoResetOnBuild() const override { return false; }

   bool SearchModuleInstance(const FGuid& InNodeId, FSnapMapModuleInstanceSerializedData& OutModuleData);

public:
   UPROPERTY()
      TArray<FSnapConnectionInstance> Connections;

   UPROPERTY()
      TArray<FSnapMapModuleInstanceSerializedData> ModuleInstances;

   UPROPERTY()
      UGrammarScriptGraph* MissionGraph;
};
