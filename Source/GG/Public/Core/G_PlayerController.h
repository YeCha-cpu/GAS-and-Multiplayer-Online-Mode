// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "G_PlayerController.generated.h"

class UComp_Interaction;

/**
 * 
 */
UCLASS()
class GG_API AG_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AG_PlayerController();
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditAnywhere, Category = "G|Movement")
	float RunSpeed = 800.0f;
	UPROPERTY(EditAnywhere, Category = "G|Movement")
	float WalkSpeed = 600.0f;
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
	// 是否已打开背包
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "G|Inventory", DisplayName = "是否已打开背包")
	bool bIsOpenInventory = false;
	
private:
	/* ------------------------------ IMC ------------------------------ */
	UPROPERTY(EditAnywhere, Category = "G|Input")
	TObjectPtr<UInputMappingContext> IMC;
	
	/* ------------------------------ 输入动作 ------------------------------ */
	UPROPERTY(EditAnywhere, Category = "G|Input")
	TObjectPtr<UInputAction> MoveAction;
	
	UPROPERTY(EditAnywhere, Category = "G|Input")
	TObjectPtr<UInputAction> LookAction;
	
	UPROPERTY(EditAnywhere, Category = "G|Input")
	TObjectPtr<UInputAction> ZoomAction;
	
	UPROPERTY(EditAnywhere, Category = "G|Input")
	TObjectPtr<UInputAction> JumpAction;
	
	UPROPERTY(EditAnywhere, Category = "G|Input")
	TObjectPtr<UInputAction> RunAction;
	
	UPROPERTY(EditAnywhere, Category = "G|Input")
	TObjectPtr<UInputAction> EquipAction;
	
	UPROPERTY(EditAnywhere, Category = "G|Input")
	TObjectPtr<UInputAction> InteractAction;
	
	UPROPERTY(EditAnywhere, Category = "G|Input")
	TObjectPtr<UInputAction> AttackAction;
	
	UPROPERTY(EditAnywhere, Category = "G|Input")
	TObjectPtr<UInputAction> ToggleViewAction;     // Q 键切换视角

	UPROPERTY(EditAnywhere, Category = "G|Input")
	TObjectPtr<UInputAction> LeftClickAction;      // 鼠标左键点击（俯视角下交互）
	
	UPROPERTY(EditAnywhere, Category = "G|Input")
	TObjectPtr<UInputAction> OpenInventoryAction;  // Tab打开背包

	/* ------------------------------ 输入回调函数 ------------------------------ */
	void MoveInput(const FInputActionValue& InputActionValue);
	void LookInput(const FInputActionValue& InputActionValue);
	void ZoomInput(const FInputActionValue& InputActionValue);
	void JumpInput();
	void EquipInput();
	void InteractInput();
	void AttackInput();
	void ToggleViewMode();                         // 切换视角模式
	void OnLeftClick();                            // 鼠标左键点击（执行射线检测）
protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Inventory", DisplayName = "打开/关闭背包")
	void OpenInventory();							// 打开背包
private:
	/* ------------------------------ 状态变量 ------------------------------ */
	bool bFreeLook = true;                        // true=自由视角, false=俯视角锁定

	// 保存自由视角下的弹簧臂旋转（用于恢复）
	FRotator SavedSpringArmRotation;

	/* ------------------------------ 交互组件 ------------------------------ */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	UComp_Interaction* InteractionComp;

	/* ------------------------------ 辅助函数 ------------------------------ */
	// 从鼠标位置发射射线检测交互物（已弃用，改由组件处理）
};