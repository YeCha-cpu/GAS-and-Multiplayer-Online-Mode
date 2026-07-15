// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/G_Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystemComponent.h"
#include "Core/G_PlayerState.h"
#include "GameplayEffect.h"
#include "BP/G_Bullet.h"
#include "Component/Comp_Inventory.h"
#include "Engine/Engine.h"
#include "GAS/AS_Player.h"
#include "Net/UnrealNetwork.h"
#include "Item/G_Items.h"
#include "Item/Weapon/G_Weapons.h"   // 新增包含武器头文件
#include "Components/SkeletalMeshComponent.h"
#include "Core/G_PlayerController.h"
#include "UI/CharacterUIWidget.h"
#include "Component/Comp_Inventory.h"   // 包含背包组件头文件

AG_Character::AG_Character()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 400.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->TargetArmLength = 300.f;
	SpringArmComp->bUsePawnControlRotation = true;
	
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);
}

void AG_Character::BeginPlay()
{
	Super::BeginPlay();

	// 客户端提前初始化属性集，防止属性复制回调时缺失
	if (GetLocalRole() == ROLE_AutonomousProxy || GetLocalRole() == ROLE_SimulatedProxy)
	{
		InitAbilityActorInfo();
	}
}

void AG_Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 更新瞄准角度（仅在本地控制的客户端执行）
	if (IsLocallyControlled())
	{
		UpdateAimPitch();
	}
}

void AG_Character::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilityActorInfo();
	if (HasAuthority())
	{
		InitializeGAS();
	}
}

void AG_Character::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitAbilityActorInfo();
}

void AG_Character::InitAbilityActorInfo()
{
	AG_PlayerState* G_PlayerState = GetPlayerState<AG_PlayerState>();
	if (!G_PlayerState) return;

	UAbilitySystemComponent* ASC = G_PlayerState->GetAbilitySystemComponent();
	if (!ASC) return;

	ASC->InitAbilityActorInfo(G_PlayerState, this);
	AbilitySystemComponent = ASC;

	// 从 PlayerState 获取属性集（确保 Outer 为 AG_PlayerState，实现 IAbilitySystemInterface）
	AttributeSet = G_PlayerState->GetAttributeSet();
	if (!AttributeSet)
	{
		// 如果 PlayerState 中尚未创建，则创建并设置（兼容旧逻辑，但通常不会发生）
		AttributeSet = NewObject<UAS_Player>(G_PlayerState);
	}

	// 确保属性集已注册到 ASC（若未注册则添加）
	if (AttributeSet)
	{
		TArray<UAttributeSet*> Sets = ASC->GetSpawnedAttributes();
		if (!Sets.Contains(AttributeSet))
		{
			ASC->AddAttributeSetSubobject(AttributeSet);
		}
	}
}

void AG_Character::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(AG_Character, EquippedWeapon, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AG_Character, bIsCombatMode, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AG_Character, bIsShootingMode, COND_None, REPNOTIFY_Always);
	// ★ 修改：复制 AimPitch，并添加 REPNOTIFY_Always 以触发 OnRep
	DOREPLIFETIME_CONDITION_NOTIFY(AG_Character, AimPitch, COND_None, REPNOTIFY_Always);
}

void AG_Character::InitializeGAS()
{
	if (!AbilitySystemComponent) return;

	// 注意：AttributeSet 已在 InitAbilityActorInfo 中创建，此处不再创建

	for (TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (!AbilityClass) continue;
		FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
		AbilitySystemComponent->GiveAbility(Spec);
	}

	if (DefaultInitialEffect)
	{
		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		Context.AddSourceObject(this);
		FGameplayEffectSpecHandle GESpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultInitialEffect, 1, Context);
		if (GESpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*GESpecHandle.Data.Get());
		}
	}
}

