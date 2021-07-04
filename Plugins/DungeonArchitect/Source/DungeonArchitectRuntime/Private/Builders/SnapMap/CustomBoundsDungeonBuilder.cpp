// Fill out your copyright notice in the Description page of Project Settings.


#include "Builders/SnapMap/CustomBoundsDungeonBuilder.h"

#include "Builders/SnapMap/SnapMapAsset.h" //needed
#include "Builders/SnapMap/CustomBoundsDungeonModel.h" //ok
#include "Builders/SnapMap/SnapMapDungeonQuery.h"
#include "Builders/SnapMap/SnapMapDungeonSelectorLogic.h"
#include "Builders/SnapMap/SnapMapDungeonToolData.h"
#include "Builders/SnapMap/CustomDungeonTransformLogic.h"
#include "Core/Dungeon.h" //needed
#include "Core/Volumes/DungeonNegationVolume.h"
#include "Core/Volumes/DungeonThemeOverrideVolume.h"
#include "Frameworks/GraphGrammar/GraphGrammar.h"
#include "Frameworks/GraphGrammar/GraphGrammarProcessor.h"
#include "Frameworks/GraphGrammar/Script/GrammarScriptGraph.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionActor.h"
#include "Frameworks/Snap/Lib/Streaming/SnapStreaming.h" //needed
#include "Frameworks/Snap/Lib/Utils/SnapDiagnostics.h"
#include "Frameworks/Snap/SnapMap/SnapMapLibrary.h" //needed
#include "Frameworks/ThemeEngine/SceneProviders/PooledDungeonSceneProvider.h"

#include "DrawDebugHelpers.h"
#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY(CustomBoundsDungeonBuilderLog);

void UCustomBoundsDungeonBuilder::BuildNonThemedDungeonImpl(UWorld* World, TSharedPtr<FDungeonSceneProvider> SceneProvider)
{
   UE_LOG(CustomBoundsDungeonBuilderLog, Error, TEXT("Test: BuildNonThemedDungeonImpl for custom bounds")); //roberta
   SnapMapModel = Cast<UCustomBoundsDungeonModel>(model);
   SnapMapConfig = Cast<UCustomBoundsDungeonConfig>(config);

   if (!World || !SnapMapConfig.IsValid() || !SnapMapModel.IsValid()) {
      UE_LOG(CustomBoundsDungeonBuilderLog, Error, TEXT("Invalid reference passed to the snap builder"));
      return;
   }

   if (!SnapMapConfig->DungeonFlowGraph) {
      UE_LOG(CustomBoundsDungeonBuilderLog, Error, TEXT("Dungeon Flow asset not specified"));
      return;
   }

   if (!SnapMapConfig->ModuleDatabase) {
      UE_LOG(CustomBoundsDungeonBuilderLog, Error, TEXT("Module Database asset is not specified"));
      return;
   }

   if (LevelStreamHandler.IsValid()) {
      LevelStreamHandler->ClearStreamingLevels();
      LevelStreamHandler.Reset();
   }

   SnapMapModel->Reset();
   WorldMarkers.Reset();

   UDungeonLevelStreamingModel* LevelStreamModel = Dungeon ? Dungeon->LevelStreamingModel : nullptr;
   LevelStreamHandler = MakeShareable(new FCustomBoundsStreamingChunkHandler(GetWorld(), SnapMapModel.Get(), LevelStreamModel));
   LevelStreamHandler->ClearStreamingLevels();

   if (Diagnostics.IsValid()) {
      Diagnostics->Clear();
   }

   const int32 MaxTries = SnapMapConfig->bSupportBuildRetries ? FMath::Max(1, SnapMapConfig->NumBuildRetries) : 1;
   int32 NumTries = 0;

   TSet<int32> ProcessedSeeds;
   int32 Seed = SnapMapConfig->Seed;
   SnapLib::FModuleNodePtr GraphRootNode = nullptr;
   while (NumTries < MaxTries && !GraphRootNode.IsValid()) {
      ProcessedSeeds.Add(Seed);
      GraphRootNode = GenerateModuleNodeGraph(Seed);
      NumTries++;

      if (!GraphRootNode.IsValid()) {
         FRandomStream SeedGenerator(Seed);
         Seed = SeedGenerator.FRandRange(0, MAX_int32 - 1);
         while (ProcessedSeeds.Contains(Seed)) {
            Seed++;
         }
      }
   }

   if (!GraphRootNode) {
      UE_LOG(CustomBoundsDungeonBuilderLog, Error, TEXT("Cannot find a valid snap solution"));
      return;
   }

   FSnapMapGraphSerializer::Serialize({ GraphRootNode }, SnapMapModel->ModuleInstances, SnapMapModel->Connections);
   FGuid SpawnRoomId = GraphRootNode->ModuleInstanceId;
   FSnapStreaming::GenerateLevelStreamingModel(World, { GraphRootNode }, Dungeon->LevelStreamingModel, USnapStreamingChunk::StaticClass(),
      [this, &SpawnRoomId](UDungeonStreamingChunk* InChunk, SnapLib::FModuleNodePtr Node) {
         USnapStreamingChunk* Chunk = Cast<USnapStreamingChunk>(InChunk);
         if (Chunk) {
            Chunk->ModuleTransform = Node->WorldTransform;
            if (Node->ModuleInstanceId == SpawnRoomId) {
               Chunk->bSpawnRoomChunk = true;
            }
            if (LevelStreamHandler.IsValid()) {
               LevelStreamHandler->RegisterEvents(Chunk);
            }
         }
      });
}

