// HanjaCharacterRecognizer.cpp

#include "HanjaCharacterRecognizer.h"

const FString UHanjaCharacterRecognizer::CharacterDisplayName = TEXT("Water (\u6C34)");

void UHanjaCharacterRecognizer::Initialize()
{
	if (!StrokeMatcher)
	{
		StrokeMatcher = NewObject<UDollarGestureRecognizer>(this);
	}
	CurrentStrokeIndex = 0;
	BuildWaterStrokeTemplates();
}

void UHanjaCharacterRecognizer::ResetSequence()
{
	CurrentStrokeIndex = 0;
}

FCastProgressResult UHanjaCharacterRecognizer::SubmitStroke(const TArray<FVector2D>& RawStrokePoints, float MinScoreThreshold)
{
	FCastProgressResult Result;
	Result.CharacterName = CharacterDisplayName;
	Result.StrokesRequired = TotalStrokes;

	if (!StrokeMatcher)
	{
		// Initialize() wasn't called - fail safely rather than crash.
		Result.State = ECastSequenceState::Failed;
		return Result;
	}

	const FString ExpectedTemplateName = FString::Printf(TEXT("Water_Stroke%d"), CurrentStrokeIndex);
	const float Score = StrokeMatcher->ScoreAgainstTemplate(RawStrokePoints, ExpectedTemplateName);
	Result.LastStrokeScore = Score;

	if (Score >= MinScoreThreshold)
	{
		++CurrentStrokeIndex;

		if (CurrentStrokeIndex >= TotalStrokes)
		{
			Result.State = ECastSequenceState::Completed;
			Result.StrokesCompleted = TotalStrokes;
			CurrentStrokeIndex = 0; // ready for the next cast attempt
		}
		else
		{
			Result.State = ECastSequenceState::InProgress;
			Result.StrokesCompleted = CurrentStrokeIndex;
		}
	}
	else
	{
		Result.State = ECastSequenceState::Failed;
		Result.StrokesCompleted = CurrentStrokeIndex;
		CurrentStrokeIndex = 0; // wrong stroke - start the attempt over
	}

	return Result;
}

int32 UHanjaCharacterRecognizer::IdentifyStroke(const TArray<FVector2D>& RawStrokePoints, float& OutScore, float MinScoreThreshold)
{
	OutScore = 0.0f;

	if (!StrokeMatcher || RawStrokePoints.Num() < 2)
	{
		return -1;
	}

	int32 BestIndex = -1;
	float BestScore = 0.0f;

	for (int32 i = 0; i < TotalStrokes; ++i)
	{
		const FString TemplateName = FString::Printf(TEXT("Water_Stroke%d"), i);

		// Strokes 2 and 3 are both simple diagonal-ish shapes that can look
		// nearly identical to each other (or to stroke 1) once the algorithm
		// is allowed to rotate them up to 45 degrees to find a better match.
		// Tighten the rotation search just for these two so a genuinely
		// differently-angled stroke can't masquerade as one of them.
		const float RotationRange = (i == 2 || i == 3) ? 15.0f : 45.0f;
		const bool bApplyIndicativeRotation = !(i == 2 || i == 3);

		const float Score = StrokeMatcher->ScoreAgainstTemplate(RawStrokePoints, TemplateName, RotationRange, bApplyIndicativeRotation);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestIndex = i;
		}
	}

	OutScore = BestScore;
	return (BestScore >= MinScoreThreshold) ? BestIndex : -1;
}

bool UHanjaCharacterRecognizer::CheckDrawnSequence(const TArray<int32>& DrawnIndices)
{
	if (DrawnIndices.Num() != TotalStrokes)
	{
		return false;
	}

	// Order no longer matters - just confirm every required stroke (0..TotalStrokes-1)
	// appears exactly once, in any order.
	TArray<bool> Seen;
	Seen.Init(false, TotalStrokes);

	for (int32 Index : DrawnIndices)
	{
		if (Index < 0 || Index >= TotalStrokes || Seen[Index])
		{
			// Out of range, or this stroke was already recorded once (a
			// duplicate) - either way, not a valid complete set.
			return false;
		}
		Seen[Index] = true;
	}

	return true;
}

void UHanjaCharacterRecognizer::BuildWaterStrokeTemplates()
{
	StrokeMatcher->ClearAllGestures();

	// Coordinate space: X grows right, Y grows downward, roughly a 0-100 box.
	// These points were traced by the user directly (Inkscape centerline
	// trace, one stroke per layer) and parsed exactly from the resulting
	// SVG path data - not approximated, these are the real drawn shapes.

	// Stroke 0: hook curving right then sweeping down-left.
	TArray<FVector2D> Stroke0;
	Stroke0.Add(FVector2D(14.4, 38.5));
	Stroke0.Add(FVector2D(21.7, 38.3));
	Stroke0.Add(FVector2D(36.5, 33.7));
	Stroke0.Add(FVector2D(36.6, 37.4));
	Stroke0.Add(FVector2D(30.6, 51.2));
	Stroke0.Add(FVector2D(24.5, 60.2));
	Stroke0.Add(FVector2D(18.2, 67.0));
	Stroke0.Add(FVector2D(10.2, 72.8));
	StrokeMatcher->AddGesture(TEXT("Water_Stroke0"), Stroke0);

	// Stroke 1: near-vertical line curling up-left at the bottom.
	TArray<FVector2D> Stroke1;
	Stroke1.Add(FVector2D(47.6, 9.0));
	Stroke1.Add(FVector2D(48.3, 41.0));
	Stroke1.Add(FVector2D(48.0, 63.5));
	Stroke1.Add(FVector2D(47.5, 76.0));
	Stroke1.Add(FVector2D(46.0, 82.4));
	Stroke1.Add(FVector2D(41.0, 77.7));
	Stroke1.Add(FVector2D(36.9, 75.6));
	StrokeMatcher->AddGesture(TEXT("Water_Stroke1"), Stroke1);

	// Stroke 2: sweeping curve from upper-middle-left down to the right.
	TArray<FVector2D> Stroke2;
	Stroke2.Add(FVector2D(48.5, 36.8));
	Stroke2.Add(FVector2D(65.2, 55.2));
	Stroke2.Add(FVector2D(75.5, 63.1));
	Stroke2.Add(FVector2D(82.1, 66.1));
	Stroke2.Add(FVector2D(85.0, 66.5));
	StrokeMatcher->AddGesture(TEXT("Water_Stroke2"), Stroke2);

	// Stroke 3: short diagonal sweeping up to the right.
	TArray<FVector2D> Stroke3;
	Stroke3.Add(FVector2D(52.8, 41.8));
	Stroke3.Add(FVector2D(71.5, 24.2));
	Stroke3.Add(FVector2D(73.3, 21.7));
	Stroke3.Add(FVector2D(73.4, 20.6));
	StrokeMatcher->AddGesture(TEXT("Water_Stroke3"), Stroke3);
}