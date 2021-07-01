//$ Copyright 2015-21, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Assets/SnapMap/ModuleDatabase/SnapMapModuleDBTypeActions.h"

#include "Builders/SnapMap/SnapMapDungeonConfig.h"
#include "Core/Common/ContentBrowserMenuExtensions.h"
#include "DungeonArchitectEditorModule.h"
#include "Frameworks/Snap/SnapMap/SnapMapModuleDatabase.h"
#include "Frameworks/Snap/SnapMap/CustomBoundsModuleDB.h"

#include "AssetToolsModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

//////////////////////////////////////////////////////////////////////////
// FSnapMapModuleDBTypeActions

FText FSnapMapModuleDBTypeActions::GetName() const {
    return LOCTEXT("SnapMapModuleDBTypeActionsName", "Snap Map - Module Database");
}

FColor FSnapMapModuleDBTypeActions::GetTypeColor() const {
    return FColor(64, 128, 255);
}

UClass* FSnapMapModuleDBTypeActions::GetSupportedClass() const {
    return USnapMapModuleDatabase::StaticClass();
}

uint32 FSnapMapModuleDBTypeActions::GetCategories() {
    return IDungeonArchitectEditorModule::Get().GetDungeonAssetCategoryBit();
}

const TArray<FText>& FSnapMapModuleDBTypeActions::GetSubMenus() const {
    static const TArray<FText> SubMenus = {
        FDAContentBrowserSubMenuNames::SnapMap
    };
    return SubMenus;
}

//////////////////////////////////////////////////////////////////////////
// FCustomBoundsModuleDBTypeActions

FText FCustomBoundsModuleDBTypeActions::GetName() const {
   return LOCTEXT("CustomBoundsModuleDBTypeActions", "Custom Bounds - Module Database");
}

FColor FCustomBoundsModuleDBTypeActions::GetTypeColor() const {
   return FColor(64, 128, 255);
}

UClass* FCustomBoundsModuleDBTypeActions::GetSupportedClass() const {
   return UCustomBoundsModuleDB::StaticClass();
}

uint32 FCustomBoundsModuleDBTypeActions::GetCategories() {
   return IDungeonArchitectEditorModule::Get().GetDungeonAssetCategoryBit();
}

const TArray<FText>& FCustomBoundsModuleDBTypeActions::GetSubMenus() const {
   static const TArray<FText> SubMenus = {
       FDAContentBrowserSubMenuNames::SnapMap
   };
   return SubMenus;
}

#undef LOCTEXT_NAMESPACE

