// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/Comp_Inventory.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Component/InventorySlotData.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "Core/G_PlayerState.h"
#include "GAS/AS_Player.h"
#include "GameplayTagContainer.h"
#include "Character/G_Character.h"

UComp_Inventory::UComp_Inventory()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UComp_Inventory::BeginPlay()
{
    Super::BeginPlay();
    if (Capacity < 1) Capacity = 1;
    Slots.SetNum(Capacity);
}

void UComp_Inventory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION_NOTIFY(UComp_Inventory, Slots, COND_None, REPNOTIFY_Always);
}

// ---------- 背包更新 ----------
void UComp_Inventory::OnRep_Slots()
{
    BroadcastInventoryUpdated();
    // 如果是本地角色，刷新武器UI
    if (AG_Character* Char = Cast<AG_Character>(GetOwner()))
    {
        if (Char->IsLocallyControlled())
        {
            Char->UpdateWeaponUI();
        }
    }
}

// ---------- 添加物品 ----------
bool UComp_Inventory::AddItem(const FItemData& ItemData, int32 Quantity)
{
    if (!GetOwner()->HasAuthority()) return false;
    if (Quantity <= 0 || ItemData.ID.IsNone()) return false;

    int32 Remaining = Quantity;

    if (ItemData.bCanStack)
    {
        for (int32 i = 0; i < Slots.Num(); ++i)
        {
            if (Slots[i].CanStackWith(ItemData))
            {
                int32 MaxStack = ItemData.ItemMaxStack;
                int32 Current = Slots[i].Quantity;
                int32 Space = MaxStack - Current;
                int32 AddAmount = FMath::Min(Remaining, Space);
                if (AddAmount > 0)
                {
                    Slots[i].Quantity += AddAmount;
                    Remaining -= AddAmount;
                    if (Remaining == 0) break;
                }
            }
        }
    }

    if (Remaining > 0)
    {
        for (int32 i = 0; i < Slots.Num(); ++i)
        {
            if (Slots[i].IsEmpty())
            {
                Slots[i] = FInventorySlot(ItemData, Remaining);
                Remaining = 0;
                break;
            }
        }
    }

    if (Remaining > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("背包空间不足，剩余 %d 个未放入"), Remaining);
        return false;
    }

    OnRep_Slots();
    return true;
}

// ---------- 移除物品 ----------
bool UComp_Inventory::RemoveItem(int32 SlotIndex, int32 RemoveQuantity)
{
    if (!GetOwner()->HasAuthority()) return false;
    if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty()) return false;

    FInventorySlot& Slot = Slots[SlotIndex];
    if (Slot.Quantity < RemoveQuantity) return false;

    Slot.Quantity -= RemoveQuantity;
    if (Slot.Quantity == 0)
    {
        Slot = FInventorySlot();
    }
    OnRep_Slots();
    return true;
}

// ---------- 使用物品 ----------
void UComp_Inventory::UseItem(int32 SlotIndex)
{
    if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty()) return;
    if (!GetOwner()->HasAuthority())
    {
        ServerUseItem(SlotIndex);
        return;
    }
    ServerUseItem_Implementation(SlotIndex);
}

void UComp_Inventory::ServerUseItem_Implementation(int32 SlotIndex)
{
    if (!GetOwner()->HasAuthority()) return;
    if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty()) return;

    const FInventorySlot& Slot = Slots[SlotIndex];
    if (Slot.ItemData.bCanUse)
    {
        ApplyItemEffect(Slot.ItemData);
        RemoveItem(SlotIndex, 1);
    }
}

// ---------- 丢弃物品 ----------
void UComp_Inventory::DropItem(int32 SlotIndex, int32 Quantity)
{
    if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty()) return;
    if (!GetOwner()->HasAuthority())
    {
        ServerDropItem(SlotIndex, Quantity);
        return;
    }
    ServerDropItem_Implementation(SlotIndex, Quantity);
}