// ========== 装备功能 ==========
void AG_Character::EquipWeapon(const FItemData& ItemData)
{
	UE_LOG(LogTemp, Warning, TEXT("[Server] EquipWeapon called, ItemName: %s, bCanEquip: %d, Class: %s"),
		*ItemData.ItemName,
		ItemData.bCanEquip,
		ItemData.ItemClass ? *ItemData.ItemClass->GetName() : TEXT("NULL"));

	if (!HasAuthority()) return;
	if (!ItemData.bCanEquip) return;

	if (EquippedWeapon)
	{
		InternalUnequipWeapon();
	}

	if (!ItemData.ItemClass) 
	{
		UE_LOG(LogTemp, Error, TEXT("EquipWeapon: ItemClass is null!"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AG_Items* NewWeapon = World->SpawnActor<AG_Items>(ItemData.ItemClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!NewWeapon)
	{
		UE_LOG(LogTemp, Error, TEXT("EquipWeapon: Failed to spawn weapon actor!"));
		return;
	}

	NewWeapon->SetItemData(ItemData);

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (CharacterMesh)
	{
		NewWeapon->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
	}

	if (NewWeapon)
	{
		DrawDebugSphere(GetWorld(), NewWeapon->GetActorLocation(), 20.f, 12, FColor::Yellow, false, 10.0f);
	}
	
	EquippedWeapon = NewWeapon;
	bIsCombatMode = true;

	OnRep_EquippedWeapon();
	OnRep_IsCombatMode();   // ★ 主动触发蓝图事件，服务器也会执行
}

void AG_Character::InternalUnequipWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}
	bIsCombatMode = false;

	// 卸下武器时自动退出射击模式
	if (bIsShootingMode)
	{
		SetShootingMode(false);
	}

	OnRep_EquippedWeapon();
	OnRep_IsCombatMode();
}

void AG_Character::UnequipWeapon()
{
	if (!HasAuthority()) return;
	InternalUnequipWeapon();
}

void AG_Character::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && GetLocalRole() == ROLE_SimulatedProxy)
	{
		USkeletalMeshComponent* CharacterMesh = GetMesh();
		if (CharacterMesh)
		{
			EquippedWeapon->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
		}
	}
	// 当装备武器时更新玩家主界面UI（服务器和客户端均会触发）
	UpdateWeaponUI();
}

void AG_Character::OnRep_IsCombatMode()
{
	OnCombatModeChanged(bIsCombatMode);   // ★ 此事件在服务器和客户端都会触发
}

void AG_Character::SetShootingMode(bool bEnabled)
{
	if (bIsShootingMode == bEnabled) return;

	// 进入射击模式时检查是否装备武器
	if (bEnabled && EquippedWeapon == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetShootingMode函数: 无法进入射击模式，未装备武器"));
		return;
	}

	// 原有逻辑
	if (HasAuthority())
	{
		bIsShootingMode = bEnabled;
		OnRep_ShootingMode();
	}
	else
	{
		ServerSetShootingMode(bEnabled);
	}
}

void AG_Character::ServerSetShootingMode_Implementation(bool bEnabled)
{
	SetShootingMode(bEnabled);
}

// 当射击模式改变时调用
void AG_Character::OnRep_ShootingMode()
{
	// 更新移动组件和控制器旋转
	if (bIsShootingMode)
	{
		// 关闭角色朝向移动方向旋转
		GetCharacterMovement()->bOrientRotationToMovement = false;
		// 设置角色速度上限为 300
		GetCharacterMovement()->MaxWalkSpeed = 300.f;
		// 允许控制器控制角色Yaw旋转（使角色面朝摄像机方向）
		bUseControllerRotationYaw = true;
		// 创建并显示准星UI
		if (CrosshairWidgetClass && !CrosshairWidgetInstance)
		{
			CrosshairWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), CrosshairWidgetClass);
			if (CrosshairWidgetInstance)
			{
				CrosshairWidgetInstance->AddToViewport();
			}
		}
		// 同时设置摄像机相对位置（越肩视角）和视场（FOV）
		CameraComp->SetRelativeLocation(FVector(0.f, 100.f, 80.f));
		CameraComp->SetRelativeRotation(FRotator(0.f, -5.f, 0.f));
		CameraComp->SetFieldOfView(80.f);
	}
	else
	{
		// 恢复移动旋转
		GetCharacterMovement()->bOrientRotationToMovement = true;
		bUseControllerRotationYaw = false;
		// 恢复角色速度上限为 600
		GetCharacterMovement()->MaxWalkSpeed = 600.f;
		// 移除准星UI
		if (CrosshairWidgetInstance)
		{
			CrosshairWidgetInstance->RemoveFromParent();
			CrosshairWidgetInstance = nullptr;
		}
		// 恢复默认的摄像机设置
		CameraComp->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
		CameraComp->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
		CameraComp->SetFieldOfView(90.f);
	}

	// 可选：通知蓝图（如播放切换动画）
	OnShootingModeChanged(bIsShootingMode);   // ★ 此事件在服务器和客户端都会触发
}

