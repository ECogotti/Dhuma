//$ Copyright 2015-21, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/LevelStreaming/DungeonLevelStreamer.h"

#include "Core/Dungeon.h"
#include "Frameworks/LevelStreaming/DungeonLevelStreamingModel.h"
#include "Frameworks/LevelStreaming/DungeonLevelStreamingNavigation.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY(LogLevelStreamer);

struct FDungeonLevelStreamingStateRequest {
    TWeakObjectPtr<UDungeonStreamingChunk> Chunk;
    bool bRequestVisible;
    bool bRequestLoaded;
};

void FDungeonLevelStreamer::GetPlayerLocations(UWorld* World, TArray<FVector>& OutPlayerLocations) {
    OutPlayerLocations.Reset();
    for (TActorIterator<APlayerController> It(World); It; ++It) {
        APlayerController* PlayerController = *It;
        if (PlayerController) {
            APawn* Pawn = PlayerController->GetPawnOrSpectator();
            if (Pawn) {
                FVector PlayerLocation = Pawn->GetActorLocation();
                OutPlayerLocations.Add(PlayerLocation);
            }
        }
    }
}

void FDungeonLevelStreamer::Process(UWorld* InWorld, const FDungeonLevelStreamingConfig& Config, UDungeonLevelStreamingModel* InModel) {
    if (!InModel || InModel->Chunks.Num() == 0) return;
    InModel->StreamingNavigation->bEnabled = Config.bProcessStreamingNavigation;

    TArray<FDungeonLevelStreamingStateRequest> UpdateRequests;
    TArray<UDungeonStreamingChunk*> ActiveChunks;
    if (Config.bEnabledLevelStreaming) {
        // Start by finding the source streaming points
        TArray<FVector> SourceLocations;
        EDungeonLevelStreamChunkSelection ChunkSelection = Config.InitialLoadLocation;
        if (InModel->HasNotifiedInitialChunkLoadEvent()) {
            // Default to streaming in the chunks closest to the player location if the initial chunks have already been loaded
            ChunkSelection = EDungeonLevelStreamChunkSelection::PlayerLocations;
        }
        InModel->GetStreamingSourceLocations(InWorld, ChunkSelection, SourceLocations);

        // Find the active chunks (near the source streaming points)
        {
            // Find the chunks that contain the source streaming points 
            TArray<TWeakObjectPtr<UDungeonStreamingChunk>> HostChunks;
            for (const FVector& SourceLocation : SourceLocations) {
                UDungeonStreamingChunk* StartChunk = nullptr;
                if (!GetNearestChunk(InModel->Chunks, SourceLocation, StartChunk)) {
                    continue;
                }
                HostChunks.AddUnique(StartChunk);
            }

            // Find the active chunks to stream (they usually are around the source host chunks)
            for (TWeakObjectPtr<UDungeonStreamingChunk> HostChunk : HostChunks) {
                GetVisibleChunks(Config, HostChunk.Get(), InModel->Chunks, ActiveChunks);
            }
        }

        for (UDungeonStreamingChunk* Chunk : InModel->Chunks) {
            const bool bShouldBeVisible = ActiveChunks.Contains(Chunk);
            bool bShouldBeLoaded = Chunk->IsLevelLoaded();

            // Handle Loading Method
            switch(Config.LoadMethod) {
                case EDungeonLevelStreamLoadMethod::LoadEverythingInMemory:
                    bShouldBeLoaded = true;
                    break;

                case EDungeonLevelStreamLoadMethod::LoadOnDemand:
                default:
                    bShouldBeLoaded |= bShouldBeVisible;
                    break;
            }

            
            // Handle Unloading Method
            switch(Config.UnloadMethod) {
                case EDungeonLevelStreamUnloadMethod::UnloadHiddenChunks:
                    if (!bShouldBeVisible) {
                        bShouldBeLoaded = false;
                    }
                    break;
                
                default:
                case EDungeonLevelStreamUnloadMethod::KeepHiddenChunksInMemory:
                    break;
            }
            
            if (Chunk->RequiresStateUpdate(bShouldBeVisible, bShouldBeLoaded)) {
                FDungeonLevelStreamingStateRequest Request;
                Request.Chunk = Chunk;
                Request.bRequestVisible = bShouldBeVisible;
                Request.bRequestLoaded = bShouldBeLoaded;
                UpdateRequests.Add(Request);
            }
        }
        
        // Sort the update requests based on the view location, so the nearby chunks are loaded first
        UpdateRequests.Sort([SourceLocations](const FDungeonLevelStreamingStateRequest& A,
                                            const FDungeonLevelStreamingStateRequest& B) -> bool {
            float BestDistA = MAX_flt;
            float BestDistB = MAX_flt;
            for (const FVector& ViewLocation : SourceLocations) {
               
               //take the min distance of all bounds
               float AMinDist = -1;
               for (auto bound : A.Chunk->Bounds)
               {
                  const float DistA = /*A.Chunk->Bounds*/bound.ComputeSquaredDistanceToPoint(ViewLocation);
                  AMinDist = (AMinDist == -1 || AMinDist > DistA) ? DistA : AMinDist;
               }

               float BMinDist = -1;
               for (auto bound : B.Chunk->Bounds)
               {
                  const float DistB = /*B.Chunk->Bounds*/bound.ComputeSquaredDistanceToPoint(ViewLocation);
                  BMinDist = (BMinDist == -1 || BMinDist > DistB) ? DistB : BMinDist;
               }
                
                BestDistA = FMath::Min(BestDistA, AMinDist);
                BestDistB = FMath::Min(BestDistB, BMinDist);
            }
            return BestDistA < BestDistB;
        });
    }
    else {
        ActiveChunks.Append(InModel->Chunks);
        for (UDungeonStreamingChunk* Chunk : InModel->Chunks) {
            if (Chunk->RequiresStateUpdate(true, true)) {
                FDungeonLevelStreamingStateRequest Request;
                Request.Chunk = Chunk;
                Request.bRequestLoaded = true;
                Request.bRequestVisible = true;
                UpdateRequests.Add(Request);
            }
        }
    }
    
    // Process the update requests (show/hide chunks)
    for (const FDungeonLevelStreamingStateRequest& Request : UpdateRequests) {
        Request.Chunk->SetStreamingLevelState(Request.bRequestVisible, Request.bRequestLoaded);
    }
    
    // Invoke the initial chunks loaded notification, if all the active nodes have been updated for the first time
    if (!InModel->HasNotifiedInitialChunkLoadEvent() && ActiveChunks.Num() > 0) {
        if (ChunksLoadedAndVisible(ActiveChunks)) {
            InModel->NotifyInitialChunksLoaded();
        }
    }
}

