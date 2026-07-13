// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/Comp_Interaction.h"

#include "Component/Comp_Inventory.h"
#include "Item/G_Items.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

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
		return Cast<AG_Items>(HitResult.GetActor());
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

// RPC 实现，执行物品销毁
void UComp_Interaction::ServerDestroyItem_Implementation(AG_Items* Item)
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor && OwnerActor->HasAuthority() && IsValid(Item))
	{
		// 获取物品数据
		FItemData ItemData = Item->GetItemData();

		// 获取控制器（因为组件挂在 Controller 上）
		APlayerController* PC = Cast<APlayerController>(OwnerActor);
		if (PC)
		{
			APawn* ControlledPawn = PC->GetPawn();
			if (ControlledPawn)
			{
				UComp_Inventory* Inventory = ControlledPawn->FindComponentByClass<UComp_Inventory>();
				if (Inventory)
				{
					// 销毁前先添加物品到背包
					Inventory->AddItem(ItemData, 1);
					UE_LOG(LogTemp, Log, TEXT("物品 %s 已添加到背包"), *ItemData.ItemName);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("未找到背包组件"));
				}
			}
		}

		Item->Destroy();
		UE_LOG(LogTemp, Log, TEXT("物品 %s 已在服务器销毁"), *Item->GetName());
	}
}