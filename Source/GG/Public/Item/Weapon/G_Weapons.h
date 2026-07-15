// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/G_Items.h"
#include "Character/G_Character.h"
#include "Component/Comp_Inventory.h"
#include "G_Weapons.generated.h"

/**
 * 武器基类，继承自 AG_Items
 * 添加射速、伤害、弹夹等武器特有属性
 */
UCLASS()
class GG_API AG_Weapons : public AG_Items
{
	GENERATED_BODY()

public:
	AG_Weapons();
	virtual void OnUsed_Implementation(APawn* User, const FItemData& InItemData) override;
	
	// 射速：单位秒（两次射击之间的最小间隔）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FireRate = 0.1f;   // 默认 0.1 秒（即 10 发/秒）
	
	// 该武器使用的弹药物品ID（需与背包中物品ID匹配）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName AmmoItemID;

	// 武器单发伤害
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Weapon")
	float WeaponDamage = 10.0f;

	// 弹夹容量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Weapon")
	int32 MagazineCapacity = 30;

	// 当前弹药数量（复制，并在变化时触发通知）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentAmmo, Category = "Weapon")
	int32 CurrentAmmo = 0;

	// 换弹所需时间（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float ReloadTime = 2.0f;

	// 是否正在换弹（复制）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Weapon")
	bool bIsReloading = false;

	// 客户端请求换弹
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Reload();

	// 服务器执行换弹（RPC）
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReload();

	// 消耗弹药（服务器调用），返回是否成功
	bool ConsumeAmmo(int32 Amount = 1);

	// 获取当前弹药
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	// 获取最大弹药
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetMaxAmmo() const { return MagazineCapacity; }

	// 是否满弹
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsFullAmmo() const { return CurrentAmmo >= MagazineCapacity; }

	// 当弹药改变时触发的蓝图事件（用于更新UI）
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void OnAmmoChanged(int32 NewAmmo);

	// 换弹完成时的蓝图事件
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void OnReloadComplete();

	// 网络复制声明
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// 弹药复制回调
	UFUNCTION()
	void OnRep_CurrentAmmo();

	// 换弹计时器句柄
	FTimerHandle ReloadTimerHandle;

	// 换弹完成时的内部处理
	void FinishReload();
};