namespace CustomBounds{
   void PopulateNegationVolumeBounds(ADungeon* InDungeon, TArray<SnapLib::FSnapNegationVolumeState>& OutNegationVolumes) {
      UWorld* World = InDungeon ? InDungeon->GetWorld() : nullptr;
      if (!World) return;

      // Grab the bounds of all the negation volumes
      for (TObjectIterator<ADungeonNegationVolume> NegationVolume; NegationVolume; ++NegationVolume) {
         if (!NegationVolume->IsValidLowLevel() || NegationVolume->IsPendingKill()) {
            continue;
         }
         if (NegationVolume->Dungeon != InDungeon) {
            continue;
         }

         FVector Origin, Extent;
         NegationVolume->GetActorBounds(false, Origin, Extent);

         SnapLib::FSnapNegationVolumeState State;
         State.Bounds = FBox(Origin - Extent, Origin + Extent);
         State.bInverse = NegationVolume->Reversed;

         OutNegationVolumes.Add(State);
      }
   }
}

SnapLib::FModuleNodePtr UCustomBoundsDungeonBuilder::GenerateModuleNodeGraph(int32 InSeed) const {
   UE_LOG(CustomBoundsDungeonBuilderLog, Error, TEXT("Test: GenerateModuleNodeGraph")); //roberta
   UGrammarScriptGraph* MissionGraph = NewObject<UGrammarScriptGraph>(SnapMapModel.Get());
   SnapMapModel->MissionGraph = MissionGraph;

   UGraphGrammar* MissionGrammar = SnapMapConfig->DungeonFlowGraph->MissionGrammar;

   // build the mission graph from the mission grammar rules
   {
      FGraphGrammarProcessor GraphGrammarProcessor;
      GraphGrammarProcessor.Initialize(MissionGraph, MissionGrammar, InSeed);
      GraphGrammarProcessor.Execute(MissionGraph, MissionGrammar);
   }

   SnapLib::FGrowthStaticState StaticState;
   StaticState.Random = random;
   StaticState.BoundsContraction = SnapMapConfig->CollisionTestContraction;
   StaticState.DungeonBaseTransform = Dungeon
      ? FTransform(FRotator::ZeroRotator, Dungeon->GetActorLocation())
      : FTransform::Identity;
   StaticState.StartTimeSecs = FPlatformTime::Seconds();
   StaticState.MaxProcessingTimeSecs = SnapMapConfig->MaxProcessingTimeSecs;
   StaticState.bAllowModuleRotations = SnapMapConfig->bAllowModuleRotations;
   StaticState.Diagnostics = Diagnostics;

   CustomBounds::PopulateNegationVolumeBounds(Dungeon, StaticState.NegationVolumes);

   SnapLib::IModuleDatabasePtr ModDB = MakeShareable(new FCustomBoundsModuleDatabaseImpl(SnapMapConfig->ModuleDatabase));
   SnapLib::FSnapGraphGenerator GraphGenerator(ModDB, StaticState);
   SnapLib::ISnapGraphNodePtr StartNode = MakeShareable(new FSnapGraphGrammarNode(MissionGraph->FindRootNode()));
   return GraphGenerator.Generate(StartNode);
}

bool UCustomBoundsDungeonBuilder::IdentifyBuildSucceeded() const {
   return SnapMapModel.IsValid() && SnapMapModel->ModuleInstances.Num() > 0;
}

