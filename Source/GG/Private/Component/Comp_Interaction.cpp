// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/Comp_Interaction.h"

#include "Character/G_Character.h"
#include "Component/Comp_Inventory.h"
#include "Item/G_Items.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Item/Ammo/G_AmmunitionBox.h"
#include "Item/Medicines/G_Medicines.h"
#include "Item/Weapon/G_Weapons.h"

UComp_Interaction::UComp_Interaction()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UComp_Interaction::BeginPlay()
{
	Super::BeginPlay();
	CurrentInteractable = nullptr;
	LastInteractable = nullptr;
}

void UComp_Interaction::UpdateInteraction()
{
	AG_Items* HitItem = TraceForInteractable();

	bool bValid = false;
	if (HitItem && HitItem->GetItemData().bCanInteract)
	{
		APlayerController* OwnerPC = Cast<APlayerController>(GetOwner());
		if (OwnerPC)
		{
			APawn* ControlledPawn = OwnerPC->GetPawn();
			if (ControlledPawn)
			{
				float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), HitItem->GetActorLocation());
				if (Distance <= InteractionRange)
				{
					bValid = true;
				}
			}
		}
	}

	if (bValid)
	{
		if (CurrentInteractable != HitItem)
		{
			ClearHighlight();
			CurrentInteractable = HitItem;
			ApplyHighlight(CurrentInteractable);
		}
	}
	else
	{
		ClearHighlight();
		CurrentInteractable = nullptr;
	}
}

void UComp_Interaction::TryInteract()
{
	if (!CurrentInteractable)
		return;

	APlayerController* OwnerPC = Cast<APlayerController>(GetOwner());
	if (!OwnerPC)
	{
		UE_LOG(LogTemp, Warning, TEXT("拾取失败：无法获取控制器"));
		return;
	}

	APawn* ControlledPawn = OwnerPC->GetPawn();
	if (!ControlledPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("拾取失败：无法获取角色"));
		return;
	}

	float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), CurrentInteractable->GetActorLocation());
	if (Distance > InteractionRange)
	{
		UE_LOG(LogTemp, Warning, TEXT("物品太远，无法拾取 (距离: %f)"), Distance);
		return;
	}

	if (CurrentInteractable->GetItemData().bCanPickUp)
	{
		OnItemPickedUp(CurrentInteractable);// 在蓝图实现
		ClearHighlight();
		ServerDestroyItem(CurrentInteractable);
		CurrentInteractable = nullptr;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("当前物品不可拾取"));
	}
}

AG_Items* UComp_Interaction::TraceForInteractable() const
{
	APlayerController* OwnerPC = Cast<APlayerController>(GetOwner());
	if (!OwnerPC)
		return nullptr;

	FHitResult HitResult;
	OwnerPC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	if (HitResult.bBlockingHit)
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor)
		{
			return Cast<AG_Items>(HitActor);
		}
	}
	return nullptr;
}

void UComp_Interaction::ClearHighlight()
{
	if (LastInteractable)
	{
		LastInteractable->SetHighlightMaterial(nullptr);
		LastInteractable = nullptr;
	}
}

void UComp_Interaction::ApplyHighlight(AG_Items* Item)
{
	if (Item && HighlightMaterial)
	{
		Item->SetHighlightMaterial(HighlightMaterial);
		LastInteractable = Item;
	}
}

// 服务器端实现，销毁物品
// Comp_Interaction.cpp
void UComp_Interaction::ServerDestroyItem_Implementation(AG_Items* Item)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !IsValid(Item))
		return;

	APlayerController* PC = Cast<APlayerController>(OwnerActor);
	if (!PC) return;
	APawn* ControlledPawn = PC->GetPawn();
	if (!ControlledPawn) return;
	AG_Character* Char = Cast<AG_Character>(ControlledPawn);
	if (!Char) return;

	FItemData ItemData = Item->GetItemData();
	UComp_Inventory* Inventory = Char->FindComponentByClass<UComp_Inventory>();

	if (Inventory)
	{
		// 尝试将物品添加到背包（数量为1）
		bool bAdded = Inventory->AddItem(ItemData, 1);
		if (bAdded)
		{
			UE_LOG(LogTemp, Log, TEXT("物品 %s 已添加到背包"), *ItemData.ItemName);
			Item->Destroy(); // 拾取物销毁
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("物品 %s 添加背包失败（背包已满），保留物品"), *ItemData.ItemName);
			// 保留物品，不销毁（可由玩家稍后再次拾取）
		}
	}
	else
	{
		// 没有背包组件，直接销毁（安全措施）
		Item->Destroy();
	}
}