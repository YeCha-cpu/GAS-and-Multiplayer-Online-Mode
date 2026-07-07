// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/G_PlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Character/G_Character.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/SpringArmComponent.h"
#include "Component/Comp_Interaction.h"   // 引入交互组件
#include "Component/Comp_Inventory.h"

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
	EnhancedInputComponent->BindAction(LeftClickAction, ETriggerEvent::Started, this, &AG_PlayerController::OnLeftClick);
	EnhancedInputComponent->BindAction(OpenInventoryAction, ETriggerEvent::Started, this, &AG_PlayerController::OpenInventory);
	
	
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

void AG_PlayerController::OnLeftClick()
{
	if (bFreeLook)
		return;

	if (InteractionComp)
	{
		InteractionComp->TryInteract();
	}
}

void AG_PlayerController::OpenInventory_Implementation()
{
	
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