void UCustomBoundsDungeonBuilder::BuildPreviewSnapLayout() {
   UE_LOG(CustomBoundsDungeonBuilderLog, Error, TEXT("Test: BuildPreviewSnapLayout")); //roberta
   SnapMapModel = Cast<UCustomBoundsDungeonModel>(model);
   SnapMapConfig = Cast<UCustomBoundsDungeonConfig>(config);

   UWorld* World = Dungeon ? Dungeon->GetWorld() : nullptr;

   if (!World || !SnapMapConfig.IsValid() || !SnapMapModel.IsValid()) {
      UE_LOG(CustomBoundsDungeonBuilderLog, Error, TEXT("Invalid reference passed to the snap builder"));
      return;
   }

   if (!SnapMapConfig->DungeonFlowGraph) {
      UE_LOG(CustomBoundsDungeonBuilderLog, Error, TEXT("Dungeon Flow asset not specified"));
      return;
   }

   const int32 Seed = config->Seed;
   random.Initialize(Seed);
   SnapMapModel->Reset();
   if (LevelStreamHandler.IsValid()) {
      LevelStreamHandler->ClearStreamingLevels();
   }
   WorldMarkers.Reset();

   const SnapLib::FModuleNodePtr GraphRootNode = GenerateModuleNodeGraph(Seed);
   FSnapMapGraphSerializer::Serialize({ GraphRootNode }, SnapMapModel->ModuleInstances, SnapMapModel->Connections);
}


void UCustomBoundsDungeonBuilder::DestroyNonThemedDungeonImpl(UWorld* World) {
   UDungeonBuilder::DestroyNonThemedDungeonImpl(World);

   SnapMapModel = Cast<UCustomBoundsDungeonModel>(model);
   SnapMapConfig = Cast<UCustomBoundsDungeonConfig>(config);

   if (LevelStreamHandler.IsValid()) {
      LevelStreamHandler->ClearStreamingLevels();
      LevelStreamHandler.Reset();
   }

   SnapMapModel->Reset();

   if (Diagnostics.IsValid()) {
      Diagnostics->Clear();
   }
}

void UCustomBoundsDungeonBuilder::GetSnapConnectionActors(ULevel* ModuleLevel,
   TArray<ASnapConnectionActor*>& OutConnectionActors) {
   OutConnectionActors.Reset();
   for (AActor* Actor : ModuleLevel->Actors) {
      if (!Actor) continue;
      if (ASnapConnectionActor* ConnectionActor = Cast<ASnapConnectionActor>(Actor)) {
         OutConnectionActors.Add(ConnectionActor);
      }
   }
}

void UCustomBoundsDungeonBuilder::SetDiagnostics(TSharedPtr<SnapLib::FDiagnostics> InDiagnostics) {
   Diagnostics = InDiagnostics;
}

void UCustomBoundsDungeonBuilder::DrawDebugData(UWorld* InWorld, bool bPersistent /*= false*/, float LifeTime /*= 0*/) {
   TMap<FGuid, TArray<FSnapConnectionInstance>> ConnectionsByModuleId;
   UCustomBoundsDungeonModel* DModel = Cast<UCustomBoundsDungeonModel>(model);
   if (DModel) {
      for (const FSnapConnectionInstance& Connection : DModel->Connections) {
         TArray<FSnapConnectionInstance>& ModuleConnections = ConnectionsByModuleId.FindOrAdd(Connection.ModuleA);
         ModuleConnections.Add(Connection);
      }
   }

   if (Dungeon && Dungeon->LevelStreamingModel) {
      for (UDungeonStreamingChunk* Chunk : Dungeon->LevelStreamingModel->Chunks) {
         if (!Chunk) continue;
         //Choose a random color for each chunk for debug purpose
         FColor color = Chunk->Color;
         // Draw the bounds
         FVector Center, Extent;
         for (auto bound : Chunk->Bounds)
         {
            bound.GetCenterAndExtents(Center, Extent);
            //hunk->Bounds.GetCenterAndExtents(Center, Extent);
            DrawDebugBox(InWorld, Center, Extent, /*Chunk->getRota*/ FQuat::Identity, /*FColor::Red*/ color, false, -1, 0, 10);
         }

         // Draw the connections to the doors
         float AvgZ = 0;
         const FVector CylinderOffset = FVector(0, 0, 10);
         TArray<FSnapConnectionInstance>& ModuleConnections = ConnectionsByModuleId.FindOrAdd(Chunk->ID);
         for (const FSnapConnectionInstance& Connection : ModuleConnections) {
            FVector Location = Connection.WorldTransform.GetLocation();
            DrawDebugCylinder(InWorld, Location, Location + CylinderOffset, 50, 8, FColor::Red, false, -1, 0, 8);
            AvgZ += Location.Z;
         }

         if (ModuleConnections.Num() > 0) {
            AvgZ /= ModuleConnections.Num();
            FVector CenterPoint = Center;
            CenterPoint.Z = AvgZ;

            DrawDebugCylinder(InWorld, CenterPoint, CenterPoint + CylinderOffset, 80, 16, FColor::Green, false, -1,
               0, 4);

            // Draw a point in the center of the room
            for (const FSnapConnectionInstance& Connection : ModuleConnections) {
               FVector Start = CenterPoint + CylinderOffset / 2.0f;
               FVector End = Connection.WorldTransform.GetLocation() + CylinderOffset / 2.0f;
               DrawDebugLine(InWorld, Start, End, FColor::Green, false, -1, 0, 20);
            }

         }
      }
   }
}


