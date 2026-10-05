#pragma once

#include "CoreMinimal.h"
#include "EnemyBase.h"
#include "EnemyBoss.generated.h"

UENUM(BlueprintType)
enum class EBossPhase : uint8
{
	Phase1 UMETA(DisplayName = "1페이즈"),
	Phase2 UMETA(DisplayName = "2페이즈")
};

// 보스: 기획 확정 전 뼈대. 체력 비율로 페이즈 전환만 구현. 공격은 기획 확정 후 추가.
UCLASS()
class CHEONJA_MAGIC_API AEnemyBoss : public AEnemyBase
{
	GENERATED_BODY()

public:
	AEnemyBoss();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	                         class AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|보스")
	float Phase2HealthRatio = 0.5f;   // 체력이 이 비율 이하가 되면 2페이즈

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|보스")
	EBossPhase Phase = EBossPhase::Phase1;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|보스")
	void OnPhaseChanged(EBossPhase NewPhase);
};
