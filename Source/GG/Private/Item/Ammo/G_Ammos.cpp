// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/Ammo/G_Ammos.h"
#include "Character/G_Character.h"
#include "Component/Comp_Inventory.h"

AG_Ammos::AG_Ammos()
{
	// 设置默认值（可在蓝图中覆盖）
	// 注意：ItemData 应在生成时通过 SetItemData 从数据表加载，此处不设默认值
}

void AG_Ammos::OnUsed_Implementation(APawn* User, const FItemData& InItemData)
{
	Super::OnUsed_Implementation(User, InItemData);

	// 如果希望“使用”弹药直接装填武器，可以在此实现
	// 但通常弹药是换弹时自动消耗，因此这里可留空
	// 若需要，可获取当前武器并调用 Reload 逻辑
	UE_LOG(LogTemp, Log, TEXT("弹药被使用，但当前未定义具体效果"));
}