TSubclassOf<UDungeonModel> UCustomBoundsDungeonBuilder::GetModelClass() {
   return UCustomBoundsDungeonModel::StaticClass();
}

TSubclassOf<UDungeonConfig> UCustomBoundsDungeonBuilder::GetConfigClass() {
   return UCustomBoundsDungeonConfig::StaticClass();
}

TSubclassOf<UDungeonToolData> UCustomBoundsDungeonBuilder::GetToolDataClass() {
   return USnapMapDungeonToolData::StaticClass();
}

TSubclassOf<UDungeonQuery> UCustomBoundsDungeonBuilder::GetQueryClass() {
   return USnapMapDungeonQuery::StaticClass();
}


bool UCustomBoundsDungeonBuilder::ProcessSpatialConstraint(UDungeonSpatialConstraint* SpatialConstraint,
   const FTransform& Transform, FQuat& OutRotationOffset) {
   return false;
}

bool UCustomBoundsDungeonBuilder::SupportsProperty(const FName& PropertyName) const {
   if (PropertyName == "Themes") return false;
   if (PropertyName == "MarkerEmitters") return false;
   if (PropertyName == "EventListeners") return false;

   return true;
}

TSharedPtr<class FDungeonSceneProvider> UCustomBoundsDungeonBuilder::CreateSceneProvider(
   UDungeonConfig* Config, ADungeon* pDungeon, UWorld* World) {
   return MakeShareable(new FPooledDungeonSceneProvider(pDungeon, World));
}

bool UCustomBoundsDungeonBuilder::CanBuildDungeon(FString& OutMessage) {
   ADungeon* OuterDungeon = Cast<ADungeon>(GetOuter());
   if (OuterDungeon) {
      SnapMapConfig = Cast<UCustomBoundsDungeonConfig>(OuterDungeon->GetConfig());

      if (!SnapMapConfig.IsValid()) {
         OutMessage = "Dungeon not initialized correctly";
         return false;
      }

      if (!SnapMapConfig->DungeonFlowGraph) {
         OutMessage = "Dungeon Flow asset not assigned";
         return false;
      }

      if (!SnapMapConfig->ModuleDatabase) {
         OutMessage = "Module Database asset not assigned";
         return false;
      }
   }
   else {
      OutMessage = "Dungeon not initialized correctly";
      return false;
   }

   return true;
}

bool UCustomBoundsDungeonBuilder::PerformSelectionLogic(const TArray<UDungeonSelectorLogic*>& SelectionLogics,
   const FPropSocket& socket) {
   return false;
}

FTransform UCustomBoundsDungeonBuilder::PerformTransformLogic(const TArray<UDungeonTransformLogic*>& TransformLogics,
   const FPropSocket& socket) {
   return FTransform::Identity;
}

///////////////////////////////////// FSnapMapStreamingChunkHandler /////////////////////////////////////
FCustomBoundsStreamingChunkHandler::FCustomBoundsStreamingChunkHandler(UWorld* InWorld, UCustomBoundsDungeonModel* InSnapMapModel, UDungeonLevelStreamingModel* InLevelStreamingModel)
   : FSnapStreamingChunkHandlerBase(InWorld, InLevelStreamingModel)
   , SnapMapModel(InSnapMapModel)
{
}

TArray<FSnapConnectionInstance>* FCustomBoundsStreamingChunkHandler::GetConnections() const {
   return SnapMapModel.IsValid() ? &SnapMapModel->Connections : nullptr;
}