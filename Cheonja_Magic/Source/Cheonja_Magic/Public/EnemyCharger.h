#pragma once

#include "CoreMinimal.h"
#include "EnemyBase.h"
#include "EnemyCharger.generated.h"

// 돌진 몬스터(여섯줄아가미가오리): 회전하며 플레이어를 향해 직선 돌진.
UCLASS()
class CHEONJA_MAGIC_API AEnemyCharger : public AEnemyBase
{
	GENERATED_BODY()

public:
	AEnemyCharger();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|돌진")
	float DashSpeed = 1800.f;     // cm/s

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|돌진")
	float DashDuration = 0.6f;    // 돌진 지속 시간(초)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|돌진")
	float HitRadius = 80.f;       // 머리와 이 거리 안을 지나가면 피격

	// 암시·돌진 중 몸통 회전 속도(도/초). 공격 애니메이션에 회전이 이미 들어 있으면 0으로.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|돌진")
	float SpinSpeed = 720.f;

protected:
	virtual void BeginPlay() override;
	virtual void PerformAttack() override;

	FVector DashDir = FVector::ForwardVector;
	bool bHitThisDash = false;
	FRotator MeshBaseRotation = FRotator::ZeroRotator;
};
