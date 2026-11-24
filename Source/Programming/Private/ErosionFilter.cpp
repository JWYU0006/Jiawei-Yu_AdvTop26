#include "ErosionFilter.h"

FVector2d FErosionFilter::Hash(const FVector2d& X)
{
	const FVector2d K(0.3183099, 0.3678794);
	const FVector2d Y = X * K + FVector2d(K.Y, K.X);
	// Shared inner term of both components
	const double Inner = FMath::Frac(Y.X * Y.Y * (Y.X + Y.Y));
	return FVector2d(
		-1.0 + 2.0 * FMath::Frac(16.0 * K.X * Inner),
		-1.0 + 2.0 * FMath::Frac(16.0 * K.Y * Inner));
}

FVector3d FErosionFilter::Noised(const FVector2d& P)
{
	// Integer cell and position inside the cell
	const FVector2d I(FMath::FloorToDouble(P.X), FMath::FloorToDouble(P.Y));
	const FVector2d F = P - I;

	// Quintic fade curve and its derivative
	auto Fade = [](double T) { return T * T * T * (T * (T * 6.0 - 15.0) + 10.0); };
	auto FadeDerivative = [](double T) { return 30.0 * T * T * (T * (T - 2.0) + 1.0); };
	const FVector2d U(Fade(F.X), Fade(F.Y));
	const FVector2d DU(FadeDerivative(F.X), FadeDerivative(F.Y));

	// Random gradients at the four cell corners
	const FVector2d GA = Hash(I + FVector2d(0.0, 0.0));
	const FVector2d GB = Hash(I + FVector2d(1.0, 0.0));
	const FVector2d GC = Hash(I + FVector2d(0.0, 1.0));
	const FVector2d GD = Hash(I + FVector2d(1.0, 1.0));

	// Each corner's contribution: its gradient dotted with the offset from that corner
	const double VA = FVector2d::DotProduct(GA, F - FVector2d(0.0, 0.0));
	const double VB = FVector2d::DotProduct(GB, F - FVector2d(1.0, 0.0));
	const double VC = FVector2d::DotProduct(GC, F - FVector2d(0.0, 1.0));
	const double VD = FVector2d::DotProduct(GD, F - FVector2d(1.0, 1.0));

	// Bilinear blend of the four corners, weighted by the fade curve
	const double Cross = VA - VB - VC + VD;
	const double Value = VA + U.X * (VB - VA) + U.Y * (VC - VA) + U.X * U.Y * Cross;

	// Derivative of the blend above (product rule)
	const FVector2d Gradient =
		GA + (GB - GA) * U.X + (GC - GA) * U.Y + (GA - GB - GC + GD) * (U.X * U.Y)
		+ DU * (FVector2d(U.Y, U.X) * Cross + FVector2d(VB - VA, VC - VA));

	return FVector3d(Value, Gradient.X, Gradient.Y);
}

FVector3d FErosionFilter::FractalNoise(const FVector2d& P, double Frequency, int32 Octaves, double Lacunarity, double Gain)
{
	FVector3d Sum = FVector3d::ZeroVector;
	double Amplitude = 1.0;
	for (int32 i = 0; i < Octaves; ++i)
	{
		const FVector3d N = Noised(P * Frequency);
		// Sampling at P * Frequency makes the slope Frequency times steeper (chain rule)
		Sum += FVector3d(N.X, N.Y * Frequency, N.Z * Frequency) * Amplitude;
		Amplitude *= Gain;
		Frequency *= Lacunarity;
	}
	return Sum;
}

FVector4d FErosionFilter::PhacelleNoise(const FVector2d& P, const FVector2d& NormDir, double Frequency, double Offset, double Normalization)
{
	// Perpendicular to the stripes, length = angular frequency of the waves
	const FVector2d SideDir = FVector2d(-NormDir.Y, NormDir.X) * Frequency * UE_DOUBLE_TWO_PI;
	const double PhaseOffset = Offset * UE_DOUBLE_TWO_PI;

	const FVector2d PInt(FMath::FloorToDouble(P.X), FMath::FloorToDouble(P.Y));
	const FVector2d PFrac = P - PInt;

	FVector2d PhaseDir = FVector2d::ZeroVector;
	double WeightSum = 0.0;
	// 4x4 cells around P: any cell further away is at least 1.5 units from P and has zero weight
	for (int32 i = -1; i <= 2; ++i)
	{
		for (int32 j = -1; j <= 2; ++j)
		{
			const FVector2d GridOffset(i, j);
			// Cell point = grid point + random offset in [-0.5, 0.5]
			const FVector2d RandomOffset = Hash(PInt + GridOffset) * 0.5;
			// Same as P - CellPoint, but computed from the small fractional part for precision
			const FVector2d VectorFromCellPoint = PFrac - GridOffset - RandomOffset;

			// Bell-shaped weight: 1 at the cell point, 0 at distance 1.5
			const double SqrDist = VectorFromCellPoint.SizeSquared();
			const double Weight = FMath::Max(0.0, FMath::Exp(-SqrDist * 2.0) - 0.01111);
			WeightSum += Weight;

			// Phase measured from this cell's point, so each cell has its own stripe positions
			const double WaveInput = FVector2d::DotProduct(VectorFromCellPoint, SideDir) + PhaseOffset;
			PhaseDir += FVector2d(FMath::Cos(WaveInput), FMath::Sin(WaveInput)) * Weight;
		}
	}

	const FVector2d Interpolated = PhaseDir / WeightSum;
	// Blending shortens the (cos, sin) vector, scale it back towards length 1
	const double Magnitude = FMath::Max(1.0 - Normalization, Interpolated.Size());
	const FVector2d Normalized = Interpolated / Magnitude;
	return FVector4d(Normalized.X, Normalized.Y, SideDir.X, SideDir.Y);
}