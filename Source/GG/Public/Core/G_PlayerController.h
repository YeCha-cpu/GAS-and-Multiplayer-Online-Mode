// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "G_PlayerController.generated.h"

class UComp_Interaction;
class UCharacterUIWidget;

UCLASS()
class GG_API AG_PlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AG_PlayerController();
    virtual void Tick(float DeltaTime) override;

    // 角色 UI 类（需要在蓝图中设置）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UCharacterUIWidget> CharacterUIClass;

    // 当前 UI 实例
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    UCharacterUIWidget* CharacterUIWidget;

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnRep_PlayerState() override;

    // ========== 输入动作（与蓝图中配置的 IMC 匹配） ==========
    UPROPERTY(EditAnywhere, Category = "G|Input")
    TObjectPtr<UInputMappingContext> IMC;

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
    TObjectPtr<UInputAction> ToggleViewAction;
    UPROPERTY(EditAnywhere, Category = "G|Input")
    TObjectPtr<UInputAction> LeftClickAction;
    UPROPERTY(EditAnywhere, Category = "G|Input")
    TObjectPtr<UInputAction> OpenInventoryAction;

    // ========== 输入回调函数 ==========
    void MoveInput(const FInputActionValue& InputActionValue);
    void LookInput(const FInputActionValue& InputActionValue);
    void ZoomInput(const FInputActionValue& InputActionValue);
    void JumpInput();
    void EquipInput();
    void InteractInput();
    void AttackInput();
    void ToggleViewMode();
    void OnLeftClick();
    UFUNCTION(BlueprintNativeEvent, Category = "Inventory")
    void OpenInventory();

    // 背包移动（RPC）
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void RequestMoveItem(int32 SourceIndex, int32 TargetIndex);
    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerMoveItem(int32 SourceIndex, int32 TargetIndex);

    // ========== 暴露给蓝图的背包开关变量 ==========
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "G|Inventory", meta = (DisplayName = "是否已打开背包"))
    bool bIsOpenInventory = false;

private:
    // 创建并绑定 UI（内部）
    void CreateAndBindUI();

    // 视角模式状态
    bool bFreeLook = true;
    FRotator SavedSpringArmRotation;

    // 交互组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
    UComp_Interaction* InteractionComp;

    // 背包 UI 引用
    UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UUserWidget> InventoryWidget;
};