// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AS_Player.generated.h"

// get、set、init属性的宏定义
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class GG_API UAS_Player : public UAttributeSet
{
	GENERATED_BODY()
	public:
	UAS_Player();
	
	// 网络复制属性
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// 属性值变化前调用，用于限制属性值的范围
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	// 属性值变化后调用，用于执行属性值变化后的效果
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
	/*---------------------------------------------网络复制的属性声明---------------------------------------------*/
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, DisplayName = "生命值")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UAS_Player, Health);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, DisplayName = "最大生命值")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UAS_Player, MaxHealth);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Defense, DisplayName = "防御值")
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS(UAS_Player, Defense);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxDefense, DisplayName = "最大防御值")
	FGameplayAttributeData MaxDefense;
	ATTRIBUTE_ACCESSORS(UAS_Player, MaxDefense);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, DisplayName = "体力值")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UAS_Player, Stamina);
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, DisplayName = "最大体力值")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UAS_Player, MaxStamina);
	
	/*---------------------------------------------网络复制的函数声明---------------------------------------------*/
	
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth) const;
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const;
	UFUNCTION()
	void OnRep_Defense(const FGameplayAttributeData& OldDefense) const;
	UFUNCTION()
	void OnRep_MaxDefense(const FGameplayAttributeData& OldMaxDefense) const;
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldMaxStamina) const;
	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxDefense) const;
	
};
