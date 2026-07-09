// CharacterUIWidget.cpp
#include "UI/CharacterUIWidget.h"
#include "AbilitySystemComponent.h"
#include "GAS/AS_Player.h"

void UCharacterUIWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 如果已有 ASC，自动绑定（通常由外部的 PlayerController 或 Pawn 设置）
    if (AbilitySystemComponent.IsValid())
    {
        BindToAbilitySystem(AbilitySystemComponent.Get());
    }
}

void UCharacterUIWidget::NativeDestruct()
{
    // 解绑所有委托
    if (AbilitySystemComponent.IsValid())
    {
        UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
        ASC->GetGameplayAttributeValueChangeDelegate(UAS_Player::GetHealthAttribute()).Remove(HealthChangedHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(UAS_Player::GetMaxHealthAttribute()).Remove(MaxHealthChangedHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(UAS_Player::GetStaminaAttribute()).Remove(StaminaChangedHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(UAS_Player::GetMaxStaminaAttribute()).Remove(MaxStaminaChangedHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(UAS_Player::GetDefenseAttribute()).Remove(DefenseChangedHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(UAS_Player::GetMaxDefenseAttribute()).Remove(MaxDefenseChangedHandle);
    }

    Super::NativeDestruct();
}

void UCharacterUIWidget::BindToAbilitySystem(UAbilitySystemComponent* InASC)
{
    if (!InASC)
        return;

    // 如果已绑定其他 ASC，先解绑
    if (AbilitySystemComponent.IsValid() && AbilitySystemComponent.Get() != InASC)
    {
        NativeDestruct(); // 清理旧绑定
    }

    AbilitySystemComponent = InASC;

    // 绑定各属性变化委托的回调函数
    HealthChangedHandle = InASC->GetGameplayAttributeValueChangeDelegate(UAS_Player::GetHealthAttribute())
        .AddUObject(this, &UCharacterUIWidget::OnHealthAttributeChanged);
    MaxHealthChangedHandle = InASC->GetGameplayAttributeValueChangeDelegate(UAS_Player::GetMaxHealthAttribute())
        .AddUObject(this, &UCharacterUIWidget::OnMaxHealthAttributeChanged);
    StaminaChangedHandle = InASC->GetGameplayAttributeValueChangeDelegate(UAS_Player::GetStaminaAttribute())
        .AddUObject(this, &UCharacterUIWidget::OnStaminaAttributeChanged);
    MaxStaminaChangedHandle = InASC->GetGameplayAttributeValueChangeDelegate(UAS_Player::GetMaxStaminaAttribute())
        .AddUObject(this, &UCharacterUIWidget::OnMaxStaminaAttributeChanged);
    DefenseChangedHandle = InASC->GetGameplayAttributeValueChangeDelegate(UAS_Player::GetDefenseAttribute())
        .AddUObject(this, &UCharacterUIWidget::OnDefenseAttributeChanged);
    MaxDefenseChangedHandle = InASC->GetGameplayAttributeValueChangeDelegate(UAS_Player::GetMaxDefenseAttribute())
        .AddUObject(this, &UCharacterUIWidget::OnMaxDefenseAttributeChanged);

    // 初次刷新所有值
    RefreshAllValues();
}

void UCharacterUIWidget::RefreshAllValues()
{
    UAS_Player* AS = GetAttributeSet();
    if (!AS) return;

    OnHealthChanged(AS->GetHealth(), AS->GetMaxHealth());
    OnStaminaChanged(AS->GetStamina(), AS->GetMaxStamina());
    OnDefenseChanged(AS->GetDefense(), AS->GetMaxDefense());
}

UAS_Player* UCharacterUIWidget::GetAttributeSet() const
{
    if (!AbilitySystemComponent.IsValid())
        return nullptr;
    // 通过 ASC 获取属性集（需确保 AS_Player 已添加到 ASC）
    TArray<UAttributeSet*> Sets = AbilitySystemComponent->GetSpawnedAttributes();
    for (UAttributeSet* Set : Sets)
    {
        if (UAS_Player* AS = Cast<UAS_Player>(Set))
            return AS;
    }
    return nullptr;
}

// -------- 属性变化回调 --------
void UCharacterUIWidget::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
    UAS_Player* AS = GetAttributeSet();
    if (!AS) return;
    OnHealthChanged(Data.NewValue, AS->GetMaxHealth());
}

void UCharacterUIWidget::OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
    UAS_Player* AS = GetAttributeSet();
    if (!AS) return;
    OnHealthChanged(AS->GetHealth(), Data.NewValue);
}

void UCharacterUIWidget::OnStaminaAttributeChanged(const FOnAttributeChangeData& Data)
{
    UAS_Player* AS = GetAttributeSet();
    if (!AS) return;
    OnStaminaChanged(Data.NewValue, AS->GetMaxStamina());
}

void UCharacterUIWidget::OnMaxStaminaAttributeChanged(const FOnAttributeChangeData& Data)
{
    UAS_Player* AS = GetAttributeSet();
    if (!AS) return;
    OnStaminaChanged(AS->GetStamina(), Data.NewValue);
}

void UCharacterUIWidget::OnDefenseAttributeChanged(const FOnAttributeChangeData& Data)
{
    UAS_Player* AS = GetAttributeSet();
    if (!AS) return;
    OnDefenseChanged(Data.NewValue, AS->GetMaxDefense());
}

void UCharacterUIWidget::OnMaxDefenseAttributeChanged(const FOnAttributeChangeData& Data)
{
    UAS_Player* AS = GetAttributeSet();
    if (!AS) return;
    OnDefenseChanged(AS->GetDefense(), Data.NewValue);
}