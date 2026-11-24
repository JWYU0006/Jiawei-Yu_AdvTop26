// Port of runevision's "Advanced Terrain Erosion Filter": https://www.shadertoy.com/view/wXcfWn
// Phacelle Noise and ErosionFilter copyright (c) 2025 Rune Skovbo Johansen.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
// Noised: gradient noise with analytical derivatives by Inigo Quilez, https://www.shadertoy.com/view/XdXBRH

#pragma once

#include "CoreMinimal.h"

// Stateless math functions. Horizontal coordinates and heights must use the same units.
// Height-and-slope vectors: X = height, Y = dh/dx, Z = dh/dy
class PROGRAMMING_API FErosionFilter
{
public:
	// Gradient noise, height in about [-1, 1], with exact derivatives
	static FVector3d Noised(const FVector2d& P);

	// Several octaves of Noised, derivatives included
	static FVector3d FractalNoise(const FVector2d& P, double Frequency, int32 Octaves, double Lacunarity, double Gain);
	
	// Stripes aligned with NormDir (must be normalized), blended from 4x4 jittered cells.
	// Returns X = cosine wave, Y = sine wave (both normalized),
	// ZW = side direction, multiply it onto Y to get the derivative of X
	static FVector4d PhacelleNoise(const FVector2d& P, const FVector2d& NormDir, double Frequency, double Offset, double Normalization);

private:
	// Pseudo-random vector in [-1, 1] for a grid point
	static FVector2d Hash(const FVector2d& X);
};