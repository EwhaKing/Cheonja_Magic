#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyBase.generated.h"

class USceneComponent;

// 몬스터 상태. 기획서의 3단계(암시/공격/쿨타임)에 대기·추격·사망을 더한 것.
UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle      UMETA(DisplayName = "대기"),
	Chasing   UMETA(DisplayName = "추격"),
	Windup    UMETA(DisplayName = "공격 암시"),
	Attacking UMETA(DisplayName = "공격"),
	Cooldown  UMETA(DisplayName = "쿨타임"),
	Dead      UMETA(DisplayName = "사망")
};

// 모든 몬스터의 공통 뼈대. 직접 배치 불가(Abstract) — 자식 클래스/BP로만 사용.
UCLASS(Abstract)
class CHEONJA_MAGIC_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyBase();

	virtual void Tick(float DeltaTime) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	                         class AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	EEnemyState GetState() const { return State; }

	// ================= 기획 튜닝 값 (BP에서 수정) =================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|스탯")
	float MaxHealth = 45.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|스탯")
	float Health = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|감지")
	float DetectRange = 500.f;   // 수평 감지 반경(cm)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|감지")
	float DetectHeight = 360.f;  // 수직 감지 범위(cm)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|전투")
	float AttackRange = 150.f;   // 이 거리 안이면 공격 암시 시작

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|전투")
	float WindupTime = 1.f;      // 공격 암시 시간(기획서 1초)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|전투")
	float AttackDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|전투")
	float CooldownMin = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|전투")
	float CooldownMax = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|이동")
	bool bApproachDuringCooldown = true;  // 원거리몹은 false

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|이동")
	bool bFlying = false;                 // 가오리·덤보문어: true (중력 없이 떠다님)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|이동")
	float TurnSpeed = 5.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|상태")
	EEnemyState State = EEnemyState::Idle;

protected:
	virtual void BeginPlay() override;

	// 자식 클래스가 재정의하는 실제 공격. 끝나면 반드시 FinishAttack() 호출.
	virtual void PerformAttack();

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void FinishAttack();

	// ============ BP 연출 훅 (구현은 BP에서, 안 해도 됨) ============
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void OnStateChanged(EEnemyState NewState);   // 애니메이션 전환용

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void OnDetectionChanged(bool bDetected);     // 감지 색 파랑↔빨강

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void OnWindupStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void OnAttackStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void OnDeath();

	// ============ 내부 도우미 ============
	void SetState(EEnemyState NewState);
	FVector GetPlayerTargetLocation() const;   // VR 머리(HMD) 위치
	bool IsPlayerInRange(float Range) const;
	void MoveTowardPlayer(float DeltaTime);
	void FacePlayer(float DeltaTime);

	float StateTime = 0.f;
	float CurrentCooldown = 4.f;

	UPROPERTY()
	TObjectPtr<APawn> PlayerPawn;

	// VR 폰의 원점은 플레이 공간 바닥 중심이라 실제 플레이어 위치와 다름 → 카메라를 조준점으로.
	UPROPERTY()
	TObjectPtr<USceneComponent> PlayerHead;
};
