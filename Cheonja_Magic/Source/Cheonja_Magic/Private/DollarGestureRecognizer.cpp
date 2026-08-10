// DollarGestureRecognizer.cpp

#include "DollarGestureRecognizer.h"

// ---------- Public API ----------

void UDollarGestureRecognizer::AddGesture(const FString& GestureName, const TArray<FVector2D>& RawPoints)
{
	if (RawPoints.Num() < 2)
	{
		return;
	}

	FGestureTemplate NewTemplate;
	NewTemplate.Name = GestureName;
	NewTemplate.Points = Normalize(RawPoints);
	Templates.Add(NewTemplate);
}

void UDollarGestureRecognizer::ClearGesture(const FString& GestureName)
{
	Templates.RemoveAll([&GestureName](const FGestureTemplate& T) { return T.Name == GestureName; });
}

void UDollarGestureRecognizer::ClearAllGestures()
{
	Templates.Empty();
}

FGestureResult UDollarGestureRecognizer::RecognizeGesture(const TArray<FVector2D>& RawPoints, float MinScoreThreshold)
{
	FGestureResult Result;

	if (RawPoints.Num() < 2 || Templates.Num() == 0)
	{
		return Result;
	}

	const TArray<FVector2D> Candidate = Normalize(RawPoints);

	const float ThetaA = FMath::DegreesToRadians(AngleSearchRange);
	const float ThetaDelta = FMath::DegreesToRadians(AngleSearchPrecision);
	const float HalfDiagonal = 0.5f * FMath::Sqrt(SquareSize * SquareSize + SquareSize * SquareSize);

	float BestScore = -1.0f;
	FString BestName;

	for (const FGestureTemplate& Template : Templates)
	{
		const float Distance = DistanceAtBestAngle(Candidate, Template.Points, -ThetaA, ThetaA, ThetaDelta);
		const float Score = 1.0f - (Distance / HalfDiagonal);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestName = Template.Name;
		}
	}

	Result.Name = BestName;
	Result.Score = FMath::Clamp(BestScore, 0.0f, 1.0f);
	Result.bMatched = Result.Score >= MinScoreThreshold;
	return Result;
}

TArray<FVector2D> UDollarGestureRecognizer::ProjectStrokeToViewPlane(const TArray<FVector>& WorldPoints, const FVector& ViewLocation, const FVector& ViewForward, const FVector& ViewRight, const FVector& ViewUp)
{
	TArray<FVector2D> Projected;
	Projected.Reserve(WorldPoints.Num());

	for (const FVector& P : WorldPoints)
	{
		const FVector Offset = P - ViewLocation;
		const float X = FVector::DotProduct(Offset, ViewRight);
		const float Y = FVector::DotProduct(Offset, ViewUp);
		Projected.Add(FVector2D(X, Y));
	}

	return Projected;
}

// ---------- Preprocessing pipeline ----------

TArray<FVector2D> UDollarGestureRecognizer::Normalize(const TArray<FVector2D>& RawPoints)
{
	TArray<FVector2D> Points = Resample(RawPoints, NumResamplePoints);
	const float Radians = IndicativeAngle(Points);
	Points = RotateBy(Points, -Radians);
	Points = ScaleToSquare(Points, SquareSize);
	Points = TranslateToOrigin(Points);
	return Points;
}

TArray<FVector2D> UDollarGestureRecognizer::Resample(const TArray<FVector2D>& InPoints, int32 TargetCount)
{
	TArray<FVector2D> Points = InPoints;
	const float IntervalLength = PathLength(Points) / static_cast<float>(TargetCount - 1);

	if (IntervalLength <= KINDA_SMALL_NUMBER)
	{
		// Degenerate stroke (a single point / near-zero movement): pad with copies.
		TArray<FVector2D> Padded;
		Padded.Init(Points.Num() > 0 ? Points[0] : FVector2D::ZeroVector, TargetCount);
		return Padded;
	}

	float AccumulatedDistance = 0.0f;
	TArray<FVector2D> NewPoints;
	NewPoints.Add(Points[0]);

	for (int32 i = 1; i < Points.Num(); ++i)
	{
		const float SegmentDistance = FVector2D::Distance(Points[i - 1], Points[i]);

		if ((AccumulatedDistance + SegmentDistance) >= IntervalLength)
		{
			const float T = (IntervalLength - AccumulatedDistance) / SegmentDistance;
			const FVector2D NewPoint = FMath::Lerp(Points[i - 1], Points[i], T);

			NewPoints.Add(NewPoint);
			Points.Insert(NewPoint, i); // continue resampling from this inserted point
			AccumulatedDistance = 0.0f;
		}
		else
		{
			AccumulatedDistance += SegmentDistance;
		}
	}

	// Floating point rounding can leave us one point short.
	if (NewPoints.Num() < TargetCount)
	{
		NewPoints.Add(Points.Last());
	}
	else if (NewPoints.Num() > TargetCount)
	{
		NewPoints.SetNum(TargetCount);
	}

	return NewPoints;
}

float UDollarGestureRecognizer::PathLength(const TArray<FVector2D>& Points)
{
	float Length = 0.0f;
	for (int32 i = 1; i < Points.Num(); ++i)
	{
		Length += FVector2D::Distance(Points[i - 1], Points[i]);
	}
	return Length;
}

