// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Comp_Interaction.generated.h"

class AG_Items;

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GG_API UComp_Interaction : public UActorComponent
{
	GENERATED_BODY()

public:	
	UComp_Interaction();

	// 每帧调用，执行射线检测并更新高亮（由控制器在俯视角模式下调用）
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void UpdateInteraction();

	// 尝试与当前高亮物品交互（拾取）
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

	// 获取当前高亮的目标（可交互物）
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AG_Items* GetCurrentInteractable() const { return CurrentInteractable; }

protected:
	virtual void BeginPlay() override;

	// 交互距离（蓝图可调整）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionRange = 500.0f;

	// 高亮材质（在蓝图中赋值）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	UMaterialInterface* HighlightMaterial;

	// 当前高亮物品
	UPROPERTY(Transient)
	AG_Items* CurrentInteractable;

	// 上一帧高亮物品（用于清除旧高亮）
	UPROPERTY(Transient)
	AG_Items* LastInteractable;

	// 核心射线检测函数，返回命中的AG_Items（不进行条件筛选）
	AG_Items* TraceForInteractable() const;

	// 清除高亮（将上次高亮物品的材质恢复）
	void ClearHighlight();

	// 应用高亮材质
	void ApplyHighlight(AG_Items* Item);

	// 拾取成功时调用的蓝图事件（可在蓝图中重载实现放入背包逻辑）
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnItemPickedUp(AG_Items* PickedItem);
};