// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/G_Items.h"
#include "Components/WrapBox.h"
#include "Blueprint/UserWidget.h"
#include "Comp_Inventory.generated.h"

// 对物品增删改查后，通知背包更新信息
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdatedDelegate);

// 背包槽位结构体
USTRUCT(BlueprintType)
struct FInventorySlot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FItemData ItemData;

    UPROPERTY(BlueprintReadWrite)
    int32 Quantity = 0;

    FInventorySlot() : Quantity(0) {}
    FInventorySlot(const FItemData& InItemData, int32 InQuantity) : ItemData(InItemData), Quantity(InQuantity) {}

    bool IsEmpty() const { return Quantity <= 0 || ItemData.ID.IsNone(); }

    bool CanStackWith(const FItemData& Other) const
    {
        if (IsEmpty() || Other.ID.IsNone()) return false;
        return ItemData.ID == Other.ID && ItemData.bCanStack;
    }
};

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GG_API UComp_Inventory : public UActorComponent
{
    GENERATED_BODY()

public:
    UComp_Inventory();

    // 背包容量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 Capacity = 20;

    // 获取所有槽位（只读）
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    const TArray<FInventorySlot>& GetSlots() const { return Slots; }

    // -------- 核心功能 --------
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItem(const FItemData& ItemData, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItem(int32 SlotIndex, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void UseItem(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void DropItem(int32 SlotIndex, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    int32 GetItemTotalQuantity(FName ItemID) const;

    // -------- 新增：为 TileView 提供数据对象数组 --------
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    TArray<UInventorySlotData*> GetSlotDataObjects();
    
    // 创建背包更新委托
    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventoryUpdatedDelegate OnInventoryUpdatedDelegate;

    // 通知所有订阅者刷新背包信息
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void BroadcastInventoryUpdated();

    // 在服务器上执行物品移动（合并/交换），返回是否成功
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool MoveItem(int32 SourceIndex, int32 TargetIndex);
    
protected:
    virtual void BeginPlay() override;

    // 背包槽位数组（网络同步）
    UPROPERTY(ReplicatedUsing = OnRep_Slots)
    TArray<FInventorySlot> Slots;

    // 
    UFUNCTION()
    void OnRep_Slots();

    // 【服务器】上使用物品
    UFUNCTION(Server, Reliable)
    void ServerUseItem(int32 SlotIndex);

    // 【服务器】上丢弃物品
    UFUNCTION(Server, Reliable)
    void ServerDropItem(int32 SlotIndex, int32 Quantity);

    // 生成掉落物品
    void SpawnDroppedItem(const FItemData& ItemData, int32 Quantity);
    
    // 应用物品效果
    void ApplyItemEffect(const FItemData& ItemData);

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};