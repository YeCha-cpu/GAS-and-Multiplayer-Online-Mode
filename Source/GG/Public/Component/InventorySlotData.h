// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Comp_Inventory.h"
#include "UObject/NoExportTypes.h"
#include "Item/G_Items.h"          // 包含 FInventorySlot 定义
#include "InventorySlotData.generated.h"

/**
 * 用于 TileView 数据绑定的 UObject 包装类
 * 每个实例对应一个背包槽位
 */
UCLASS(BlueprintType)
class GG_API UInventorySlotData : public UObject
{
	GENERATED_BODY()

public:
	// 槽位数据（包含物品信息）
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	FInventorySlot SlotData;

	// 槽位索引（便于后续操作）
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	int32 SlotIndex = -1;

	UInventorySlotData() {}

	UInventorySlotData(const FInventorySlot& InSlotData, int32 InSlotIndex)
		: SlotData(InSlotData), SlotIndex(InSlotIndex) {}
};