// Fill out your copyright notice in the Description page of Project Settings.


#include "Builders/SnapMap/CustomDungeonTransformLogic.h"


#include "Builders/SnapMap/CustomBoundsDungeonModel.h"

void UCustomDungeonTransformLogic::GetNodeOffset_Implementation(UCustomBoundsDungeonModel* Model, FTransform& Offset) {
   Offset = FTransform::Identity;
}