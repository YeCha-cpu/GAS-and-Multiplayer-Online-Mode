// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/G_Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystemComponent.h"
#include "Core/G_PlayerState.h"
#include "GameplayEffect.h"
#include "Component/Comp_Inventory.h"
#include "Engine/Engine.h"
#include "GAS/AS_Player.h"
#include "Net/UnrealNetwork.h"
#include "Item/G_Items.h"
#include "Components/SkeletalMeshComponent.h"

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

	EquippedWeapon = NewWeapon;
	bIsCombatMode = true;

	OnRep_EquippedWeapon();
	OnRep_IsCombatMode();
}

void AG_Character::InternalUnequipWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}
	bIsCombatMode = false;
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
}

void AG_Character::OnRep_IsCombatMode()
{
	OnCombatModeChanged(bIsCombatMode);
}