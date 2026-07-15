// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/G_PlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Character/G_Character.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/SpringArmComponent.h"
#include "Component/Comp_Interaction.h"
#include "Component/Comp_Inventory.h"
#include "Core/G_PlayerState.h"          // 新增：用于获取 ASC
#include "UI/CharacterUIWidget.h"        // 新增：用于 UI 类
#include "AbilitySystemComponent.h"      // 新增：用于 ASC 操作
#include "TimerManager.h"
#include "Item/Weapon/G_Weapons.h"       // ★ 新增：用于访问武器换弹逻辑

AG_PlayerController::AG_PlayerController()
{
    bReplicates = true;
    PrimaryActorTick.bCanEverTick = true;
}

void AG_PlayerController::BeginPlay()
{
    Super::BeginPlay();
    check(IMC);

    // 查找蓝图中添加的组件
    InteractionComp = FindComponentByClass<UComp_Interaction>();
    if (!InteractionComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("未找到 InteractionComp，请确认是否在蓝图中添加了该组件！"));
    }

    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
    if (Subsystem)
    {
        Subsystem->AddMappingContext(IMC, 0);
    }

    // 初始为自由视角，隐藏鼠标
    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());

    // 创建并绑定角色 UI（仅客户端执行）
    CreateAndBindUI();
}

void AG_PlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    // 当角色被控制时重新绑定 UI（例如切换角色或重生）
    CreateAndBindUI();
}

void AG_PlayerController::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    // 当 PlayerState 复制完成时，重新绑定 UI（确保 ASC 已就绪）
    CreateAndBindUI();
}

void AG_PlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 仅在俯视角模式下更新交互检测
    if (!bFreeLook && InteractionComp)
    {
        InteractionComp->UpdateInteraction();
    }
}

void AG_PlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);

    EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AG_PlayerController::MoveInput);
    EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AG_PlayerController::LookInput);
    EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AG_PlayerController::ZoomInput);
    EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AG_PlayerController::JumpInput);
    EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Started, this, &AG_PlayerController::EquipInput);
    EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AG_PlayerController::InteractInput);
    EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AG_PlayerController::AttackInput);
    EnhancedInputComponent->BindAction(ToggleViewAction, ETriggerEvent::Started, this, &AG_PlayerController::ToggleViewMode);
    EnhancedInputComponent->BindAction(LeftClickAction, ETriggerEvent::Triggered, this, &AG_PlayerController::OnLeftClick);
    EnhancedInputComponent->BindAction(OpenInventoryAction, ETriggerEvent::Started, this, &AG_PlayerController::OpenInventory);
    EnhancedInputComponent->BindAction(ShootModeAction, ETriggerEvent::Started, this, &AG_PlayerController::StartShootMode);
    EnhancedInputComponent->BindAction(ShootModeAction, ETriggerEvent::Completed, this, &AG_PlayerController::EndShootMode);
    EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AG_PlayerController::ReloadInput);
}

void AG_PlayerController::MoveInput(const FInputActionValue& InputActionValue)
{
    const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();
    const FRotator Rotation = GetControlRotation();
    const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    if (APawn* ControlledPawn = GetPawn<APawn>())
    {
        ControlledPawn->AddMovementInput(ForwardDirection, InputAxisVector.Y);
        ControlledPawn->AddMovementInput(RightDirection, InputAxisVector.X);
    }
}

void AG_PlayerController::LookInput(const FInputActionValue& InputActionValue)
{
    // 俯视角模式下禁止自由旋转
    if (!bFreeLook)
        return;

    const FVector2D LookAxisVector = InputActionValue.Get<FVector2D>();
    if (APawn* ControlledPawn = GetPawn<APawn>())
    {
        ControlledPawn->AddControllerYawInput(LookAxisVector.X);
        ControlledPawn->AddControllerPitchInput(LookAxisVector.Y);
    }
}

void AG_PlayerController::ZoomInput(const FInputActionValue& InputActionValue)
{
    const float ZoomValue = InputActionValue.Get<float>();

    if (AG_Character* G_Character = Cast<AG_Character>(GetCharacter()))
    {
        if (USpringArmComponent* SpringArm = G_Character->FindComponentByClass<USpringArmComponent>())
        {
            SpringArm->TargetArmLength += ZoomValue * 100.f;
            SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength, 200.f, 1000.f);
        }
    }
}

void AG_PlayerController::JumpInput()
{
    if (AG_Character* G_Character = Cast<AG_Character>(GetCharacter()))
    {
        G_Character->Jump();
    }
}

void AG_PlayerController::InteractInput()
{
    // 暂未实现
}

void AG_PlayerController::EquipInput()
{
    // 暂未实现
}

void AG_PlayerController::AttackInput()
{
    // 暂未实现
}

