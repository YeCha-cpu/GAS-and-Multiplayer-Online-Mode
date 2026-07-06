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


AG_Character::AG_Character()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	
	// 玩家角色移动设置
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 400.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	
	// 禁用控制器旋转
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	// 设置弹簧臂组件
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->TargetArmLength = 300.f;
	SpringArmComp->bUsePawnControlRotation = true;
	
	// 设置相机组件
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);


}

void AG_Character::BeginPlay()
{
	Super::BeginPlay();
	
}

void AG_Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AG_Character::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilityActorInfo();      // 绑定 ASC 的 Owner 和 Avatar
	if (HasAuthority())          // 确保只在服务器执行
	{
		InitializeGAS();         // 创建属性集、赋予技能、应用初始效果
	}
}

void AG_Character::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitAbilityActorInfo();      // 客户端仅需重新绑定 ASC 的 Owner 和 Avatar
}

void AG_Character::InitAbilityActorInfo()
{
	AG_PlayerState* G_PlayerState = GetPlayerState<AG_PlayerState>();
	if (!G_PlayerState) return;

	UAbilitySystemComponent* ASC = G_PlayerState->GetAbilitySystemComponent();
	if (!ASC) return;
 
	ASC->InitAbilityActorInfo(G_PlayerState, this);
	AbilitySystemComponent = ASC;
}

void AG_Character::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AG_Character::InitializeGAS()
{
	if (!AbilitySystemComponent) return;

	if (!AttributeSet)
	{
		AttributeSet = NewObject<UAS_Player>(this);
		AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet);
	}

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