FVector2D UDollarGestureRecognizer::Centroid(const TArray<FVector2D>& Points)
{
	FVector2D Sum = FVector2D::ZeroVector;
	for (const FVector2D& P : Points)
	{
		Sum += P;
	}
	return Points.Num() > 0 ? (Sum / static_cast<float>(Points.Num())) : FVector2D::ZeroVector;
}

float UDollarGestureRecognizer::IndicativeAngle(const TArray<FVector2D>& Points)
{
	if (Points.Num() == 0)
	{
		return 0.0f;
	}
	const FVector2D C = Centroid(Points);
	return FMath::Atan2(C.Y - Points[0].Y, C.X - Points[0].X);
}

TArray<FVector2D> UDollarGestureRecognizer::RotateBy(const TArray<FVector2D>& Points, float Radians)
{
	const FVector2D C = Centroid(Points);
	const float CosA = FMath::Cos(Radians);
	const float SinA = FMath::Sin(Radians);

	TArray<FVector2D> Result;
	Result.Reserve(Points.Num());

	for (const FVector2D& P : Points)
	{
		const float Dx = P.X - C.X;
		const float Dy = P.Y - C.Y;
		const float NewX = (Dx * CosA) - (Dy * SinA) + C.X;
		const float NewY = (Dx * SinA) + (Dy * CosA) + C.Y;
		Result.Add(FVector2D(NewX, NewY));
	}
	return Result;
}

TArray<FVector2D> UDollarGestureRecognizer::ScaleToSquare(const TArray<FVector2D>& Points, float Size)
{
	FVector2D Min(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
	FVector2D Max(TNumericLimits<float>::Lowest(), TNumericLimits<float>::Lowest());

	for (const FVector2D& P : Points)
	{
		Min.X = FMath::Min(Min.X, P.X);
		Min.Y = FMath::Min(Min.Y, P.Y);
		Max.X = FMath::Max(Max.X, P.X);
		Max.Y = FMath::Max(Max.Y, P.Y);
	}

	const float Width = FMath::Max(Max.X - Min.X, KINDA_SMALL_NUMBER);
	const float Height = FMath::Max(Max.Y - Min.Y, KINDA_SMALL_NUMBER);

	TArray<FVector2D> Result;
	Result.Reserve(Points.Num());

	for (const FVector2D& P : Points)
	{
		const float NewX = (P.X - Min.X) * (Size / Width);
		const float NewY = (P.Y - Min.Y) * (Size / Height);
		Result.Add(FVector2D(NewX, NewY));
	}
	return Result;
}

TArray<FVector2D> UDollarGestureRecognizer::TranslateToOrigin(const TArray<FVector2D>& Points)
{
	const FVector2D C = Centroid(Points);

	TArray<FVector2D> Result;
	Result.Reserve(Points.Num());

	for (const FVector2D& P : Points)
	{
		Result.Add(P - C);
	}
	return Result;
}

// ---------- Matching ----------

float UDollarGestureRecognizer::PathDistance(const TArray<FVector2D>& PointsA, const TArray<FVector2D>& PointsB)
{
	// Both arrays are assumed to be the same length (NumResamplePoints) and
	// correspond index-for-index.
	const int32 Count = FMath::Min(PointsA.Num(), PointsB.Num());
	if (Count == 0)
	{
		return 0.0f;
	}

	float Total = 0.0f;
	for (int32 i = 0; i < Count; ++i)
	{
		Total += FVector2D::Distance(PointsA[i], PointsB[i]);
	}
	return Total / static_cast<float>(Count);
}

float UDollarGestureRecognizer::DistanceAtAngle(const TArray<FVector2D>& Points, const TArray<FVector2D>& Template, float Radians)
{
	const TArray<FVector2D> Rotated = RotateBy(Points, Radians);
	return PathDistance(Rotated, Template);
}

float UDollarGestureRecognizer::DistanceAtBestAngle(const TArray<FVector2D>& Points, const TArray<FVector2D>& Template, float ThetaA, float ThetaB, float ThetaDelta)
{
	// Golden section search for the rotation angle that minimizes distance.
	const float Phi = 0.5f * (-1.0f + FMath::Sqrt(5.0f)); // ~0.618

	float X1 = Phi * ThetaA + (1.0f - Phi) * ThetaB;
	float F1 = DistanceAtAngle(Points, Template, X1);
	float X2 = (1.0f - Phi) * ThetaA + Phi * ThetaB;
	float F2 = DistanceAtAngle(Points, Template, X2);

	while (FMath::Abs(ThetaB - ThetaA) > ThetaDelta)
	{
		if (F1 < F2)
		{
			ThetaB = X2;
			X2 = X1;
			F2 = F1;
			X1 = Phi * ThetaA + (1.0f - Phi) * ThetaB;
			F1 = DistanceAtAngle(Points, Template, X1);
		}
		else
		{
			ThetaA = X1;
			X1 = X2;
			F1 = F2;
			X2 = (1.0f - Phi) * ThetaA + Phi * ThetaB;
			F2 = DistanceAtAngle(Points, Template, X2);
		}
	}

	return FMath::Min(F1, F2);
}