// ========== 更新瞄准角度（客户端计算） ==========
void AG_Character::UpdateAimPitch()
{
	// 仅在射击模式下更新
	if (!bIsShootingMode)
	{
		// 如果退出射击模式，将 AimPitch 归零
		if (AimPitch != 0.0f)
		{
			AimPitch = 0.0f;
			// ★ 新增：如果当前是本地控制的客户端，通知服务器归零
			if (IsLocallyControlled() && !HasAuthority())
			{
				ServerSetAimPitch(0.0f);
			}
		}
		return;
	}

	// 获取摄像机旋转的 Pitch
	if (CameraComp)
	{
		FRotator CamRot = CameraComp->GetComponentRotation();
		// 将 Pitch 范围限制在 -90 ~ 90 之间
		float NewPitch = FMath::Clamp(CamRot.Pitch, -90.0f, 90.0f);
		
		// ★ 修改：仅在值发生明显变化时更新并发送 RPC
		if (!FMath::IsNearlyEqual(AimPitch, NewPitch, 0.001f))
		{
			AimPitch = NewPitch;
			// ★ 新增：如果当前是本地控制的客户端且不是服务器，向服务器报告新值
			if (IsLocallyControlled() && !HasAuthority())
			{
				ServerSetAimPitch(NewPitch);
			}
		}
	}
}

// 新增：ServerSetAimPitch 实现
bool AG_Character::ServerSetAimPitch_Validate(float NewAimPitch)
{
	// 简单验证范围
	return FMath::IsWithin(NewAimPitch, -90.0f, 90.0f);
}

void AG_Character::ServerSetAimPitch_Implementation(float NewAimPitch)
{
	AimPitch = NewAimPitch;
	// 服务器上修改后，会自动触发 OnRep_AimPitch（如果有绑定）
}

// 新增：OnRep_AimPitch 回调（可用于触发动画更新等）
void AG_Character::OnRep_AimPitch()
{
	// 可以在这里调用蓝图事件，或者更新动画实例
	// 例如：如果你有动画蓝图，可以通过接口通知
	// 这里留空，但保留以便你后续扩展
	// 你也可以在此触发一个蓝图可实现事件
}

// ========== 开火逻辑 ==========
void AG_Character::FireWeapon()
{
	UE_LOG(LogTemp, Warning, TEXT("FireWeapon 被调用, 角色: %s, 拥有权威: %d"), *GetName(), HasAuthority());

	if (!HasAuthority())
	{
		// 客户端计算位置和方向
		FVector MuzzleLoc = GetMuzzleLocation();
		FVector FireDir = CalculateFireDirection();
		UE_LOG(LogTemp, Warning, TEXT("FireWeapon: 客户端计算位置 %s, 方向 %s"), *MuzzleLoc.ToString(), *FireDir.ToString());
		ServerFireWeapon(MuzzleLoc, FireDir);
	}
	else
	{
		// 如果是服务器自身调用（如AI），则自己计算（但可能不准，建议AI也通过客户端触发）
		FVector MuzzleLoc = GetMuzzleLocation();
		FVector FireDir = CalculateFireDirection();
		ServerFireWeapon_Implementation(MuzzleLoc, FireDir);
	}
}

// ★ RPC 验证（可添加距离、冷却等验证）
bool AG_Character::ServerFireWeapon_Validate(const FVector& MuzzleLocation, const FVector& FireDirection)
{
	// 简单验证：方向向量是否为单位向量
	if (!FireDirection.IsNormalized())
		return false;
	// 可验证位置距离是否合理（例如不超过某个范围）
	return true;
}

