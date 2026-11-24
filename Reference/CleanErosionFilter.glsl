// Source: "Clean Terrain Erosion Filter" by Rune Skovbo Johansen (runevision)
// https://www.shadertoy.com/view/33cXW8
// Retrieved 2026-11-23. Only the erosion-related parts are kept (Buffer A + hash/noised from Common).
// This is the earlier (2025) version. The Advanced version (AdvancedErosionFilter.glsl) replaces
// Gullies with Phacelle noise and adds fading, detail, rounding, onset and a ridge map.
//
// Copyright 2025 Rune Skovbo Johansen
// Copyright 2023 Fewes
// Copyright 2020 Clay John
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#define PI 3.14159265358979

// Erosion parameters (defaults from the Common tab)
#define EROSION_SCALE 0.08333        // Try the width of a mountain divided by five to ten.
#define EROSION_STRENGTH 0.16        // Gully depth relative to scale, 0 to 0.5 usually good.
#define EROSION_SLOPE_POWER 0.6      // 1.0: smooth peaks and ridges, 0.5: sharper.
#define EROSION_CELL_SCALE 1.0       // Smaller: grainy gullies, larger: longer unbroken gullies.
#define EROSION_HEIGHT_OFFSET -0.5   // -1: only lowers terrain, 1: only raises.
#define EROSION_OCTAVES 5
#define EROSION_GAIN 0.5
#define EROSION_LACUNARITY 2.0

// Base height noise parameters
#define HEIGHT_TILES 3.0
#define HEIGHT_OCTAVES 3
#define HEIGHT_AMP 0.25
#define HEIGHT_GAIN 0.1
#define HEIGHT_LACUNARITY 2.0
#define WATER_HEIGHT 0.465

vec2 hash(in vec2 x) {
    const vec2 k = vec2(0.3183099, 0.3678794);
    x = x * k + k.yx;
    return -1.0 + 2.0 * fract(16.0 * k * fract(x.x * x.y * (x.x + x.y)));
}

// Gradient noise with analytical derivatives, from https://www.shadertoy.com/view/XdXBRH
vec3 noised(in vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f*f*f*(f*(f*6.0-15.0)+10.0);
    vec2 du = 30.0*f*f*(f*(f-2.0)+1.0);
    vec2 ga = hash(i + vec2(0.0, 0.0));
    vec2 gb = hash(i + vec2(1.0, 0.0));
    vec2 gc = hash(i + vec2(0.0, 1.0));
    vec2 gd = hash(i + vec2(1.0, 1.0));
    float va = dot(ga, f - vec2(0.0, 0.0));
    float vb = dot(gb, f - vec2(1.0, 0.0));
    float vc = dot(gc, f - vec2(0.0, 1.0));
    float vd = dot(gd, f - vec2(1.0, 1.0));
    return vec3(va + u.x*(vb-va) + u.y*(vc-va) + u.x*u.y*(va-vb-vc+vd),
        ga + u.x*(gb-ga) + u.y*(gc-ga) + u.x*u.y*(ga-gb-gc+gd) +
        du * (u.yx*(va-vb-vc+vd) + vec2(vb,vc) - va));
}

