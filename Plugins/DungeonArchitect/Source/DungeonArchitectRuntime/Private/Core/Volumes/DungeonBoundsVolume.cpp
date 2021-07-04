// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Volumes/DungeonBoundsVolume.h"

#include "Components/BrushComponent.h"
#include "Engine/CollisionProfile.h"


ADungeonBoundsVolume::ADungeonBoundsVolume(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer) {
   UBrushComponent* BrushComp = GetBrushComponent();
   if (BrushComp) {
      BrushComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
      BrushComp->SetGenerateOverlapEvents(false);
   }
}
