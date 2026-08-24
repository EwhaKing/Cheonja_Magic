#pragma once
// ↑ "이 헤더가 여러 번 include되어도 한 번만 읽어라"는 표준 안전장치. 모든 헤더 첫 줄 고정.

#include "CoreMinimal.h"              // 언리얼 기본 타입(FVector 등) 모음. 거의 모든 헤더에 필요.
#include "GameFramework/Character.h"  // 부모 클래스인 ACharacter의 설계도.
#include "EnemyBase.generated.h"
// ↑ 언리얼이 자동 생성하는 코드. 반드시 "include 목록의 맨 마지막"이어야 함 (언리얼 규칙).

// 몬스터의 상태 목록. 기획서의 3단계(암시/공격/쿨타임)에 이동·대기·사망을 더한 것.
// UENUM(BlueprintType): 이 목록을 Blueprint에서도 쓸 수 있게 등록.
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

// UCLASS(Abstract): "이 클래스는 뼈대일 뿐이니 레벨에 직접 배치하지 못하게 막아라".
// CHEONJA_MAGIC_API: 다른 모듈에서 이 클래스를 쓸 수 있게 하는 내보내기 표식(관례상 붙임).
UCLASS(Abstract)
class CHEONJA_MAGIC_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()  // 언리얼 리플렉션(에디터 연동)용 필수 매크로. 클래스 본문 첫 줄 고정.

public:
	AEnemyBase();  // 생성자: 액터가 만들어질 때 1회 실행되는 초기 설정.

protected:
	virtual void BeginPlay() override;
	// ↑ 게임이 시작되어 이 액터가 활동을 개시할 때 1회 호출. override = "부모의 동명 함수를 대체한다".

public:
	virtual void Tick(float DeltaTime) override;
	// ↑ 매 프레임 호출. DeltaTime = 직전 프레임과의 시간 간격(초). FSM이 여기서 돌아감.

	// 피격 처리. 언리얼 내장 데미지 시스템의 표준 창구를 우리 입맛대로 재정의.
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

	// ================= 기획 튜닝 값 =================
	// EditAnywhere: 에디터/Blueprint 자식에서 수정 가능. BlueprintReadWrite: BP 그래프에서 읽고 쓰기 가능.
	// Category: 에디터 상세 패널에서 묶여 보이는 그룹명.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|스탯")
	float MaxHealth = 45.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|스탯")
	float Health = 0.f;  // 현재 체력. VisibleAnywhere = 보이기만 하고 실수로 못 고치게.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|감지")
	float DetectRange = 500.f;  // 수평 감지 반경(cm). 기획 확정 시 수정.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|감지")
	float DetectHeight = 360.f;  // 수직 감지 범위(cm). 기획서: 마네킹 키의 두 배 ≈ 360.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|전투")
	float AttackRange = 150.f;  // 이 거리 안에 들어오면 공격 개시.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|전투")
	float WindupTime = 1.f;  // 공격 암시 시간. 기획서: 1초.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|전투")
	float AttackDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|전투")
	float CooldownMin = 3.f;  // 쿨타임 최소. 기획서: 3~5초.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|전투")
	float CooldownMax = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|이동")
	bool bApproachDuringCooldown = true;  // 기획서: 일몹·빠른몹은 쿨타임에도 접근. 원거리몹 BP에서 끔.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|이동")
	float TurnSpeed = 5.f;  // 플레이어를 향해 도는 속도(클수록 빠릿함).

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|상태")
	EEnemyState State = EEnemyState::Idle;  // 현재 상태(디버깅용으로 노출).

protected:
	// ============ 파생 클래스가 재정의할 것 ============
	virtual void PerformAttack();  // 실제 공격 행위. 근접/돌진/원거리가 각자 다르게 구현.

	// 공격 동작이 끝났을 때 호출 → 쿨타임으로 전환. BP에서도 부를 수 있게 BlueprintCallable.
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void FinishAttack();

	// ============ Blueprint 연출 훅 ============
	// BlueprintImplementableEvent: C++에는 껍데기만 있고, 내용은 BP에서 구현하는 이벤트.
	// 로직(C++)과 연출(BP)을 분리하는 핵심 장치. 구현 안 해도 에러 없음.

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void OnDetectionChanged(bool bDetected);  // 감지 시작/해제 → 눈 색 파랑↔빨강 연출은 BP에서.

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void OnWindupStarted();  // 공격 암시 연출(가오리: 회전 시작).

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void OnAttackStarted();  // 공격 순간 연출.

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void OnDeath();  // 사망 연출.

	// ============ 내부 도우미 ============
	void SetState(EEnemyState NewState);          // 상태 전환 + 경과 시간 리셋.
	bool IsPlayerInRange(float Range) const;      // 플레이어가 수평 Range·수직 DetectHeight 안인가.
	void MoveTowardPlayer(float DeltaTime);       // 플레이어 쪽으로 이동 + 회전.

	float StateTime = 0.f;        // 현재 상태에 머문 시간(초). 암시 1초, 쿨타임 측정에 사용.
	float CurrentCooldown = 4.f;  // 이번에 뽑힌 쿨타임 길이(3~5 사이 랜덤).

	// 플레이어 참조 저장. UPROPERTY를 붙여야 언리얼 메모리 관리(GC)가 "사용 중"임을 알고
	// 대상이 파괴됐을 때 자동으로 null 처리해줌. 이거 빼먹으면 미래의 크래시 1순위 후보.
	UPROPERTY()
	TObjectPtr<APawn> PlayerPawn;
};