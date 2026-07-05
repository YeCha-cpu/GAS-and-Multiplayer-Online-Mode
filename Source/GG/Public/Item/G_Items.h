// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "G_Items.generated.h"

class USkeletalMeshComponent;

// 创建一个结构体，用于保存物品信息
USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "物品ID")
	FName ID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "物品类型")
	FGameplayTag ItemType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "物品名称")
	FString ItemName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "物品描述")
	FText ItemDescription;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "物品重量")
	float ItemWeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "使用时恢复数值")
	float ItemUseValue;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "物品最大堆叠数量")
	int32 ItemMaxStack;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "物品UI图标")
	TSoftObjectPtr<UTexture2D> ItemIcon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "物品价值")
	int32 ItemValue;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "是否可交互")
	bool bCanInteract;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "是否可拾取")
	bool bCanPickUp;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "是否可装备")
	bool bCanEquip;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "是否可使用")
	bool bCanUse;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "是否可堆叠")
	bool bCanStack;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "是否可维修")
	bool bCanRepair;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "是否可交易")
	bool bCanTrade;
};

UCLASS()
class GG_API AG_Items : public AActor
{
	GENERATED_BODY()
	
public:	
	AG_Items();
	virtual void Tick(float DeltaTime) override;

	// 获取物品数据
	UFUNCTION(BlueprintCallable, Category = "Item")
	const FItemData& GetItemData() const { return ItemData; }
	
	// 设置物品数据，初始化物品信息时调用（在蓝图中用数据表格获取行）
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetItemData(const FItemData& Data);

	// 设置覆层材质（高亮）
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetHighlightMaterial(UMaterialInterface* Material);

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> ItemMesh;

	// 物品数据（可在蓝图或实例中编辑）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FItemData ItemData;
};