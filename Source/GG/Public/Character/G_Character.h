// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "G_Character.generated.h"

class AG_Bullet;
class UComp_Inventory;
class UAS_Player;
class UGameplayEffect;
class UGameplayAbility;
class UAbilitySystemComponent;
class UCameraComponent;
class USpringArmComponent;
class AG_Items;

UCLASS()
class GG_API AG_Character : public ACharacter
{
	GENERATED_BODY()

public:
	AG_Character();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	
	// ========== 组件 ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;

	// ========== GAS相关 ==========
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	UAS_Player* AttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Effects")
	TSubclassOf<UGameplayEffect> DefaultInitialEffect;

	// ========== 新增：战斗/装备相关 ==========
	// 装备武器
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EquipWeapon(const FItemData& ItemData);

	// 卸下武器（可被蓝图调用）
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void UnequipWeapon();

	// 当前装备的武器（网络复制）
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon, BlueprintReadOnly, Category = "Combat")
	AG_Items* EquippedWeapon;

	// 是否为战斗姿态（射击姿态）
	UPROPERTY(ReplicatedUsing = OnRep_IsCombatMode, BlueprintReadOnly, Category = "Combat")
	bool bIsCombatMode = false;

	// 武器挂载点 Socket 名称（可在蓝图编辑）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName WeaponSocketName = "HandGrip_R";

	// 姿态变化时蓝图事件（可选）
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnCombatModeChanged(bool bNewMode);

protected:
	virtual void InitAbilityActorInfo();
	
	UFUNCTION(BlueprintCallable, Category = "GAS")
	void InitializeGAS();

private:
	// 内部卸下（清除武器Actor，不处理背包归还）
	void InternalUnequipWeapon();

	// RepNotify 回调
	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_IsCombatMode();
	
	// 在 AG_Character 类中

public:
	// 切换射击模式（可从蓝图或控制器调用）
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetShootingMode(bool bEnabled);

	// 当前是否处于射击姿态（网络复制）
	UPROPERTY(ReplicatedUsing = OnRep_ShootingMode, BlueprintReadOnly, Category = "Combat")
	bool bIsShootingMode = false;

	// 准星UI类（在蓝图设置）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> CrosshairWidgetClass;

protected:
	// 网络复制回调
	UFUNCTION()
	void OnRep_ShootingMode();

	// 服务器RPC, 设置射击模式
	UFUNCTION(Server, Reliable)
	void ServerSetShootingMode(bool bEnabled);
	
	// 在头文件中声明蓝图可实现事件
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat", DisplayName = "当射击姿态改变时")
	void OnShootingModeChanged(bool bNewMode);
	
private:
	// 当前准星UI实例
	UPROPERTY(Transient)
	UUserWidget* CrosshairWidgetInstance;
	
public:
	// 开火（客户端调用，自动走 RPC）
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void FireWeapon();

protected:
	// ★ 修改：服务器执行实际开火，接收客户端计算好的位置和方向
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFireWeapon(const FVector& MuzzleLocation, const FVector& FireDirection);

	// 子弹生成参数
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<AG_Bullet> BulletClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float BulletSpeed = 2000.0f;

	// 改为 Socket 名称（对应武器骨骼网格体上的 Socket）
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	FName MuzzleSocketName = "Muzzle_Socket";   // 默认名称，可在蓝图修改，需与武器骨骼上的Socket名一致

	// 获取开火起点（结合武器）
	FVector GetFireStartLocation() const;
	FRotator GetFireRotation() const;
	
private:
	// 获取枪口世界位置（使用武器上的 Socket）
	FVector GetMuzzleLocation() const;

	// 计算瞄准方向（客户端调用）
	FVector CalculateFireDirection() const;
};