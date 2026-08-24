#include "EnemyBase.h"
#include "Kismet/GameplayStatics.h"                     // GetPlayerPawn 사용.
#include "GameFramework/CharacterMovementComponent.h"    // 이동 컴포넌트 접근.

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;  // Tick을 쓰겠다고 선언.

	// 레벨에 배치되거나 스폰되면 자동으로 AI 컨트롤러가 빙의하게 함.
	// 컨트롤러가 없으면 캐릭터 이동 입력이 무시되므로 필수.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;  // 회전은 우리가 직접 제어.
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();  // 부모(ACharacter)의 시작 처리 먼저. override 함수의 관례.

	Health = MaxHealth;
	// 플레이어(0번 = 로컬 플레이어)의 폰을 찾아 저장. VR 폰이 이걸로 잡힘.
	PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	SetState(EEnemyState::Idle);
}

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (State == EEnemyState::Dead)
	{
		return;  // 죽었으면 아무것도 안 함.
	}

	StateTime += DeltaTime;  // 현재 상태 경과 시간 누적.

	switch (State)  // ★ FSM 본체: 상태에 따라 이번 프레임에 할 일을 고른다.
	{
	case EEnemyState::Idle:
		// 기획서: 배틀 안 하는 상태에선 가만히. 감지되면 추격 개시.
		if (IsPlayerInRange(DetectRange))
		{
			OnDetectionChanged(true);           // BP에 "감지됨" 신호(색 변화 연출).
			SetState(EEnemyState::Chasing);
		}
		break;

	case EEnemyState::Chasing:
		// 감지 범위를 넉넉히(1.2배) 벗어나면 추격 포기. 경계에서 껌뻑거리는 것 방지.
		if (!IsPlayerInRange(DetectRange * 1.2f))
		{
			OnDetectionChanged(false);
			SetState(EEnemyState::Idle);
			break;
		}
		MoveTowardPlayer(DeltaTime);
		if (IsPlayerInRange(AttackRange))       // 사거리 진입 → 공격 암시 개시.
		{
			SetState(EEnemyState::Windup);
			OnWindupStarted();
		}
		break;

	case EEnemyState::Windup:
		if (StateTime >= WindupTime)            // 암시 1초 경과 → 공격 실행.
		{
			SetState(EEnemyState::Attacking);
			OnAttackStarted();
			PerformAttack();
		}
		break;

	case EEnemyState::Attacking:
		// 공격의 종료 시점은 공격마다 다르므로(할퀴기 즉발, 돌진은 수 초)
		// 파생 클래스가 FinishAttack()을 불러줄 때까지 기다린다.
		break;

	case EEnemyState::Cooldown:
		if (bApproachDuringCooldown)            // 기획서: 쿨타임에도 접근(원거리몹 제외).
		{
			MoveTowardPlayer(DeltaTime);
		}
		if (StateTime >= CurrentCooldown)
		{
			SetState(EEnemyState::Chasing);     // 쿨타임 종료 → 다시 추격부터.
		}
		break;

	default:
		break;
	}
}

void AEnemyBase::PerformAttack()
{
	// 기본 구현은 "아무것도 안 하고 즉시 종료". 파생 클래스가 반드시 재정의할 것.
	FinishAttack();
}

void AEnemyBase::FinishAttack()
{
	if (State != EEnemyState::Attacking)
	{
		return;  // 공격 중이 아닐 때 잘못 불려도 무시(안전장치).
	}
	CurrentCooldown = FMath::RandRange(CooldownMin, CooldownMax);  // 3~5초 랜덤.
	SetState(EEnemyState::Cooldown);
}

void AEnemyBase::SetState(EEnemyState NewState)
{
	State = NewState;
	StateTime = 0.f;  // 상태가 바뀌면 스톱워치 리셋.
}

bool AEnemyBase::IsPlayerInRange(float Range) const
{
	if (!IsValid(PlayerPawn))  // 플레이어가 없거나 파괴됐으면 false. 널 체크 필수.
	{
		return false;
	}
	const FVector Mine = GetActorLocation();
	const FVector Theirs = PlayerPawn->GetActorLocation();

	if (FMath::Abs(Theirs.Z - Mine.Z) > DetectHeight)  // 수직 범위부터 검사.
	{
		return false;
	}
	// 수평 거리 비교. DistSquaredXY = 제곱 거리(제곱근 계산 생략) → VR 성능 규칙.
	return FVector::DistSquaredXY(Mine, Theirs) <= FMath::Square(Range);
}

void AEnemyBase::MoveTowardPlayer(float DeltaTime)
{
	if (!IsValid(PlayerPawn))
	{
		return;
	}
	FVector ToPlayer = PlayerPawn->GetActorLocation() - GetActorLocation();
	ToPlayer.Z = 0.f;                    // 수평 방향만(높이는 무시).
	if (ToPlayer.IsNearlyZero())
	{
		return;                          // 이미 겹쳐 있으면 계산 생략(0으로 나누기 방지).
	}
	const FVector Dir = ToPlayer.GetSafeNormal();  // 길이 1짜리 방향 벡터로 정규화.

	AddMovementInput(Dir);  // 캐릭터 이동 컴포넌트에 "이쪽으로 가라" 입력. 속도는 컴포넌트 설정값.

	// 이동 방향을 향해 부드럽게 회전(RInterpTo = 현재→목표 각도를 서서히 보간).
	const FRotator TargetRot = Dir.Rotation();
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, TurnSpeed));
}

float AEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	// 부모의 기본 처리(데미지 배율 등)를 먼저 거친 실제 적용량을 받는다.
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
		OnDeath();                                            // BP 사망 연출 신호.
		GetCharacterMovement()->StopMovementImmediately();    // 그 자리에 정지.
		SetActorEnableCollision(false);                       // 시체와 충돌하지 않게.
		SetLifeSpan(2.f);                                     // 2초 뒤 자동 소멸(연출 시간 확보).
	}
	return Applied;
}