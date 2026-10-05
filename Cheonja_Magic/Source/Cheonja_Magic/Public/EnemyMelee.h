#pragma once

#include "CoreMinimal.h"
#include "EnemyBase.h"
#include "EnemyMelee.generated.h"

// 근접 몬스터(쇄아): 다가와서 전방 부채꼴 할퀴기.
UCLASS()
class CHEONJA_MAGIC_API AEnemyMelee : public AEnemyBase
{
	GENERATED_BODY()

public:
	AEnemyMelee();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|근접")
	float HitDelay = 0.2f;          // 공격 시작 → 실제 판정까지(할퀴는 모션 타이밍)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|근접")
	float AttackDuration = 0.6f;    // 공격 상태 전체 길이(이후 쿨타임)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|근접")
	float HitAngleDegrees = 60.f;   // 전방 판정 부채꼴의 반각

protected:
	virtual void PerformAttack() override;
	void DoHitCheck();

	FTimerHandle HitTimer;
	FTimerHandle EndTimer;
};
