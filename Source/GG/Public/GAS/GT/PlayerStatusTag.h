// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "NativeGameplayTags.h"

/**
 * 使用嵌套命名空间组织 GameplayTag 常量
 * 分类说明：
 * - PlayerStatusTag::Status  : 角色状态/效果类标签（如 Buff、Debuff）
 * - PlayerStatusTag::Data    : 用于 SetByCaller 或传递数值的标签
 * - 可根据需要继续添加其他分类，如 PlayerStatusTag::Ability、PlayerStatusTag::Item 等
 */
namespace PlayerStatusTag
{
	namespace Status
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(PainKiller);
		
	}
	
}