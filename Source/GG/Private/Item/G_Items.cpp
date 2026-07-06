// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/G_Items.h"
#include "Components/SkeletalMeshComponent.h"

AG_Items::AG_Items()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// 开启网络复制
	SetNetUpdateFrequency(100.f);
	bReplicates = true;

	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMesh);
	ItemMesh->SetCollisionProfileName(FName("BlockAll"), true);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
	ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
}

void AG_Items::BeginPlay()
{
	Super::BeginPlay();
}

void AG_Items::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AG_Items::SetItemData(const FItemData& Data)
{
	ItemData = Data;
}

void AG_Items::SetHighlightMaterial(UMaterialInterface* Material)
{
	if (ItemMesh)
	{
		ItemMesh->SetOverlayMaterial(Material);
	}
}
