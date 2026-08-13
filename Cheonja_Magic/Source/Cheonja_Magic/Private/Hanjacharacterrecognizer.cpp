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
		const float Score = StrokeMatcher->ScoreAgainstTemplate(RawStrokePoints, TemplateName);

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
	// Stroke order follows the standard kanji rule for symmetrical characters:
	// center stroke first, then the surrounding strokes, top-to-bottom /
	// left-to-right. These are approximate "clean" reference shapes - the
	// recognizer is tolerant of size, position, and rotation, so the player
	// does not need to trace them exactly.

	// Stroke 0: center vertical stroke with a small leftward hook at the bottom.
	TArray<FVector2D> Stroke0;
	Stroke0.Add(FVector2D(50, 10));
	Stroke0.Add(FVector2D(50, 40));
	Stroke0.Add(FVector2D(50, 70));
	Stroke0.Add(FVector2D(50, 80));
	Stroke0.Add(FVector2D(44, 86));
	Stroke0.Add(FVector2D(40, 88));
	StrokeMatcher->AddGesture(TEXT("Water_Stroke0"), Stroke0);

	// Stroke 1: short diagonal tick, upper-left area.
	TArray<FVector2D> Stroke1;
	Stroke1.Add(FVector2D(38, 25));
	Stroke1.Add(FVector2D(34, 30));
	Stroke1.Add(FVector2D(30, 35));
	StrokeMatcher->AddGesture(TEXT("Water_Stroke1"), Stroke1);

	// Stroke 2: longer diagonal sweeping from center down to lower-left.
	TArray<FVector2D> Stroke2;
	Stroke2.Add(FVector2D(45, 55));
	Stroke2.Add(FVector2D(35, 65));
	Stroke2.Add(FVector2D(25, 78));
	Stroke2.Add(FVector2D(15, 90));
	StrokeMatcher->AddGesture(TEXT("Water_Stroke2"), Stroke2);

	// Stroke 3: longest diagonal, upper area sweeping down to lower-right.
	TArray<FVector2D> Stroke3;
	Stroke3.Add(FVector2D(55, 45));
	Stroke3.Add(FVector2D(68, 62));
	Stroke3.Add(FVector2D(78, 78));
	Stroke3.Add(FVector2D(88, 92));
	StrokeMatcher->AddGesture(TEXT("Water_Stroke3"), Stroke3);
}