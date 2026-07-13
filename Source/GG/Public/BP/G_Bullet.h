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

	// 初始化子弹（设置方向和速度）
	void InitBullet(const FVector& Direction, float Speed = 2000.0f, float LifeSpan = 3.0f);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UProjectileMovementComponent* ProjectileMovement;
};