#pragma once

#include "CoreMinimal.h"
#include "EnemyBase.h"
#include "EnemyRanged.generated.h"

class AEnemyProjectile;

// 원거리 몬스터(덤보문어): 둥둥 떠서 입으로 투사체 발사.
UCLASS()
class CHEONJA_MAGIC_API AEnemyRanged : public AEnemyBase
{
	GENERATED_BODY()

public:
	AEnemyRanged();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|원거리")
	TSubclassOf<AEnemyProjectile> ProjectileClass;   // BP에서 BP_EnemyProjectile 지정

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|원거리")
	float MuzzleForwardOffset = 60.f;  // 몸 중심에서 발사 위치까지 앞쪽 거리

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|원거리")
	float BobAmplitude = 15.f;         // 위아래 흔들림 폭(cm)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|원거리")
	float BobFrequency = 1.f;          // 초당 왕복 횟수

protected:
	virtual void PerformAttack() override;

	float BobTime = 0.f;
};
