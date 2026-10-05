#include "EnemyBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;  // AI 컨트롤러 자동 빙의(이동 입력에 필요)
	bUseControllerRotationYaw = false;
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (PlayerPawn)
	{
		PlayerHead = PlayerPawn->FindComponentByClass<UCameraComponent>();
	}

	if (bFlying)
	{
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->SetMovementMode(MOVE_Flying);
			Move->BrakingDecelerationFlying = 2048.f;  // 0이면 입력이 끊겨도 계속 미끄러짐
		}
	}

	SetState(EEnemyState::Idle);
}

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (State == EEnemyState::Dead)
	{
		return;
	}

	StateTime += DeltaTime;

	switch (State)
	{
	case EEnemyState::Idle:
		if (IsPlayerInRange(DetectRange))
		{
			OnDetectionChanged(true);
			SetState(EEnemyState::Chasing);
		}
		break;

	case EEnemyState::Chasing:
		if (!IsPlayerInRange(DetectRange * 1.2f))  // 경계에서 깜빡임 방지용 여유
		{
			OnDetectionChanged(false);
			SetState(EEnemyState::Idle);
			break;
		}
		MoveTowardPlayer(DeltaTime);
		if (IsPlayerInRange(AttackRange))
		{
			SetState(EEnemyState::Windup);
			OnWindupStarted();
		}
		break;

	case EEnemyState::Windup:
		FacePlayer(DeltaTime);  // 암시 동안 플레이어 쪽으로 몸을 돌림
		if (StateTime >= WindupTime)
		{
			SetState(EEnemyState::Attacking);
			OnAttackStarted();
			PerformAttack();
		}
		break;

	case EEnemyState::Attacking:
		break;  // 종료는 자식 클래스가 FinishAttack()으로 알려줌

	case EEnemyState::Cooldown:
		if (bApproachDuringCooldown)
		{
			MoveTowardPlayer(DeltaTime);
		}
		else
		{
			FacePlayer(DeltaTime);
		}
		if (StateTime >= CurrentCooldown)
		{
			SetState(EEnemyState::Chasing);
		}
		break;

	default:
		break;
	}
}

void AEnemyBase::PerformAttack()
{
	FinishAttack();  // 기본 구현: 아무것도 안 함
}

void AEnemyBase::FinishAttack()
{
	if (State != EEnemyState::Attacking)
	{
		return;
	}
	CurrentCooldown = FMath::RandRange(CooldownMin, CooldownMax);
	SetState(EEnemyState::Cooldown);
}

void AEnemyBase::SetState(EEnemyState NewState)
{
	State = NewState;
	StateTime = 0.f;
	OnStateChanged(NewState);
}

FVector AEnemyBase::GetPlayerTargetLocation() const
{
	if (IsValid(PlayerHead))
	{
		return PlayerHead->GetComponentLocation();
	}
	if (IsValid(PlayerPawn))
	{
		return PlayerPawn->GetActorLocation();
	}
	return GetActorLocation();
}

bool AEnemyBase::IsPlayerInRange(float Range) const
{
	if (!IsValid(PlayerPawn))
	{
		return false;
	}
	const FVector Mine = GetActorLocation();
	const FVector Theirs = GetPlayerTargetLocation();

	if (FMath::Abs(Theirs.Z - Mine.Z) > DetectHeight)
	{
		return false;
	}
	return FVector::DistSquaredXY(Mine, Theirs) <= FMath::Square(Range);  // 제곱근 생략(성능)
}

void AEnemyBase::MoveTowardPlayer(float DeltaTime)
{
	if (!IsValid(PlayerPawn))
	{
		return;
	}
	FVector ToPlayer = GetPlayerTargetLocation() - GetActorLocation();
	if (!bFlying)
	{
		ToPlayer.Z = 0.f;  // 지상몹은 수평으로만
	}
	if (ToPlayer.IsNearlyZero())
	{
		return;
	}
	AddMovementInput(ToPlayer.GetSafeNormal());
	FacePlayer(DeltaTime);
}

void AEnemyBase::FacePlayer(float DeltaTime)
{
	if (!IsValid(PlayerPawn))
	{
		return;
	}
	const FVector ToPlayer = GetPlayerTargetLocation() - GetActorLocation();
	if (ToPlayer.IsNearlyZero())
	{
		return;
	}
	const FRotator Target(0.f, ToPlayer.Rotation().Yaw, 0.f);  // 좌우(Yaw)만 회전
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Target, DeltaTime, TurnSpeed));
}

float AEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                             AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (State == EEnemyState::Dead || Applied <= 0.f)
	{
		return Applied;
	}

	Health -= Applied;

	if (Health <= 0.f)
	{
		Health = 0.f;
		SetState(EEnemyState::Dead);
		OnDeath();
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->StopMovementImmediately();
		}
		SetActorEnableCollision(false);
		SetLifeSpan(2.f);
	}
	return Applied;
}
