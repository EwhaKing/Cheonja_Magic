#include "EnemyMelee.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

AEnemyMelee::AEnemyMelee()
{
	MaxHealth = 45.f;
	AttackRange = 150.f;
	AttackDamage = 10.f;
}

void AEnemyMelee::PerformAttack()
{
	// SetTimer는 시간이 0 이하면 타이머를 걸지 않으므로 최솟값 보정.
	const float Delay = FMath::Max(HitDelay, 0.01f);
	const float Duration = FMath::Max(AttackDuration, Delay + 0.01f);
	GetWorldTimerManager().SetTimer(HitTimer, this, &AEnemyMelee::DoHitCheck, Delay, false);
	GetWorldTimerManager().SetTimer(EndTimer, this, &AEnemyMelee::FinishAttack, Duration, false);
}

void AEnemyMelee::DoHitCheck()
{
	if (State != EEnemyState::Attacking || !IsValid(PlayerPawn))
	{
		return;  // 그사이 죽었거나 플레이어가 없음
	}
	if (!IsPlayerInRange(AttackRange * 1.2f))
	{
		return;  // 피했음
	}
	FVector ToTarget = GetPlayerTargetLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	const float CosLimit = FMath::Cos(FMath::DegreesToRadians(HitAngleDegrees));
	if (FVector::DotProduct(GetActorForwardVector(), ToTarget.GetSafeNormal()) < CosLimit)
	{
		return;  // 등 뒤/옆으로 빠졌음
	}
	UGameplayStatics::ApplyDamage(PlayerPawn, AttackDamage, GetController(), this, UDamageType::StaticClass());
}
