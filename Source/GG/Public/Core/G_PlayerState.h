// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "G_PlayerState.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
// 添加 UAS_Player 前置声明（或包含头文件，此处用前置声明）
class UAS_Player;

UCLASS()
class GG_API AG_PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	AG_PlayerState();
	
	// 获取能力系统组件的接口
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	// 获取属性集的函数（返回 UAS_Player*）
	FORCEINLINE UAS_Player* GetAttributeSet() const { return AttributeSet; }
	
protected:
	// 能力系统组件 和 属性集 对象引用
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	UPROPERTY()
	TObjectPtr<UAS_Player> AttributeSet;   // 类型改为 UAS_Player
};