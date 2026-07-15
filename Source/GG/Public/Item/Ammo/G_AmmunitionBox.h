// G_AmmunitionBox.h
#pragma once

#include "CoreMinimal.h"
#include "Item/G_Items.h"
#include "G_AmmunitionBox.generated.h"

UCLASS()
class GG_API AG_AmmunitionBox : public AG_Items
{
	GENERATED_BODY()

public:
	AG_AmmunitionBox();

	// ★ 弹药模板：用于背包中使用时补充的弹药类型（在蓝图中配置）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoBox")
	FItemData AmmoItemData;

	// 使用效果（拾取时调用，但改为不直接补充，而是将弹药箱自身放入背包）
	virtual void OnUsed_Implementation(APawn* User, const FItemData& InItemData) override;

	// ★ 新增：从数据表获取弹药完整数据（用于UI显示）
	UFUNCTION(BlueprintCallable, Category = "AmmoBox")
	FItemData GetAmmoData() const { return AmmoItemData; }
};