// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "G_Character.generated.h"

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
	UPROPERTY(EditDefaultsOnly)
	USpringArmComponent* SpringArmComp;
	
	UPROPERTY(EditDefaultsOnly)
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
	FName WeaponSocketName = "weapon_socket";

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
};