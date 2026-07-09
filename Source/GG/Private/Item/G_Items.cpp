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

// ---------- OnUsed 默认实现（可在蓝图中重写） ----------
void AG_Items::OnUsed_Implementation(APawn* User, const FItemData& InItemData)
{
	// 默认什么都不做，子类或蓝图可重写此事件实现具体效果
	// 使用传入的 InItemData 而不是成员变量（因为可能在 CDO 上调用）
	UE_LOG(LogTemp, Warning, TEXT("AG_Items::OnUsed 未被重写，物品 %s 没有任何效果"), *InItemData.ItemName);
}