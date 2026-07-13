// G_Bullet.cpp

#include "BP/G_Bullet.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AG_Bullet::AG_Bullet()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
	MeshComp->SetCollisionProfileName(FName("BlockAllDynamic"));
	MeshComp->SetSimulatePhysics(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2000.0f;
	ProjectileMovement->MaxSpeed = 2000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

void AG_Bullet::BeginPlay()
{
	Super::BeginPlay();
}

void AG_Bullet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AG_Bullet::InitBullet(const FVector& Direction, float Speed, float LifeSpan)
{
	ProjectileMovement->Velocity = Direction * Speed;
	SetLifeSpan(LifeSpan);
}
