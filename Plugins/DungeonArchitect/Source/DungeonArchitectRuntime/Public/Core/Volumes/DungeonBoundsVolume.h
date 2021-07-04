// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/Volumes/DungeonVolume.h"
#include "DungeonBoundsVolume.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API ADungeonBoundsVolume : public ADungeonVolume
{
	GENERATED_BODY()

public:
	ADungeonBoundsVolume(const FObjectInitializer& ObjectInitializer);
	
};
