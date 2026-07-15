// G_Bullet.cpp

#include "BP/G_Bullet.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Character/G_Character.h"          // 用于伤害目标
#include "GAS/AS_Player.h"                 // 用于修改属性
#include "Kismet/GameplayStatics.h"        // 备用
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

AG_Bullet::AG_Bullet()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
	MeshComp->SetCollisionProfileName(FName("BlockAllDynamic"));
	MeshComp->SetSimulatePhysics(false);
	// ★ 启用碰撞事件
	MeshComp->SetNotifyRigidBodyCollision(true);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2000.0f;
	ProjectileMovement->MaxSpeed = 2000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

void AG_Bullet::BeginPlay()
{
	Super::BeginPlay();

	// ★ 绑定碰撞事件
	if (MeshComp)
	{
		MeshComp->OnComponentHit.AddDynamic(this, &AG_Bullet::OnBulletHit);
	}
}

void AG_Bullet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AG_Bullet::InitBullet(const FVector& Direction, float Speed, float LifeSpan, float InDamage)
{
	ProjectileMovement->Velocity = Direction * Speed;
	SetLifeSpan(LifeSpan);
	Damage = InDamage;  // 设置伤害
}

// 碰撞命中处理
void AG_Bullet::OnBulletHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 只在服务器处理伤害
	if (!HasAuthority()) return;

	// 如果已经销毁则忽略
	if (!IsValid(this)) return;

	// 检测是否为角色
	AG_Character* TargetChar = Cast<AG_Character>(OtherActor);
	if (!TargetChar) return;

	// 获取目标的属性集
	UAS_Player* AttrSet = TargetChar->AttributeSet;
	if (!AttrSet) return;

	// 应用伤害（直接修改生命值，可在客户端通过GAS复制看到变化）
	float CurrentHealth = AttrSet->GetHealth();
	float NewHealth = FMath::Max(0.0f, CurrentHealth - Damage);
	AttrSet->SetHealth(NewHealth);

	// ★ 可选：发送伤害事件（蓝图可监听）
	// TargetChar->OnTakeDamage(Damage); // 如有蓝图事件

	// 销毁子弹（防止多次触发）
	Destroy();
}