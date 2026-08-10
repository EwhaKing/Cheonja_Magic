// HanjaCharacterRecognizer.h
//
// Recognizes the kanji/hanja character for "water" (Unicode U+6C34) drawn as 4 separate
// strokes, in the correct stroke order, using motion-controller input.
// Each stroke (button held -> drawn -> released) is checked one at a time
// against the expected next stroke in the sequence. Draw all 4 correctly,
// in order, and the character registers as complete.
//
// Internally this reuses UDollarGestureRecognizer to do the actual shape
// matching for each individual stroke - no external plugins needed.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DollarGestureRecognizer.h"
#include "HanjaCharacterRecognizer.generated.h"

UENUM(BlueprintType)
enum class ECastSequenceState : uint8
{
	// Correct stroke, but the character isn't complete yet - keep drawing.
	InProgress,
	// All strokes drawn correctly and in order - the character is complete.
	Completed,
	// Wrong stroke, or a shape that didn't match - sequence has been reset.
	Failed
};

/** Feedback returned after each individual stroke is submitted. */
USTRUCT(BlueprintType)
struct FCastProgressResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Gesture")
	ECastSequenceState State = ECastSequenceState::Failed;

	// How many strokes have been correctly drawn so far in this attempt (0-4).
	UPROPERTY(BlueprintReadOnly, Category = "Gesture")
	int32 StrokesCompleted = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Gesture")
	int32 StrokesRequired = 4;

	UPROPERTY(BlueprintReadOnly, Category = "Gesture")
	FString CharacterName;

	// Match confidence (0-1) of the stroke that was just submitted.
	UPROPERTY(BlueprintReadOnly, Category = "Gesture")
	float LastStrokeScore = 0.0f;
};

/**
 * Usage in Blueprint:
 *   1. Construct Object from Class = HanjaCharacterRecognizer, promote to a
 *      variable (e.g. HanjaRecognizer). Do this once, e.g. in BeginPlay.
 *   2. Call Initialize() on it once, right after creating it.
 *   3. Each time the player draws one stroke (button pressed -> dragged ->
 *      released), project that single stroke's 3D points to 2D (see
 *      UDollarGestureRecognizer::ProjectStrokeToViewPlane) and call
 *      SubmitStroke() with the result.
 *   4. Branch on the returned State:
 *        - InProgress: good stroke, wait for the next one.
 *        - Completed: all 4 strokes matched in order - cast the water spell.
 *        - Failed: wrong stroke - sequence auto-resets, let the player retry.
 */
UCLASS(BlueprintType)
class UHanjaCharacterRecognizer : public UObject
{
	GENERATED_BODY()

public:
	/** Call once after creating this object. Sets up the 4 stroke templates for the water character. */
	UFUNCTION(BlueprintCallable, Category = "Gesture Recognition")
	void Initialize();

	/**
	 * Call once per completed stroke (on button release), passing that
	 * stroke's 2D points. Advances, completes, or resets the sequence.
	 */
	UFUNCTION(BlueprintCallable, Category = "Gesture Recognition")
	FCastProgressResult SubmitStroke(const TArray<FVector2D>& RawStrokePoints, float MinScoreThreshold = 0.65f);

	/** Manually abort the current attempt and start over from stroke 1. */
	UFUNCTION(BlueprintCallable, Category = "Gesture Recognition")
	void ResetSequence();

	/** How many correct strokes have been drawn so far in the current attempt. */
	UFUNCTION(BlueprintCallable, Category = "Gesture Recognition")
	int32 GetCurrentStrokeIndex() const { return CurrentStrokeIndex; }

	/**
	 * Identifies which of the 4 known stroke shapes a freshly-drawn stroke
	 * looks most like, regardless of order. Returns 0-3 for a match, or -1
	 * if nothing scored above MinScoreThreshold. Use this right after each
	 * IA_Draw release - push the returned index into a flat Int32 array in
	 * Blueprint (e.g. DrawnStrokeSequence). OutScore is the best score found.
	 */
	UFUNCTION(BlueprintCallable, Category = "Gesture Recognition")
	int32 IdentifyStroke(const TArray<FVector2D>& RawStrokePoints, float& OutScore, float MinScoreThreshold = 0.6f) const;

	/**
	 * Checks whether a recorded sequence of stroke indices (e.g. your
	 * DrawnStrokeSequence array, built up over several IA_Draw calls) exactly
	 * matches the required order for this character: [0, 1, 2, 3]. Call this
	 * on IA_Cast. Both length and order must match exactly.
	 */
	UFUNCTION(BlueprintCallable, Category = "Gesture Recognition")
	bool CheckDrawnSequence(const TArray<int32>& DrawnIndices) const;


private:
	UPROPERTY()
	UDollarGestureRecognizer* StrokeMatcher = nullptr;

	int32 CurrentStrokeIndex = 0;

	static constexpr int32 TotalStrokes = 4;
	static const FString CharacterDisplayName;

	void BuildWaterStrokeTemplates();
};
