// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Builders/SnapMap/CustomBoundsDungeonConfig.h"
#include "Core/DungeonBuilder.h"
#include "Core/DungeonModel.h"
#include "Frameworks/LevelStreaming/DungeonLevelStreamingModel.h"
#include "Frameworks/Snap/Lib/Streaming/SnapStreaming.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"

#include "CustomBoundsDungeonBuilder.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(CustomBoundsDungeonBuilderLog, Log, All);

class ADungeon;
class UCustomBoundsDungeonModel;
class UCustomBoundsDungeonConfig;
class USnapConnectionComponent;
class FDungeonSceneProvider;
class ULevelStreamingDynamic;
class ASnapConnectionActor;
class ANavMeshBoundsVolume;
class UDungeonLevelStreamingModel;
class UDungeonStreamingChunk;

namespace SnapLib {
	class FDiagnostics;
}

namespace SnapLib {
	typedef TSharedPtr<struct FModuleNode> FModuleNodePtr;
	typedef TSharedPtr<struct FModuleDoor> FModuleDoorPtr;
}

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UCustomBoundsDungeonBuilder : public UDungeonBuilder
{
	GENERATED_BODY()

public:
	virtual void BuildNonThemedDungeonImpl(UWorld* World, TSharedPtr<FDungeonSceneProvider> SceneProvider) override;
	virtual void DestroyNonThemedDungeonImpl(UWorld* World) override;

	virtual void DrawDebugData(UWorld* InWorld, bool bPersistent = false, float LifeTime = -1.0f) override;
	virtual bool SupportsBackgroundTask() const override { return false; }

	virtual TSubclassOf<UDungeonModel> GetModelClass() override;
	virtual TSubclassOf<UDungeonConfig> GetConfigClass() override;
	virtual TSubclassOf<UDungeonToolData> GetToolDataClass() override;
	virtual TSubclassOf<UDungeonQuery> GetQueryClass() override;

	virtual bool ProcessSpatialConstraint(UDungeonSpatialConstraint* SpatialConstraint, const FTransform& Transform,
		FQuat& OutRotationOffset) override;
	virtual bool SupportsProperty(const FName& PropertyName) const override;
	virtual bool SupportsTheming() const override { return false; }
	virtual TSharedPtr<class FDungeonSceneProvider> CreateSceneProvider(UDungeonConfig* Config, ADungeon* pDungeon,
		UWorld* World) override;
	virtual bool CanBuildDungeon(FString& OutMessage) override;
	virtual bool SupportsLevelStreaming() const override { return true; }

	void SetDiagnostics(TSharedPtr<SnapLib::FDiagnostics> InDiagnostics);

	UFUNCTION(BlueprintCallable, Category = Dungeon)
		void BuildPreviewSnapLayout();

	static void GetSnapConnectionActors(ULevel* ModuleLevel, TArray<ASnapConnectionActor*>& OutConnectionActors);

protected:
	virtual bool PerformSelectionLogic(const TArray<UDungeonSelectorLogic*>& SelectionLogics,
		const FPropSocket& socket) override;
	virtual FTransform PerformTransformLogic(const TArray<UDungeonTransformLogic*>& TransformLogics,
		const FPropSocket& socket) override;

	SnapLib::FModuleNodePtr GenerateModuleNodeGraph(int32 InSeed) const;
	virtual bool IdentifyBuildSucceeded() const override;

protected:
	TWeakObjectPtr<UCustomBoundsDungeonModel> SnapMapModel;
	TWeakObjectPtr<UCustomBoundsDungeonConfig> SnapMapConfig;
	TSharedPtr<class FCustomBoundsStreamingChunkHandler> LevelStreamHandler;

	// Optional diagnostics for the dungeon flow editor. Will not be used in standalone builds
	TSharedPtr<SnapLib::FDiagnostics> Diagnostics;
};

class FCustomBoundsStreamingChunkHandler : public FSnapStreamingChunkHandlerBase {
public:
	FCustomBoundsStreamingChunkHandler(UWorld* InWorld, UCustomBoundsDungeonModel* InSnapMapModel, UDungeonLevelStreamingModel* InLevelStreamingModel);
	virtual TArray<struct FSnapConnectionInstance>* GetConnections() const override;

public:
	TWeakObjectPtr<UCustomBoundsDungeonModel> SnapMapModel;
};