#include "EnemyBoss.h"

AEnemyBoss::AEnemyBoss()
{
	MaxHealth = 150.f;   // 임시값
	AttackRange = 250.f;
	AttackDamage = 15.f;
}

float AEnemyBoss::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                             AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (Phase == EBossPhase::Phase1 && Health > 0.f && Health <= MaxHealth * Phase2HealthRatio)
	{
		Phase = EBossPhase::Phase2;
		OnPhaseChanged(Phase);
	}
	return Applied;
}
