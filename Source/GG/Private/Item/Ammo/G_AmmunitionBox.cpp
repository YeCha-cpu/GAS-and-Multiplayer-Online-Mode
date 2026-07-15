// G_AmmunitionBox.cpp
#include "Item/Ammo/G_AmmunitionBox.h"
#include "Character/G_Character.h"
#include "Component/Comp_Inventory.h"
#include "Engine/DataTable.h"

AG_AmmunitionBox::AG_AmmunitionBox()
{
	// 设置默认弹药模板（在蓝图中可覆盖）
	AmmoItemData.ID = "Ammo_AR";
	AmmoItemData.ItemName = "AR弹药";
	AmmoItemData.bCanStack = true;
	AmmoItemData.ItemMaxStack = 100;
	AmmoItemData.bCanUse = true;
}

// 使用弹药箱
void AG_AmmunitionBox::OnUsed_Implementation(APawn* User, const FItemData& InItemData)
{
	// 调用基类（可选，仅用于日志）
	Super::OnUsed_Implementation(User, InItemData);

	if (!HasAuthority()) return;

	AG_Character* Char = Cast<AG_Character>(User);
	if (!Char)
	{
		UE_LOG(LogTemp, Warning, TEXT("弹药箱：使用者不是 AG_Character"));
		return;
	}

	UComp_Inventory* Inventory = Char->FindComponentByClass<UComp_Inventory>();
	if (!Inventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("弹药箱：角色没有背包组件"));
		return;
	}

	// ★ 从弹药箱的 AmmoItemData 获取弹药 ID
	if (AmmoItemData.ID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("弹药箱：未配置 AmmoItemData，无法补充弹药"));
		return;
	}

	// 从数据表获取弹药的完整数据（确保图标等信息完整）
	UDataTable* DataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/0_/DT/DT_FItemData.DT_FItemData"));
	if (!DataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("弹药箱：无法加载数据表"));
		return;
	}

	FItemData* AmmoRow = DataTable->FindRow<FItemData>(AmmoItemData.ID, TEXT(""));
	if (!AmmoRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("弹药箱：数据表中找不到弹药 ID %s"), *AmmoItemData.ID.ToString());
		return;
	}

	// 获取补充数量（从弹药箱自身的 ItemUseValue）
	int32 Quantity = FMath::Max(1, (int32)InItemData.ItemUseValue);

	// ★ 添加弹药到背包
	bool bSuccess = Inventory->AddItem(*AmmoRow, Quantity);
	if (bSuccess)
	{
		// ★★★ 关键：立即刷新主界面 UI（更新备弹数量）★★★
		Char->UpdateWeaponUI();
		UE_LOG(LogTemp, Log, TEXT("使用弹药箱，补充 %s x%d"), *AmmoRow->ItemName, Quantity);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("弹药箱：补充弹药失败（背包可能已满）"));
	}
}