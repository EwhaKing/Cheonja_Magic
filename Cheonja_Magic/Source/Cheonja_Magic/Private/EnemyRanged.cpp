#include "EnemyRanged.h"
#include "EnemyProjectile.h"
#include "Engine/World.h"

AEnemyRanged::AEnemyRanged()
{
	MaxHealth = 15.f;
	DetectRange = 750.f;               // 기획서: 다른 몹의 1.5배
	AttackRange = 700.f;               // 멀리서 멈춰서 쏨
	AttackDamage = 10.f;
	bFlying = true;
	bApproachDuringCooldown = false;
}

void AEnemyRanged::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (State == EEnemyState::Dead || BobAmplitude <= 0.f)
	{
		return;
	}
	// 사인파 위치의 "이번 프레임 변화량"만큼만 이동 → 누적 오차 없이 둥둥.
	const float W = UE_TWO_PI * BobFrequency;
	const float Prev = FMath::Sin(BobTime * W);
	BobTime += DeltaTime;
	const float Now = FMath::Sin(BobTime * W);
	AddActorWorldOffset(FVector(0.f, 0.f, BobAmplitude * (Now - Prev)));
}

void AEnemyRanged::PerformAttack()
{
	if (ProjectileClass && IsValid(PlayerPawn))
	{
		const FVector Muzzle = GetActorLocation() + GetActorForwardVector() * MuzzleForwardOffset;
		const FRotator Aim = (GetPlayerTargetLocation() - Muzzle).Rotation();

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AEnemyProjectile* Proj = GetWorld()->SpawnActor<AEnemyProjectile>(ProjectileClass, Muzzle, Aim, Params))
		{
			USceneComponent* TargetPoint = IsValid(PlayerHead) ? PlayerHead.Get() : PlayerPawn->GetRootComponent();
			Proj->InitProjectile(AttackDamage, PlayerPawn, TargetPoint);
		}
	}
	FinishAttack();  // 발사는 즉발
}
