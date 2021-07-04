//$ Copyright 2015-21, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionActor.h"

#include "Engine/Level.h"
#include "Engine/LevelBounds.h"

namespace SnapModuleDatabaseBuilder {
    class FDefaultModulePolicy {
    public:
        TArray<FBox> CalculateBounds(ULevel* Level) const {
           TArray<FBox > LevelBounds;// = FBox(ForceInit);
            for (AActor* Actor : Level->Actors) {
                if (ALevelBounds* LevelBoundsActor = Cast<ALevelBounds>(Actor)) {
                    LevelBounds.Push(LevelBoundsActor->GetComponentsBoundingBox());
                    break;
                }
            }
            //if (!LevelBounds.IsValid) {
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

    template<typename TConnectionItem>
    class TDefaultConnectionPolicy {
    public:
        static void Build(ASnapConnectionActor* ConnectionActor, TConnectionItem& OutConnection) {
            OutConnection.ConnectionId = ConnectionActor->GetConnectionId();
            OutConnection.Transform = ConnectionActor->GetActorTransform();
            OutConnection.ConnectionInfo = ConnectionActor->ConnectionComponent->ConnectionInfo;
            OutConnection.ConnectionConstraint = ConnectionActor->ConnectionComponent->ConnectionConstraint;
        }
    };
}

template<typename TModuleItem, typename TConnectionItem, typename TModulePolicy, typename TConnectionPolicy>
class TSnapModuleDatabaseBuilder {
public:
    
    static void Build(TArray<TModuleItem>& InModules, UObject* InModuleDB) {
       UE_LOG(LogTemp, Log, TEXT("Builds"));
        for (TModuleItem& Module : InModules) {
            UWorld* World = Module.Level.LoadSynchronous();

            if (World) {
                ULevel* Level = World->PersistentLevel;
                Level->UpdateLevelComponents(false);
                
                TModulePolicy ModulePolicy;
                ModulePolicy.Initialize(Module, Level, InModuleDB);
                
                Module.Connections.Reset();
                for (AActor* Actor : Level->Actors) {
                    // Update the connection actor list
                    ASnapConnectionActor* ConnectionActor = Cast<ASnapConnectionActor>(Actor);
                    if (ConnectionActor && ConnectionActor->ConnectionComponent) {
                        TConnectionItem& Connection = Module.Connections.AddDefaulted_GetRef();
                        TConnectionPolicy::Build(ConnectionActor, Connection);
                    }
                }
                UE_LOG(LogTemp, Log, TEXT("Calculate Bounds"));
                Module.ModuleBounds.Reset();
                Module.ModuleBounds.Append(ModulePolicy.CalculateBounds(Level));
                ModulePolicy.PostProcess(Module, Level);
            }
        }
    }
};

