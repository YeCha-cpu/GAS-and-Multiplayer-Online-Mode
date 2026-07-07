// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/Comp_Inventory.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Component/InventorySlotData.h"   // 新增包含

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

// 背包数据更新时的回调
void UComp_Inventory::OnRep_Slots()
{
    UE_LOG(LogTemp, Log, TEXT("背包数据已更新"));
    
    // 新方式：广播委托
    BroadcastInventoryUpdated();
}

// ---------- 添加物品 ----------
bool UComp_Inventory::AddItem(const FItemData& ItemData, int32 Quantity)
{
    if (!GetOwner()->HasAuthority()) return false;
    if (Quantity <= 0 || ItemData.ID.IsNone()) return false;

    // 当前要添加物品的剩余数量
    int32 Remaining = Quantity;

    // 尝试堆叠物品
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

    // 如果仍有剩余数量，则尝试放入空槽
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

    // 如果还有剩余数量，则说明背包已满无法添加
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

    // 生成掉落物品
    SpawnDroppedItem(Slot.ItemData, DropQuantity);
    RemoveItem(SlotIndex, DropQuantity);
}

void UComp_Inventory::SpawnDroppedItem(const FItemData& ItemData, int32 Quantity)
{
    UE_LOG(LogTemp, Log, TEXT("生成丢弃物品: %s x%d"), *ItemData.ItemName, Quantity);
    // 实际生成逻辑可由蓝图或 C++ 扩展
}

void UComp_Inventory::ApplyItemEffect(const FItemData& ItemData)
{
    UE_LOG(LogTemp, Log, TEXT("使用物品 %s，恢复值 %f"), *ItemData.ItemName, ItemData.ItemUseValue);
}

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

// ---------- 新增：为 TileView 提供数据对象 ----------
TArray<UInventorySlotData*> UComp_Inventory::GetSlotDataObjects()
{
    TArray<UInventorySlotData*> Result;
    
    // 遍历所有槽位（包括空槽）
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        // 不再跳过空槽，所有槽都创建数据
        UInventorySlotData* Data = NewObject<UInventorySlotData>(this);
        Data->SlotData = Slots[i];  // 空槽的 SlotData 就是默认空值（Quantity=0, ItemData=空）
        Data->SlotIndex = i;
        Result.Add(Data);
    }
    
    return Result;
}

void UComp_Inventory::BroadcastInventoryUpdated()
{
    OnInventoryUpdatedDelegate.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("广播背包更新事件"));
}

bool UComp_Inventory::MoveItem(int32 SourceIndex, int32 TargetIndex)
{
    // 必须在服务器执行
    if (!GetOwner()->HasAuthority()) return false;

    // 边界检查
    if (!Slots.IsValidIndex(SourceIndex) || !Slots.IsValidIndex(TargetIndex)) return false;
    if (SourceIndex == TargetIndex) return true; // 相同槽位，无操作

    FInventorySlot& SourceSlot = Slots[SourceIndex];
    FInventorySlot& TargetSlot = Slots[TargetIndex];

    // 若源槽为空，无操作
    if (SourceSlot.IsEmpty()) return false;

    // ---- 情况1：目标为空 - 直接移动 ----
    if (TargetSlot.IsEmpty())
    {
        TargetSlot = SourceSlot;
        SourceSlot = FInventorySlot();
        OnRep_Slots(); // 触发更新
        return true;
    }

    // ---- 情况2：目标非空 - 检查是否可堆叠 ----
    if (SourceSlot.CanStackWith(TargetSlot.ItemData))
    {
        // 计算可容纳的数量
        int32 MaxStack = SourceSlot.ItemData.ItemMaxStack; // 两者一样
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
        else
        {
            // 目标已满，无法合并 → 执行交换（视为不同物品）
            // 继续到交换逻辑
        }
    }

    // ---- 情况3：不同物品或无法堆叠 → 交换 ----
    Swap(SourceSlot, TargetSlot);
    OnRep_Slots();
    return true;
}