void UComp_Inventory::ServerDropItem_Implementation(int32 SlotIndex, int32 DropQuantity)
{
    if (!GetOwner()->HasAuthority()) return;
    if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty()) return;

    const FInventorySlot& Slot = Slots[SlotIndex];
    if (DropQuantity > Slot.Quantity) DropQuantity = Slot.Quantity;

    SpawnDroppedItem(Slot.ItemData, DropQuantity);
    RemoveItem(SlotIndex, DropQuantity);
}

void UComp_Inventory::SpawnDroppedItem(const FItemData& ItemData, int32 Quantity)
{
    UE_LOG(LogTemp, Log, TEXT("生成丢弃物品: %s x%d"), *ItemData.ItemName, Quantity);
    // 实际生成逻辑可由蓝图或 C++ 扩展
}

// ---------- ApplyItemEffect 默认实现 ----------
void UComp_Inventory::ApplyItemEffect_Implementation(const FItemData& ItemData)
{
    AG_Character* G_Character = Cast<AG_Character>(GetOwner());
    if (!G_Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyItemEffect: 无法获取拥有者角色"));
        return;
    }

    UClass* ItemClass = ItemData.ItemClass;
    if (!ItemClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyItemEffect: ItemData 中没有指定 ItemClass"));
        return;
    }

    AG_Items* DefaultItem = Cast<AG_Items>(ItemClass->GetDefaultObject());
    if (!DefaultItem)
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyItemEffect: %s 不是 AG_Items 的子类"), *ItemClass->GetName());
        return;
    }

    DefaultItem->OnUsed(G_Character, ItemData);
    UE_LOG(LogTemp, Log, TEXT("ApplyItemEffect: 已通知物品 %s 使用"), *ItemData.ItemName);
}

// ---------- 获取改ID类物品总数 ----------
int32 UComp_Inventory::GetItemTotalQuantity(FName ItemID) const
{
    int32 Total = 0;
    for (const FInventorySlot& Slot : Slots)
    {
        if (Slot.ItemData.ID == ItemID)
            Total += Slot.Quantity;
    }
    return Total;
}

// ---------- 获取槽位数据对象 ----------
TArray<UInventorySlotData*> UComp_Inventory::GetSlotDataObjects()
{
    TArray<UInventorySlotData*> Result;
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        UInventorySlotData* Data = NewObject<UInventorySlotData>(this);
        Data->SlotData = Slots[i];
        Data->SlotIndex = i;
        Result.Add(Data);
    }
    return Result;
}

// ---------- 广播背包更新事件 ----------
void UComp_Inventory::BroadcastInventoryUpdated()
{
    OnInventoryUpdatedDelegate.Broadcast();
    //UE_LOG(LogTemp, Log, TEXT("广播背包更新事件"));
}

// ---------- 移动物品 ----------
bool UComp_Inventory::MoveItem(int32 SourceIndex, int32 TargetIndex)
{
    if (!GetOwner()->HasAuthority()) return false;
    if (!Slots.IsValidIndex(SourceIndex) || !Slots.IsValidIndex(TargetIndex)) return false;
    if (SourceIndex == TargetIndex) return true;

    FInventorySlot& SourceSlot = Slots[SourceIndex];
    FInventorySlot& TargetSlot = Slots[TargetIndex];

    if (SourceSlot.IsEmpty()) return false;

    if (TargetSlot.IsEmpty())
    {
        TargetSlot = SourceSlot;
        SourceSlot = FInventorySlot();
        OnRep_Slots();
        return true;
    }

    if (SourceSlot.CanStackWith(TargetSlot.ItemData))
    {
        int32 MaxStack = SourceSlot.ItemData.ItemMaxStack;
        int32 Space = MaxStack - TargetSlot.Quantity;
        if (Space > 0)
        {
            int32 MoveAmount = FMath::Min(SourceSlot.Quantity, Space);
            TargetSlot.Quantity += MoveAmount;
            SourceSlot.Quantity -= MoveAmount;
            if (SourceSlot.Quantity == 0)
            {
                SourceSlot = FInventorySlot();
            }
            OnRep_Slots();
            return true;
        }
    }

    Swap(SourceSlot, TargetSlot);
    OnRep_Slots();
    return true;
}

