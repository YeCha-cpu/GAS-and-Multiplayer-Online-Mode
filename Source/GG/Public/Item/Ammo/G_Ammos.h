// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/G_Items.h"
#include "G_Ammos.generated.h"

/**
 * 弹药物品类（如步枪子弹、手枪子弹）
 * 继承自 AG_Items，可在背包中堆叠、使用（用于装填武器）
 */
UCLASS()
class GG_API AG_Ammos : public AG_Items
{
	GENERATED_BODY()

public:
	AG_Ammos();

	// 可选：重写 OnUsed，实现“使用弹药”的效果（例如直接装填当前武器）
	// 但通常弹药是通过换弹逻辑自动消耗，而不是手动使用，所以这里可以留空
	virtual void OnUsed_Implementation(APawn* User, const FItemData& InItemData) override;
};