// 服务器实现：使用客户端提供的位置和方向生成子弹，并应用射速限制
void AG_Character::ServerFireWeapon_Implementation(const FVector& MuzzleLocation, const FVector& FireDirection)
{
	UE_LOG(LogTemp, Warning, TEXT("ServerFireWeapon_Implementation 开始执行"));

	if (!BulletClass)
	{
		UE_LOG(LogTemp, Error, TEXT("  失败：BulletClass 未设置"));
		return;
	}
	if (!EquippedWeapon)
	{
		UE_LOG(LogTemp, Error, TEXT("  失败：EquippedWeapon 为空，请检查装备流程"));
		return;
	}
	if (!bIsShootingMode)
	{
		UE_LOG(LogTemp, Error, TEXT("  失败：bIsShootingMode 为 false"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// ========== 射速限制 ==========
	float CurrentTime = World->GetTimeSeconds();
	float FireRate = 0.1f;
	if (AG_Weapons* Weapon = Cast<AG_Weapons>(EquippedWeapon))
	{
		FireRate = Weapon->FireRate;
	}
	if (CurrentTime - LastFireTime < FireRate)
	{
		UE_LOG(LogTemp, Warning, TEXT("  射速限制：距离上次开火 %.3f 秒，需要 %.3f 秒"), CurrentTime - LastFireTime, FireRate);
		return;
	}
	LastFireTime = CurrentTime;

	// ★ 新增：获取武器组件并检查弹药
	AG_Weapons* Weapon = Cast<AG_Weapons>(EquippedWeapon);
	if (!Weapon)
	{
		UE_LOG(LogTemp, Error, TEXT("  失败：装备的武器不是 AG_Weapons 类型"));
		return;
	}
	if (Weapon->bIsReloading)
	{
		UE_LOG(LogTemp, Warning, TEXT("  正在换弹，无法开火"));
		return;
	}
	if (Weapon->GetCurrentAmmo() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("  弹药不足，请换弹"));
		return;
	}

	// ★ 消耗一发弹药
	if (!Weapon->ConsumeAmmo(1))
	{
		UE_LOG(LogTemp, Warning, TEXT("  消耗弹药失败"));
		return;
	}

	// 直接使用传入的位置和方向生成子弹
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AG_Bullet* Bullet = World->SpawnActor<AG_Bullet>(BulletClass, MuzzleLocation, FireDirection.Rotation(), SpawnParams);
	if (Bullet)
	{
		// ★ 传递伤害值
		Bullet->InitBullet(FireDirection, BulletSpeed, 3.0f, Weapon->WeaponDamage);
		UE_LOG(LogTemp, Warning, TEXT("子弹已生成！位置: %s, 方向: %s, 伤害: %.2f"), *MuzzleLocation.ToString(), *FireDirection.ToString(), Weapon->WeaponDamage);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("生成子弹失败！请检查 BulletClass 是否有效"));
	}
}

FVector AG_Character::GetFireStartLocation() const
{
	// 从武器挂载点获取，若无则从角色眼睛位置
	if (EquippedWeapon)
	{
		// 假设武器有Socket或直接获取位置
		return EquippedWeapon->GetActorLocation();
	}
	// 否则从摄像机位置
	if (CameraComp)
	{
		return CameraComp->GetComponentLocation();
	}
	return GetActorLocation() + GetActorForwardVector() * 100.0f + FVector(0,0,50.0f);
}

FRotator AG_Character::GetFireRotation() const
{
	// 使用摄像机旋转（瞄准方向）
	if (CameraComp)
	{
		return CameraComp->GetComponentRotation();
	}
	return GetActorForwardVector().Rotation();
}

// 修改：从 Socket 获取枪口位置（原为查找组件，现改为使用武器骨骼上的 Socket）
FVector AG_Character::GetMuzzleLocation() const
{
	if (EquippedWeapon)
	{
		// 获取武器的 SkeletalMeshComponent（根组件）
		USkeletalMeshComponent* WeaponMesh = EquippedWeapon->GetItemMesh();
		if (WeaponMesh)
		{
			// 检查 Socket 是否存在
			if (WeaponMesh->DoesSocketExist(MuzzleSocketName))
			{
				FVector SocketLoc = WeaponMesh->GetSocketLocation(MuzzleSocketName);

				UE_LOG(LogTemp, Warning, TEXT("GetMuzzleLocation: 使用 Socket '%s' 位置 -> %s"), *MuzzleSocketName.ToString(), *SocketLoc.ToString());
				return SocketLoc;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("GetMuzzleLocation: Socket '%s' 不存在于武器上，回退到武器前方"), *MuzzleSocketName.ToString());
			}
		}
		// 回退：武器前方 100 单位
		return EquippedWeapon->GetActorLocation() + EquippedWeapon->GetActorForwardVector() * 100.0f;
	}

	// 无武器时的默认位置
	if (CameraComp)
	{
		return CameraComp->GetComponentLocation();
	}
	return GetActorLocation() + GetActorForwardVector() * 100.0f;
}