// ========== 装备物品 ==========

void UComp_Inventory::EquipItem(int32 SlotIndex)
{
    if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("EquipItem: 无效槽位或为空"));
        return;
    }

    if (!GetOwner()->HasAuthority())
    {
        ServerEquipItem(SlotIndex);
        return;
    }
    ServerEquipItem_Implementation(SlotIndex);
}

bool UComp_Inventory::ServerEquipItem_Validate(int32 SlotIndex)
{
    return Slots.IsValidIndex(SlotIndex);
}

void UComp_Inventory::ServerEquipItem_Implementation(int32 SlotIndex)
{
    if (!GetOwner()->HasAuthority()) return;

    UE_LOG(LogTemp, Warning, TEXT("[Server] ServerEquipItem called, SlotIndex=%d, Slots.Num()=%d"), SlotIndex, Slots.Num());

    if (!Slots.IsValidIndex(SlotIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("[Server] Invalid SlotIndex!"));
        return;
    }

    const FInventorySlot& Slot = Slots[SlotIndex];
    if (Slot.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[Server] Slot is EMPTY!"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[Server] Slot has ItemName: %s, bCanEquip: %d, ItemClass: %s"),
        *Slot.ItemData.ItemName,
        Slot.ItemData.bCanEquip,
        Slot.ItemData.ItemClass ? *Slot.ItemData.ItemClass->GetName() : TEXT("NULL"));

    if (!Slot.ItemData.bCanEquip)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Server] Item cannot be equipped (bCanEquip false)"));
        return;
    }

    AG_Character* Character = Cast<AG_Character>(GetOwner());
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("[Server] Owner is not AG_Character"));
        return;
    }

    // 先拷贝 ItemData，因为 RemoveItem 会修改槽位并可能清空数据导致传递数据失败
    FItemData ItemDataCopy = Slot.ItemData;

    // 从背包移除一个该物品
    if (!RemoveItem(SlotIndex, 1))
    {
        UE_LOG(LogTemp, Error, TEXT("[Server] Failed to remove item from inventory"));
        return;
    }

    // 使用拷贝的数据调用角色装备函数
    Character->EquipWeapon(ItemDataCopy);
}

// 按ID查找第一个匹配的槽位（用于移除时快速定位，实际移除时需要多次查找）
int32 UComp_Inventory::FindSlotByID(FName ItemID) const
{
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (!Slots[i].IsEmpty() && Slots[i].ItemData.ID == ItemID)
            return i;
    }
    return INDEX_NONE;
}

// 按ID移除指定数量（尽量从多个槽位扣除）
bool UComp_Inventory::RemoveItemByID(FName ItemID, int32 Quantity)
{
    if (!GetOwner()->HasAuthority()) return false;
    if (Quantity <= 0 || ItemID.IsNone()) return false;

    int32 Remaining = Quantity;
    // 从前往后遍历，依次扣除
    for (int32 i = 0; i < Slots.Num() && Remaining > 0; ++i)
    {
        FInventorySlot& Slot = Slots[i];
        if (Slot.IsEmpty() || Slot.ItemData.ID != ItemID)
            continue;

        int32 Available = Slot.Quantity;
        int32 Take = FMath::Min(Remaining, Available);
        Slot.Quantity -= Take;
        Remaining -= Take;

        // 如果该槽位清空，置空
        if (Slot.Quantity == 0)
        {
            Slot = FInventorySlot();
        }
    }

    // 如果完全扣除成功，触发更新
    if (Remaining == 0)
    {
        OnRep_Slots();
        return true;
    }
    else
    {
        // 部分扣除或未扣除，但已经修改了部分槽位，仍需要广播（因为客户端需要同步）
        OnRep_Slots();
        UE_LOG(LogTemp, Warning, TEXT("RemoveItemByID: 背包中物品 %s 数量不足，仅移除 %d 个"), *ItemID.ToString(), Quantity - Remaining);
        return false; // 未能完全移除所需数量
    }
}
