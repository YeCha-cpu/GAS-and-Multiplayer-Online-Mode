// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/Medicines/G_Medicines.h"
#include "Character/G_Character.h"
#include "GAS/AS_Player.h"

void AG_Medicines::OnUsed_Implementation(APawn* User, const FItemData& InItemData)
{
	Super::OnUsed_Implementation(User, InItemData);

	if (!HasAuthority()) return;

	AG_Character* Char = Cast<AG_Character>(User);
	if (Char && Char->AttributeSet)
	{
		float HealAmount = InItemData.ItemUseValue;
		float NewHealth = FMath::Min(Char->AttributeSet->GetHealth() + HealAmount, Char->AttributeSet->GetMaxHealth());
		Char->AttributeSet->SetHealth(NewHealth);
		UE_LOG(LogTemp, Log, TEXT("使用药品：%s，恢复 %.2f 生命值"), *InItemData.ItemName, HealAmount);
	}
}