bool FDungeonLevelStreamer::ChunksLoadedAndVisible(const TArray<UDungeonStreamingChunk*>& InChunks) {
    for (UDungeonStreamingChunk* const Chunk : InChunks) {
        if (!Chunk->IsLevelLoaded() || !Chunk->IsLevelVisible()) {
            return false;
        }
    }
    return true;
}

/////////////// Level Streamer Visibility Strategy ///////////////
class DUNGEONARCHITECTRUNTIME_API FDAStreamerDepthVisibilityStrategy : public IDungeonLevelStreamerVisibilityStrategy {
public:
    FDAStreamerDepthVisibilityStrategy(int32 InVisibilityRoomDepth) : VisibilityRoomDepth(InVisibilityRoomDepth) {} 
    virtual void GetVisibleChunks(UDungeonStreamingChunk* StartChunk, const TArray<UDungeonStreamingChunk*>& AllChunks, TArray<UDungeonStreamingChunk*>& OutVisibleChunks) const override {
        TSet<UDungeonStreamingChunk*> Visited;
        GetVisibleChunksRecursive(StartChunk, 0, Visited, OutVisibleChunks);
    }

private:
    void GetVisibleChunksRecursive(UDungeonStreamingChunk* Chunk, int32 DepthFromStart,
                               TSet<UDungeonStreamingChunk*>& Visited,
                               TArray<UDungeonStreamingChunk*>& OutVisibleChunks) const {
        if (!Chunk) return;

        Visited.Add(Chunk);

        OutVisibleChunks.Add(Chunk);

        if (DepthFromStart + 1 <= VisibilityRoomDepth) {
            // Traverse the children
            for (UDungeonStreamingChunk* ChildChunk : Chunk->Neighbors) {
                if (!Visited.Contains(ChildChunk)) {
                    GetVisibleChunksRecursive(ChildChunk, DepthFromStart + 1, Visited, OutVisibleChunks);
                }
            }
        }
    }

private:
    int32 VisibilityRoomDepth;
};

