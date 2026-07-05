// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Comp_Inventory.h"

UComp_Inventory::UComp_Inventory()
{
	PrimaryComponentTick.bCanEverTick = true;


}

void UComp_Inventory::BeginPlay()
{
	Super::BeginPlay();

	
}

void UComp_Inventory::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


}