// Produce gullies (height offset and slope/derivatives) orthogonal to the input slope.
// Steeper slopes produce more high frequency gullies. The amplitude is always one.
// The sine waves of all cells are along the same direction: orthogonal to the slope direction,
// meaning the wave stripes run along the slope. Both frequency and phase are relative to the
// cell center, so that small changes in slope only produce small changes in output.
vec3 Gullies(in vec2 p, vec2 slope) {
    // Vector orthogonal to the slope with proportional magnitude, pre-multiplied by 2 pi.
    vec2 sideDir = slope.yx * vec2(-1.0, 1.0) * 2.0 * PI;
    vec2 pInt = floor(p);
    vec2 pFrac = fract(p);
    vec3 heightAndSlope = vec3(0.0);
    float weightSum = 0.0;
    for (int i = -1; i <= 2; i++) {
        for (int j = -1; j <= 2; j++) {
            vec2 gridOffset = vec2(i, j);
            vec2 gridPoint = pInt + gridOffset;
            // Random offset for the cell point between -0.5 and 0.5 on each axis.
            vec2 randomOffset = hash(gridPoint) * 0.5;
            // p - (gridPoint + randomOffset) = pFrac - gridOffset - randomOffset
            vec2 vectorFromCellPoint = pFrac - gridOffset - randomOffset;
            // Bell-shaped weight, 1 at dist 0 and 0 at dist 1.5.
            float sqrDist = dot(vectorFromCellPoint, vectorFromCellPoint);
            float weight = max(0.0, exp(-sqrDist * 2.0) - 0.01111);
            weightSum += weight;
            float waveInput = dot(vectorFromCellPoint, sideDir);
            // cos() for height: at peaks slope is zero so this is the wave maximum,
            // meaning peaks "get no erosion".
            heightAndSlope += vec3(
                cos(waveInput),           // Amplitude of the wave
                -sin(waveInput) * sideDir // Analytical derivative of the wave
            ) * weight;
        }
    }
    return heightAndSlope / weightSum;
}

// Over several iterations, gullies are added as sine wave stripes that run parallel to the
// terrain slope. The gullies modify the slope, causing the next iteration of gullies to branch
// out at an angle to the previous ones.
vec3 Erosion(
    in vec2 p, vec3 heightAndSlope, float scale, float strength, float slopePower,
    float cellScale, int octaves, float gain, float lacunarity
) {
    vec3 inputHeightAndSlope = heightAndSlope;
    float freq = 1.0 / (scale * cellScale);
    strength *= scale;
    for (int i = 0; i < octaves; i++) {
        // Equivalent to raising the length of the slope to slopePower.
        float sqrLen = dot(heightAndSlope.yz, heightAndSlope.yz);
        vec2 inputSlope = heightAndSlope.yz * pow(sqrLen, 0.5 * (slopePower - 1.0));
        heightAndSlope += Gullies(p * freq, inputSlope * cellScale)
            * strength * vec3(1, freq, freq);
        strength *= gain;
        freq *= lacunarity;
    }
    return heightAndSlope - inputHeightAndSlope;
}

vec3 FractalNoise(vec2 p, float freq, int octaves, float lacunarity, float gain) {
    vec3 n = vec3(0.0);
    float nf = freq;
    float na = 1.0;
    for (int i = 0; i < octaves; i++) {
        n += noised(p * nf) * na * vec3(1.0, nf, nf);
        na *= gain;
        nf *= lacunarity;
    }
    return n;
}

// Geometric series: extents of layered noise with first layer magnitude 1.
float MagnitudeSum(int octaves, float gain) {
    return (1.0 - pow(gain, float(octaves))) / (1.0 - gain);
}

// p is in [0, 1] across the map. Returns (height, erosion mask for coloring).
vec2 Heightmap(vec2 p) {
    vec3 n = FractalNoise(p, HEIGHT_TILES, HEIGHT_OCTAVES, HEIGHT_LACUNARITY, HEIGHT_GAIN) * HEIGHT_AMP;
    // [-1, 1] -> [0, 1]
    n = n * 0.5 + vec3(0.5, 0, 0);
    float strength = EROSION_STRENGTH;
    // Fade erosion out near the water
    strength *= smoothstep(WATER_HEIGHT - 0.1, WATER_HEIGHT + 0.1, n.x);
    int octaves = EROSION_OCTAVES;
    vec3 h = Erosion(p, n, EROSION_SCALE, strength,
        EROSION_SLOPE_POWER, EROSION_CELL_SCALE,
        octaves, EROSION_GAIN, EROSION_LACUNARITY);
    float erosionMagnitude = EROSION_SCALE * strength * MagnitudeSum(octaves, EROSION_GAIN);
    float offset = erosionMagnitude * EROSION_HEIGHT_OFFSET;
    return vec2(n.x + h.x + offset, h.x / erosionMagnitude);
}
