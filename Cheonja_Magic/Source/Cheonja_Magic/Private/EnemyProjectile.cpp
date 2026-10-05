#include "EnemyProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

AEnemyProjectile::AEnemyProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(15.f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));          // 벽에 막힘
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);      // 몬스터 몸은 통과
	RootComponent = Collision;

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = 1200.f;
	Movement->MaxSpeed = 1200.f;
	Movement->ProjectileGravityScale = 0.f;   // 직선 비행
	Movement->bRotationFollowsVelocity = true;

	InitialLifeSpan = 5.f;  // 아무것도 못 맞혀도 5초 뒤 소멸
}

void AEnemyProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* MyOwner = GetOwner())
	{
		Collision->IgnoreActorWhenMoving(MyOwner, true);
	}
	Movement->OnProjectileStop.AddDynamic(this, &AEnemyProjectile::HandleStop);
}

void AEnemyProjectile::InitProjectile(float InDamage, AActor* InTargetActor, USceneComponent* InTargetPoint)
{
	Damage = InDamage;
	TargetActor = InTargetActor;
	TargetPoint = InTargetPoint;
}

void AEnemyProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsValid(TargetActor) || !IsValid(TargetPoint))
	{
		return;
	}
	// VR 폰은 충돌체가 없는 경우가 많아, 충돌 대신 머리와의 거리로 판정.
	if (FVector::DistSquared(GetActorLocation(), TargetPoint->GetComponentLocation()) <= FMath::Square(HitRadius))
	{
		UGameplayStatics::ApplyDamage(TargetActor, Damage, GetInstigatorController(), this, UDamageType::StaticClass());
		Destroy();
	}
}

void AEnemyProjectile::HandleStop(const FHitResult& ImpactResult)
{
	Destroy();
}
