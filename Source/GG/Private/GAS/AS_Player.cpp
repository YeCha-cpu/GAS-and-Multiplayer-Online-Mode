// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/AS_Player.h"
#include "GameplayEffectExtension.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

UAS_Player::UAS_Player()
{
	// 初始化属性值
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitDefense(5.f);
	InitMaxDefense(100.f);
	InitStamina(100.f);
	InitMaxStamina(100.f);
}

/*---------------------------------------------网络复制回调属性实现---------------------------------------------*/

void UAS_Player::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 此宏设置 属性 在网络上的复制行为。COND_None:无条件复制，REPNOTIFY_Always:每次值变化时都触发通知回调 
	DOREPLIFETIME_CONDITION_NOTIFY(UAS_Player, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAS_Player, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAS_Player, Defense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAS_Player, MaxDefense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAS_Player, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAS_Player, MaxStamina, COND_None, REPNOTIFY_Always);
}

/*---------------------------------------------网络复制回调函数实现---------------------------------------------*/

void UAS_Player::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	// 该宏负责：通知 AbilitySystemComponent 该属性已更新，触发相关联的 GameplayEffect 重新评估。并自动更新 FGameplayAttributeData 的内部缓存
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAS_Player, Health, OldHealth);
}
void UAS_Player::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAS_Player, MaxHealth, OldMaxHealth);
}
void UAS_Player::OnRep_Defense(const FGameplayAttributeData& OldDefense) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAS_Player, Defense, OldDefense);
}
void UAS_Player::OnRep_MaxDefense(const FGameplayAttributeData& OldMaxDefense) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAS_Player, MaxDefense, OldMaxDefense)
}
void UAS_Player::OnRep_Stamina(const FGameplayAttributeData& OldStamina) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAS_Player, Stamina, OldStamina);
}
void UAS_Player::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAS_Player, MaxStamina, OldMaxStamina);
}

// 无论是对AS直接赋值，还是用GE修改AS，都会在属性变化前调用此函数(可用以限制【基础值】变化范围)
void UAS_Player::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	if (Attribute == GetDefenseAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxDefense());
	}
	if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
}

// 属性值变化后调用
void UAS_Player::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	// Data是 GAS 传入的完整回调上下文，包含了「哪个 GameplayEffect、修改了哪个属性、修改了多少数值、谁是施法者、谁是目标」等全部信息。
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Black,
				 FString::Printf(TEXT("通过GetHealth()获取生命值: %f"), GetHealth()));
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Black,
				 FString::Printf(TEXT("健康量级: %f"), Data.EvaluatedData.Magnitude));
			
		}
	}
	
}


