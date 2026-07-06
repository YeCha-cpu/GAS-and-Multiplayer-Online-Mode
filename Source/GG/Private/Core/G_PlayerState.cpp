// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/G_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"

AG_PlayerState::AG_PlayerState()
{
	SetNetUpdateFrequency(100.f);
	
	// 创建技能系统组件
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);// 确保与网络同步
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);// 设置 玩家ASC 的复制模式为 Mixed，主要针对GE
	
	// 创建属性集
	AttributeSet = CreateDefaultSubobject<UAttributeSet>("Attributes");
}

UAbilitySystemComponent* AG_PlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
