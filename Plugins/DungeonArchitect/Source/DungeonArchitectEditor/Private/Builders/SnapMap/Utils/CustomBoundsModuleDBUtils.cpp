// Fill out your copyright notice in the Description page of Project Settings.


#include "Builders/SnapMap/Utils/CustomBoundsModuleDBUtils.h"
#include "Frameworks/Snap/SnapModuleDBBuilder.h"
#include "Frameworks/Snap/SnapMap/CustomBoundsModuleDB.h"
#include "Components/BoxComponent.h"



namespace SnapModuleDatabaseBuilder {

   static FBox GetComponentsBoundingBox(ALevelBounds* LevelBoundsActor)
   {
      if (LevelBoundsActor->GetRootComponent() != nullptr)
      {
         UE_LOG(LogTemp, Log, TEXT("GetScaledBoxExtent: %s"), *LevelBoundsActor->BoxComponent->GetScaledBoxExtent().ToString());
         FVector BoundsCenter = LevelBoundsActor->GetRootComponent()->GetComponentLocation();
         //FVector BoundsExtent = RootComponent->GetComponentTransform().GetScale3D() * 0.5f;
         FVector BoundsExtent = LevelBoundsActor->BoxComponent->GetScaledBoxExtent();
         return FBox(BoundsCenter - BoundsExtent,
            BoundsCenter + BoundsExtent);
      }
      else
         return ALevelBounds::CalculateLevelBounds((ULevel*)LevelBoundsActor);
   }


   class FCustomBoundsModulePolicy {
      
   public:
      TArray<FBox> CalculateBounds(ULevel* Level) const {
         TArray<FBox> LevelBounds; // = FBox(ForceInit);
         for (AActor* Actor : Level->Actors) {
            if (ALevelBounds* LevelBoundsActor = Cast<ALevelBounds>(Actor)) {
               //LevelBounds.Push(LevelBoundsActor->GetComponentsBoundingBox());
               LevelBounds.Push(SnapModuleDatabaseBuilder::GetComponentsBoundingBox(LevelBoundsActor));
            }
         }
         if (LevelBounds.Num() == 0) {
            LevelBounds.Push(ALevelBounds::CalculateLevelBounds(Level));
         }
         return LevelBounds;
      }

      template<typename TModuleItem>
      void Initialize(TModuleItem& ModuleItem, const ULevel* Level, const UObject* InModuleDB) {}

      template<typename TModuleItem>
      void PostProcess(TModuleItem& ModuleItem, const ULevel* Level) {}
   };
}


void FCustomBoundsModuleDBUtils::BuildModuleDatabaseCache(UCustomBoundsModuleDB* InDatabase) {
   UE_LOG(LogTemp, Log, TEXT("BuildModuleDatabaseCache custom"));
   if (!InDatabase) return;
   UE_LOG(LogTemp, Log, TEXT("Database valid"));
   typedef TSnapModuleDatabaseBuilder<
      FSnapMapModuleDatabaseItem,
      FSnapMapModuleDatabaseConnectionInfo,
      SnapModuleDatabaseBuilder::FCustomBoundsModulePolicy,
      SnapModuleDatabaseBuilder::TDefaultConnectionPolicy<FSnapMapModuleDatabaseConnectionInfo>
   > FSnapMapDatabaseBuilder;

   FSnapMapDatabaseBuilder::Build(InDatabase->Modules, InDatabase);

   InDatabase->Modify();
}