void AG_PlayerController::ToggleViewMode()
{
    bFreeLook = !bFreeLook;          // 切换状态

    if (bFreeLook)                   // 切回自由视角
    {
        // 1. 隐藏鼠标
        bShowMouseCursor = false;
        SetInputMode(FInputModeGameOnly());

        // 2. 恢复弹簧臂旋转（之前保存的）
        if (AG_Character* G_Character = Cast<AG_Character>(GetCharacter()))
        {
            if (USpringArmComponent* SpringArm = G_Character->FindComponentByClass<USpringArmComponent>())
            {
                SpringArm->SetWorldRotation(SavedSpringArmRotation);
            }
        }
    }
    else                             // 进入俯视角
    {
        // 1. 显示鼠标
        bShowMouseCursor = true;
        SetInputMode(FInputModeGameAndUI());   // 允许鼠标与 UI / 世界交互

        // 2. 保存当前弹簧臂旋转，并设置俯视角（例如 Pitch=-80°）
        if (AG_Character* G_Character = Cast<AG_Character>(GetCharacter()))
        {
            if (USpringArmComponent* SpringArm = G_Character->FindComponentByClass<USpringArmComponent>())
            {
                SavedSpringArmRotation = SpringArm->GetComponentRotation();   // 保存
                SpringArm->SetWorldRotation(FRotator(-80.f, 0.f, 0.f));      // 俯视
            }
        }
    }
}

void AG_PlayerController::StartShootMode()
{
    if (AG_Character* Char = Cast<AG_Character>(GetPawn()))
    {
        Char->SetShootingMode(true);
    }
}

void AG_PlayerController::EndShootMode()
{
    if (AG_Character* Char = Cast<AG_Character>(GetPawn()))
    {
        Char->SetShootingMode(false);
    }
}

// ★ 修改：增加详细日志
void AG_PlayerController::OnLeftClick()
{
    AG_Character* Char = Cast<AG_Character>(GetPawn());
    if (!Char)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnLeftClick: 无法获取角色"));
        return;
    }

    // 打印当前射击模式状态
    UE_LOG(LogTemp, Warning, TEXT("OnLeftClick: bIsShootingMode = %d"), Char->bIsShootingMode);

    // 如果处于射击模式，执行开火
    if (Char->bIsShootingMode)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnLeftClick: 调用 FireWeapon"));
        Char->FireWeapon();
        return;
    }

    // 否则执行原有的交互逻辑（俯视角拾取）
    if (bFreeLook)
        return;

    if (InteractionComp)
    {
        InteractionComp->TryInteract();
    }
}

void AG_PlayerController::OpenInventory_Implementation()
{
    // 暂未实现
}

void AG_PlayerController::RequestMoveItem(int32 SourceIndex, int32 TargetIndex)
{
    if (HasAuthority())
    {
        // 如果已经是服务器，直接执行
        ServerMoveItem(SourceIndex, TargetIndex);
    }
    else
    {
        // 客户端调用 RPC
        ServerMoveItem(SourceIndex, TargetIndex);
    }
}

void AG_PlayerController::ServerMoveItem_Implementation(int32 SourceIndex, int32 TargetIndex)
{
    if (APawn* MyPawn = GetPawn())
    {
        // 获取角色的背包组件
        if (UComp_Inventory* Comp_Inventory = MyPawn->FindComponentByClass<UComp_Inventory>())
        {
            // 执行背包组件的移动项目函数
            Comp_Inventory->MoveItem(SourceIndex, TargetIndex);
        }
    }
}

// ---------- 创建并绑定角色 UI ----------
void AG_PlayerController::CreateAndBindUI()
{
    // 仅在本地控制的客户端创建 UI（避免服务器或无关客户端生成）
    if (!IsLocalController())
        return;

    if (!CharacterUIClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateAndBindUI: CharacterUIClass 未设置，请在蓝图中指定"));
        return;
    }

    // 如果已存在 UI，先移除
    if (CharacterUIWidget)
    {
        CharacterUIWidget->RemoveFromParent();
        CharacterUIWidget = nullptr;
    }

    // 创建 Widget 实例
    CharacterUIWidget = CreateWidget<UCharacterUIWidget>(this, CharacterUIClass);
    if (!CharacterUIWidget)
        return;

    CharacterUIWidget->AddToViewport();

    // 获取 PlayerState 中的 AbilitySystemComponent
    AG_PlayerState* PS = GetPlayerState<AG_PlayerState>();
    if (!PS)
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateAndBindUI: 无法获取 AG_PlayerState"));
        return;
    }

    UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
    if (ASC)
    {
        CharacterUIWidget->BindToAbilitySystem(ASC);
        UE_LOG(LogTemp, Log, TEXT("角色 UI 已成功绑定到 ASC"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateAndBindUI: ASC 无效，绑定失败"));
    }
}

// 新增：换弹输入处理
void AG_PlayerController::ReloadInput()
{
    AG_Character* Char = Cast<AG_Character>(GetPawn());
    if (!Char)
    {
        UE_LOG(LogTemp, Warning, TEXT("ReloadInput: 无法获取角色"));
        return;
    }

    // 获取当前装备的武器
    AG_Items* EquippedWeapon = Char->EquippedWeapon;
    if (!EquippedWeapon)
    {
        UE_LOG(LogTemp, Warning, TEXT("ReloadInput: 未装备武器"));
        return;
    }

    // 尝试转换为 AG_Weapons
    AG_Weapons* Weapon = Cast<AG_Weapons>(EquippedWeapon);
    if (!Weapon)
    {
        UE_LOG(LogTemp, Warning, TEXT("ReloadInput: 当前装备不是武器（AG_Weapons 类型）"));
        return;
    }

    // 调用换弹函数（内部自动处理客户端→服务器的RPC）
    Weapon->Reload();
}