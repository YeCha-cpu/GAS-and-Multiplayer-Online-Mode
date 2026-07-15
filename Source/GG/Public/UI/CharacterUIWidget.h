// CharacterUIWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystemComponent.h"
#include "GAS/AS_Player.h"        // 你的属性集头文件
#include "CharacterUIWidget.generated.h"

/**
 * 角色 UI 基类，自动绑定 GAS 属性变化
 * 在蓝图中继承此类，重写 OnHealthChanged 等事件即可更新界面
 */
UCLASS(Abstract, Blueprintable)
class GG_API UCharacterUIWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 原生构造
    virtual void NativeConstruct() override;
    // 原生销毁
    virtual void NativeDestruct() override;

    // 属性绑定：获取 ASC（由子类或实例化时设置）
    UFUNCTION(BlueprintCallable, Category = "UI|GAS")
    void BindToAbilitySystem(UAbilitySystemComponent* InASC);
    
    // 更新武器信息（蓝图可重写）
    UFUNCTION(BlueprintImplementableEvent, Category = "UI|Weapon")
    void UpdateWeaponInfo(const FItemData& WeaponData, int32 CurrentAmmo, int32 MaxAmmo);

protected:
    // 当前绑定的 ASC（弱引用，避免循环依赖）
    UPROPERTY(BlueprintReadOnly, Category = "UI|GAS")
    TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    // -------- 蓝图可重写的事件（更新 UI，定义每一种不同的物品所产生的影响） --------
    UFUNCTION(BlueprintImplementableEvent, Category = "UI|Update")
    void OnHealthChanged(float NewHealth, float MaxHealth);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI|Update")
    void OnStaminaChanged(float NewStamina, float MaxStamina);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI|Update")
    void OnDefenseChanged(float NewDefense, float MaxDefense);

    // 辅助：获取属性集指针（安全）
    UAS_Player* GetAttributeSet() const;

private:
    // 存储委托句柄，用于取消绑定
    FDelegateHandle HealthChangedHandle;
    FDelegateHandle MaxHealthChangedHandle;
    FDelegateHandle StaminaChangedHandle;
    FDelegateHandle MaxStaminaChangedHandle;
    FDelegateHandle DefenseChangedHandle;
    FDelegateHandle MaxDefenseChangedHandle;

    // 内部更新函数（由委托触发）
    void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);
    void OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data);
    void OnStaminaAttributeChanged(const FOnAttributeChangeData& Data);
    void OnMaxStaminaAttributeChanged(const FOnAttributeChangeData& Data);
    void OnDefenseAttributeChanged(const FOnAttributeChangeData& Data);
    void OnMaxDefenseAttributeChanged(const FOnAttributeChangeData& Data);

    // 刷新所有 UI（初始调用）
    void RefreshAllValues();
};