// 计算瞄准方向（基于摄像机射线）
FVector AG_Character::CalculateFireDirection() const
{
	if (!CameraComp || !EquippedWeapon)
	{
		// 若无摄像机或武器，默认朝前
		return GetActorForwardVector();
	}

	UWorld* World = GetWorld();
	if (!World)
		return GetActorForwardVector();

	FVector CameraLoc = CameraComp->GetComponentLocation();
	FRotator CameraRot = CameraComp->GetComponentRotation();
	FVector CameraForward = CameraRot.Vector();

	float TraceDistance = 10000.0f;
	FVector TraceEnd = CameraLoc + CameraForward * TraceDistance;

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(EquippedWeapon);

	bool bHit = World->LineTraceSingleByChannel(Hit, CameraLoc, TraceEnd, ECC_Visibility, QueryParams);

	FVector TargetPoint = bHit ? Hit.Location : TraceEnd;
	FVector MuzzleLoc = GetMuzzleLocation();

	FVector FireDir = (TargetPoint - MuzzleLoc).GetSafeNormal();

	// 如果方向与摄像机朝向相反，则强制使用摄像机方向
	if (FVector::DotProduct(FireDir, CameraForward) < 0.0f)
	{
		FireDir = CameraForward;
	}

	return FireDir;
}

void AG_Character::UpdateWeaponUI()
{
	APlayerController* PC = GetController<APlayerController>();
	if (!PC) return;
	AG_PlayerController* GPC = Cast<AG_PlayerController>(PC);
	if (!GPC) return;
	UCharacterUIWidget* UI = GPC->CharacterUIWidget;
	if (!UI) return;

	if (EquippedWeapon)
    {
        FItemData Data = EquippedWeapon->GetItemData();
        int32 CurrentAmmo = 0;
        int32 ReserveAmmo = 0;

        if (AG_Weapons* Weapon = Cast<AG_Weapons>(EquippedWeapon))
        {
            CurrentAmmo = Weapon->GetCurrentAmmo();
            UComp_Inventory* Inventory = FindComponentByClass<UComp_Inventory>();
            if (Inventory && Weapon->AmmoItemID != NAME_None)
            {
                ReserveAmmo = Inventory->GetItemTotalQuantity(Weapon->AmmoItemID);
                UE_LOG(LogTemp, Log, TEXT("UpdateWeaponUI: 备弹 = %d"), ReserveAmmo); // 调试日志
            }
        }
        UI->UpdateWeaponInfo(Data, CurrentAmmo, ReserveAmmo);
    }
    else
    {if (EquippedWeapon)
    {
    	FItemData Data = EquippedWeapon->GetItemData();
    	int32 CurrentAmmo = 0;
    	int32 ReserveAmmo = 0;

    	if (AG_Weapons* Weapon = Cast<AG_Weapons>(EquippedWeapon))
    	{
    		CurrentAmmo = Weapon->GetCurrentAmmo();
    		UComp_Inventory* Inventory = FindComponentByClass<UComp_Inventory>();
    		if (Inventory && Weapon->AmmoItemID != NAME_None)
    		{
    			ReserveAmmo = Inventory->GetItemTotalQuantity(Weapon->AmmoItemID);
    			UE_LOG(LogTemp, Log, TEXT("UpdateWeaponUI: 备弹 = %d"), ReserveAmmo); // 调试日志
    		}
    	}
    	UI->UpdateWeaponInfo(Data, CurrentAmmo, ReserveAmmo);
    }
    else
    {
    	UI->UpdateWeaponInfo(FItemData(), 0, 0);
    }
        UI->UpdateWeaponInfo(FItemData(), 0, 0);
    }
}
