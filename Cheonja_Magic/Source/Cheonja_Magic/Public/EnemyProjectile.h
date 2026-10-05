#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

// 몬스터 투사체. 직선 비행, 플레이어 머리 근처를 지나면 피격, 벽에 닿으면 소멸.
UCLASS()
class CHEONJA_MAGIC_API AEnemyProjectile : public AActor
{
	GENERATED_BODY()

public:
	AEnemyProjectile();

	virtual void Tick(float DeltaTime) override;

	void InitProjectile(float InDamage, AActor* InTargetActor, USceneComponent* InTargetPoint);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> Collision;

	// 속도는 BP에서 이 컴포넌트의 Initial Speed / Max Speed로 조절.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float HitRadius = 40.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	float Damage = 10.f;   // 쏜 몬스터의 AttackDamage가 들어옴

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleStop(const FHitResult& ImpactResult);

	UPROPERTY()
	TObjectPtr<AActor> TargetActor;

	UPROPERTY()
	TObjectPtr<USceneComponent> TargetPoint;
};
