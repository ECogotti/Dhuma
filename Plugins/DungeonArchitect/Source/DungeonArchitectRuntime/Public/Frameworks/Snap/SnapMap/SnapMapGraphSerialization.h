//$ Copyright 2015-21, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Snap/Lib/Serialization/SnapGraphSerializer.h"
#include "SnapMapGraphSerialization.generated.h"

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FSnapMapModuleInstanceSerializedData {
    GENERATED_USTRUCT_BODY()

    UPROPERTY()
    FGuid ModuleInstanceId;

    UPROPERTY()
    FTransform WorldTransform;

    UPROPERTY()
    TSoftObjectPtr<UWorld> Level;

    UPROPERTY()
    FName Category;
    

    UPROPERTY()
    FBox ModuleBounds;
};

typedef TSnapGraphSerializer<FSnapMapModuleInstanceSerializedData> FSnapMapGraphSerializer;

