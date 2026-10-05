#include "EnemyCharger.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyCharger::AEnemyCharger()
{
	MaxHealth = 30.f;
	AttackRange = 600.f;   // 돌진형이라 멀리서 시작
	AttackDamage = 10.f;
	bFlying = true;
}

void AEnemyCharger::BeginPlay()
{
	Super::BeginPlay();
	if (GetMesh())
	{
		MeshBaseRotation = GetMesh()->GetRelativeRotation();  // 회전 연출 후 원위치용
	}
}

void AEnemyCharger::PerformAttack()
{
	bHitThisDash = false;
	DashDir = (GetPlayerTargetLocation() - GetActorLocation()).GetSafeNormal();
	if (DashDir.IsNearlyZero())
	{
		DashDir = GetActorForwardVector();
	}
	SetActorRotation(FRotator(0.f, DashDir.Rotation().Yaw, 0.f));  // 방향은 시작 순간 고정(피할 여지)
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}
}

void AEnemyCharger::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 회전 연출: 암시·돌진 중엔 진행 방향 축으로 빙글빙글, 그 외엔 원래 자세로.
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		const bool bSpinning = (State == EEnemyState::Windup || State == EEnemyState::Attacking);
		if (bSpinning && SpinSpeed != 0.f)
		{
			const FQuat Spin(GetActorForwardVector(), FMath::DegreesToRadians(SpinSpeed * DeltaTime));
			MeshComp->AddWorldRotation(Spin);
		}
		else if (!MeshComp->GetRelativeRotation().Equals(MeshBaseRotation))
		{
			MeshComp->SetRelativeRotation(MeshBaseRotation);
		}
	}

	if (State != EEnemyState::Attacking)
	{
		return;
	}

	// 돌진 이동. sweep=true: 벽에 부딪히면 멈춤.
	FHitResult Hit;
	AddActorWorldOffset(DashDir * DashSpeed * DeltaTime, true, &Hit);

	if (!bHitThisDash && IsValid(PlayerPawn) &&
		FVector::DistSquared(GetActorLocation(), GetPlayerTargetLocation()) <= FMath::Square(HitRadius))
	{
		bHitThisDash = true;  // 한 번의 돌진에 한 번만 피격
		UGameplayStatics::ApplyDamage(PlayerPawn, AttackDamage, GetController(), this, UDamageType::StaticClass());
	}

	if (Hit.bBlockingHit || StateTime >= DashDuration)
	{
		FinishAttack();
	}
}
