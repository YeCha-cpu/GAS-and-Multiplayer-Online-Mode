// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/Weapon/G_Weapons.h"

#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Character/G_Character.h"
#include "Component/Comp_Inventory.h"
#include "Character/G_Character.h"

AG_Weapons::AG_Weapons()
{
	// 初始化当前弹药为弹夹容量（若有编辑器默认值）
	CurrentAmmo = MagazineCapacity;
	bIsReloading = false;
}

void AG_Weapons::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(AG_Weapons, WeaponDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AG_Weapons, MagazineCapacity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AG_Weapons, CurrentAmmo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AG_Weapons, bIsReloading, COND_None, REPNOTIFY_Always);
}

// 弹药复制回调
#include "Character/G_Character.h"

void AG_Weapons::OnRep_CurrentAmmo()
{
	// 触发蓝图事件（已有）
	OnAmmoChanged(CurrentAmmo);
	// 通知角色更新玩家主界面UI
	if (AG_Character* Char = Cast<AG_Character>(GetOwner()))
	{
		Char->UpdateWeaponUI();
	}
}

// 客户端请求换弹
// G_Weapons.cpp
void AG_Weapons::Reload()
{
	// ★ 增加本地预检查，避免无效 RPC
	if (bIsReloading || IsFullAmmo())
		return;

	// 检查背包是否有弹药（仅当有 AmmoItemID 时）
	AG_Character* Char = Cast<AG_Character>(GetOwner());
	if (Char && AmmoItemID != NAME_None)
	{
		UComp_Inventory* Inventory = Char->FindComponentByClass<UComp_Inventory>();
		if (Inventory && Inventory->GetItemTotalQuantity(AmmoItemID) <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("换弹失败：背包中没有弹药 %s"), *AmmoItemID.ToString());
			// 可选：播放音效或显示UI提示
			return;
		}
	}

	// 通过检查后再发送RPC
	if (!HasAuthority())
	{
		ServerReload();
	}
	else
	{
		ServerReload_Implementation();
	}
}

// 换弹验证（仅服务器执行）
bool AG_Weapons::ServerReload_Validate()
{
	if (bIsReloading || IsFullAmmo())
		return false;

	// 检查背包是否有弹药（此验证不是必须的，因为 FinishReload 内部也会处理无弹药情况，但可以提前拦截无效 RPC。）
	AG_Character* Char = Cast<AG_Character>(GetOwner());
	if (Char && AmmoItemID != NAME_None)
	{
		UComp_Inventory* Inventory = Char->FindComponentByClass<UComp_Inventory>();
		if (Inventory && Inventory->GetItemTotalQuantity(AmmoItemID) <= 0)
		{
			return false; // 背包无弹药，拒绝换弹
		}
	}
	return true;
}

// 换弹实现（仅服务器执行）
void AG_Weapons::ServerReload_Implementation()
{
	if (!HasAuthority()) return;
	if (bIsReloading || IsFullAmmo()) return;

	bIsReloading = true;

	// 开始定时器，换弹完成后装填
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(ReloadTimerHandle, this, &AG_Weapons::FinishReload, ReloadTime, false);
	}
}

// 换弹完成
void AG_Weapons::FinishReload()
{
	if (!HasAuthority()) return;

	AG_Character* Char = Cast<AG_Character>(GetOwner());
	if (!Char)
	{
		bIsReloading = false;
		return;
	}

	UComp_Inventory* Inventory = Char->FindComponentByClass<UComp_Inventory>();
	if (!Inventory)
	{
		bIsReloading = false;
		return;
	}

	// 如果武器未指定弹药类型，则无法换弹（或直接补满，但最好有定义）
	if (AmmoItemID == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("武器 %s 未设置 AmmoItemID，无法从背包补充弹药"), *GetName());
		bIsReloading = false;
		return;
	}

	// 计算需要多少发才能补满弹夹
	int32 Need = MagazineCapacity - CurrentAmmo;
	if (Need <= 0)
	{
		bIsReloading = false; // 已经是满弹
		return;
	}

	// 查询背包中该弹药数量
	int32 Have = Inventory->GetItemTotalQuantity(AmmoItemID);
	if (Have <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("背包中没有弹药 %s，无法换弹"), *AmmoItemID.ToString());
		bIsReloading = false;
		OnReloadComplete(); // 触发失败事件（可选）
		return;
	}

	// 实际装填数量 = min(需要, 背包拥有)
	int32 Actual = FMath::Min(Need, Have);

	// 从背包扣除 Actual 发弹药
	if (!Inventory->RemoveItemByID(AmmoItemID, Actual))
	{
		// 理论上不会失败，因为我们已经检查了数量，但以防万一
		UE_LOG(LogTemp, Error, TEXT("从背包扣除弹药失败，可能数量不一致"));
		bIsReloading = false;
		return;
	}

	// 增加当前弹药
	CurrentAmmo += Actual;
	bIsReloading = false;

	// 触发蓝图事件和UI更新
	OnReloadComplete();
	OnRep_CurrentAmmo(); // 刷新UI

	UE_LOG(LogTemp, Log, TEXT("换弹完成，装填了 %d 发 %s 弹药，当前弹药: %d"), Actual, *AmmoItemID.ToString(), CurrentAmmo);
	
	// 通知角色更新 UI（备弹变化）
	if (Char)
	{
		Char->UpdateWeaponUI();
	}
}

// 消耗弹药（服务器调用）
bool AG_Weapons::ConsumeAmmo(int32 Amount)
{
	if (!HasAuthority()) return false;
	if (bIsReloading) return false;
	if (CurrentAmmo < Amount) return false;

	CurrentAmmo -= Amount;
	// 通知角色更新UI（服务器本地立即更新）
	if (AG_Character* Char = Cast<AG_Character>(GetOwner()))
	{
		Char->UpdateWeaponUI();
	}
	return true;
}

void AG_Weapons::OnUsed_Implementation(APawn* User, const FItemData& InItemData)
{
	Super::OnUsed_Implementation(User, InItemData);

	if (!HasAuthority()) return;

	AG_Character* Char = Cast<AG_Character>(User);
	if (Char)
	{
		Char->EquipWeapon(InItemData);  // 装备武器
		UE_LOG(LogTemp, Log, TEXT("背包中使用武器：%s"), *InItemData.ItemName);
	}
}