class DUNGEONARCHITECTRUNTIME_API FDAStreamerDistanceVisibilityStrategy : public IDungeonLevelStreamerVisibilityStrategy {
public:
    FDAStreamerDistanceVisibilityStrategy(const FVector& InVisibilityDepth) : VisibilityDepth(InVisibilityDepth) {}
    virtual void GetVisibleChunks(UDungeonStreamingChunk* StartChunk, const TArray<UDungeonStreamingChunk*>& AllChunks, TArray<UDungeonStreamingChunk*>& OutVisibleChunks) const override {
        if (!StartChunk) return;

       //Roberta: how to find the center of the startChunk?
        //Create a new big box that include all
        TArray<FVector> bounds;
        for (auto bound : StartChunk->Bounds)
        {
           bounds.Add(bound.GetCenter() + bound.GetExtent());
           bounds.Add(bound.GetCenter() - bound.GetExtent());
        }
        FBox startChunkBound = FBox(bounds.GetData(), bounds.Num());
        
        const FVector Center = /*StartChunk->Bounds*/startChunkBound.GetCenter();
        FVector Extent = /*StartChunk->Bounds*/startChunkBound.GetExtent();
        Extent += VisibilityDepth;

        const FBox VisibilityBounds(Center - Extent, Center + Extent);
        
        //Roberta: if at least one of the bounds intersects the VisibilityBounds then consider this bound visible
        for (UDungeonStreamingChunk* Chunk : AllChunks) {
            if (!Chunk) continue;
            for (auto bound : Chunk->Bounds)
            {
               if (/*Chunk->Bounds.*/bound.Intersect(VisibilityBounds)) {
                  OutVisibleChunks.Add(Chunk);
                  continue; //avoid adding the same chunk multiple times
               }
            }
        }
    }
    
private:
    FVector VisibilityDepth;
};

void FDungeonLevelStreamer::GetVisibleChunks(const FDungeonLevelStreamingConfig& Config, UDungeonStreamingChunk* StartChunk,
            const TArray<UDungeonStreamingChunk*>& AllChunks, TArray<UDungeonStreamingChunk*>& OutVisibleChunks) {
    TSharedPtr<IDungeonLevelStreamerVisibilityStrategy> Strategy;
    if (Config.StreamingStrategy == EDungeonLevelStreamingStrategy::LayoutDepth) {
        Strategy = MakeShareable(new FDAStreamerDepthVisibilityStrategy(Config.VisibilityRoomDepth));
    }
    else if (Config.StreamingStrategy == EDungeonLevelStreamingStrategy::Distance) {
        Strategy = MakeShareable(new FDAStreamerDistanceVisibilityStrategy(Config.VisibilityDistance));
    }

    Strategy->GetVisibleChunks(StartChunk, AllChunks, OutVisibleChunks);
}

bool FDungeonLevelStreamer::GetNearestChunk(const TArray<UDungeonStreamingChunk*>& Chunks, const FVector& ViewLocation,
                                            UDungeonStreamingChunk*& OutNearestChunk) {
    float NearestDistanceSq = MAX_int32;

    for (UDungeonStreamingChunk* Chunk : Chunks) {
       //the chunk is inside if at least one bound is inside
       for (auto bound : Chunk->Bounds)
       {
          if (/*Chunk->Bounds*/bound.IsInside(ViewLocation)) {
             OutNearestChunk = Chunk;
             return true;
          }
       }
       //the distance should be the min distance of all bounds
       float DistanceSq = MAX_int32;
       for (auto bound : Chunk->Bounds)
       {
          float currDistanceSq = /*Chunk->Bounds*/bound.ComputeSquaredDistanceToPoint(ViewLocation);
          DistanceSq = currDistanceSq < DistanceSq ? currDistanceSq : DistanceSq;
       }
        if (DistanceSq < NearestDistanceSq) {
            OutNearestChunk = Chunk;
            NearestDistanceSq = DistanceSq;
        }
    }


    return Chunks.Num() > 0;
}

