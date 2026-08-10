// DollarGestureRecognizer.h
// Unreal Engine 5 C++ port of the $1 Unistroke Recognizer
// Original algorithm: Wobbrock, J.O., Wilson, A.D. and Li, Y. (2007)
// "Gestures without libraries, toolkits or training: A $1 recognizer for user interface prototypes"
// This is a from scratch reimplementation of the published algorithm no external
// libraries or plugins required, just stock Unreal/Engine types.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DollarGestureRecognizer.generated.h"

/** A single named gesture template: a shape made of 2D points. */
USTRUCT(BlueprintType)
struct FGestureTemplate
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Gesture")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Gesture")
	TArray<FVector2D> Points;
};

/** Result of a recognition attempt. */
USTRUCT(BlueprintType)
struct FGestureResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Gesture")
	FString Name;

	// 0.0 (no match) to 1.0 (perfect match)
	UPROPERTY(BlueprintReadOnly, Category = "Gesture")
	float Score = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Gesture")
	bool bMatched = false;
};

/**
 * Recognizes hand-drawn shapes (spell runes) regardless of size, position,
 * or rotation. Feed it a raw stroke (array of points captured while the
 * player holds the cast button), and it returns the closest matching
 * template gesture with a confidence score.
 *
 * Usage:
 *   1. Create one instance (e.g. as a field on your Character or a Subsystem).
 *   2. Call AddGesture() once per template, ideally with 3-5 example
 *      recordings per shape for better accuracy.
 *   3. At cast time, call RecognizeGesture() with the player's drawn stroke.
 */
UCLASS(BlueprintType)
class UDollarGestureRecognizer : public UObject
{
	GENERATED_BODY()

public:
	/** Registers a new template gesture (e.g. "Fireball" = a circle shape). */
	UFUNCTION(BlueprintCallable, Category = "Gesture Recognition")
	void AddGesture(const FString& GestureName, const TArray<FVector2D>& RawPoints);

	/** Removes all stored templates for a given name. Useful for re-teaching a shape. */
	UFUNCTION(BlueprintCallable, Category = "Gesture Recognition")
	void ClearGesture(const FString& GestureName);

	/** Removes every stored template. */
	UFUNCTION(BlueprintCallable, Category = "Gesture Recognition")
	void ClearAllGestures();

	/**
	 * Compares a freshly-drawn stroke against all stored templates and
	 * returns the best match. bMatched is only true if the score clears
	 * MinScoreThreshold.
	 */
	UFUNCTION(BlueprintCallable, Category = "Gesture Recognition")
	FGestureResult RecognizeGesture(const TArray<FVector2D>& RawPoints, float MinScoreThreshold = 0.75f);

	/**
	 * Helper for VR: projects a 3D world-space stroke (captured from a
	 * motion controller) onto a 2D plane facing the given camera/view
	 * location, so it can be fed into RecognizeGesture(). This assumes the
	 * player draws roughly facing the camera, which is true for most
	 * in-air rune casting.
	 */
	UFUNCTION(BlueprintCallable, Category = "Gesture Recognition")
	static TArray<FVector2D> ProjectStrokeToViewPlane(const TArray<FVector>& WorldPoints, const FVector& ViewLocation, const FVector& ViewForward, const FVector& ViewRight, const FVector& ViewUp);

private:
	UPROPERTY()
	TArray<FGestureTemplate> Templates;

	// --- Core $1 algorithm steps ---
	static TArray<FVector2D> Resample(const TArray<FVector2D>& Points, int32 TargetCount);
	static float PathLength(const TArray<FVector2D>& Points);
	static FVector2D Centroid(const TArray<FVector2D>& Points);
	static float IndicativeAngle(const TArray<FVector2D>& Points);
	static TArray<FVector2D> RotateBy(const TArray<FVector2D>& Points, float Radians);
	static TArray<FVector2D> ScaleToSquare(const TArray<FVector2D>& Points, float Size);
	static TArray<FVector2D> TranslateToOrigin(const TArray<FVector2D>& Points);
	static TArray<FVector2D> Normalize(const TArray<FVector2D>& RawPoints);

	static float PathDistance(const TArray<FVector2D>& PointsA, const TArray<FVector2D>& PointsB);
	static float DistanceAtAngle(const TArray<FVector2D>& Points, const TArray<FVector2D>& Template, float Radians);
	static float DistanceAtBestAngle(const TArray<FVector2D>& Points, const TArray<FVector2D>& Template, float ThetaA, float ThetaB, float ThetaDelta);

	static constexpr int32 NumResamplePoints = 64;
	static constexpr float SquareSize = 250.0f;
	static constexpr float AngleSearchRange = 45.0f;  // degrees, +/-
	static constexpr float AngleSearchPrecision = 2.0f; // degrees
};
