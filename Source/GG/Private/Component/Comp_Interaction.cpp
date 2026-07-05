// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/Comp_Interaction.h"
#include "Item/G_Items.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

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
		// 获取控制器和角色
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
			// 如果无法获取角色，则 bValid 保持 false，不进行高亮
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

	// 获取控制器，并确保能获取到角色
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

	// 距离检查
	float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), CurrentInteractable->GetActorLocation());
	if (Distance > InteractionRange)
	{
		UE_LOG(LogTemp, Warning, TEXT("物品太远，无法拾取 (距离: %f)"), Distance);
		return;
	}

	// 检查是否可拾取
	if (CurrentInteractable->GetItemData().bCanPickUp)
	{
		OnItemPickedUp(CurrentInteractable);
		ClearHighlight();
		if (CurrentInteractable)
		{
			CurrentInteractable->Destroy();
			CurrentInteractable = nullptr;
		}
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