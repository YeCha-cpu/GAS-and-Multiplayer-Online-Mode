// G_Bullet.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "G_Bullet.generated.h"

UCLASS()
class GG_API AG_Bullet : public AActor
{
	GENERATED_BODY()

public:
	AG_Bullet();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ★ 修改：初始化子弹（方向和速度、生命周期、伤害）
	void InitBullet(const FVector& Direction, float Speed = 2000.0f, float LifeSpan = 3.0f, float InDamage = 10.0f);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UProjectileMovementComponent* ProjectileMovement;

	// ★ 新增：子弹伤害值（服务器权威，不复制，客户端无需知晓）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bullet")
	float Damage = 10.0f;

	// ★ 新增：命中处理（绑定碰撞事件）
	UFUNCTION()
	void OnBulletHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};