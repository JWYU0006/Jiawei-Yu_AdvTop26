#include "ErosionTestActor.h"
#include "ErosionFilter.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

AErosionTestActor::AErosionTestActor()
{
	// Nothing to update per frame
	PrimaryActorTick.bCanEverTick = false;

	DynamicMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("DynamicMeshComponent"));
	RootComponent = DynamicMeshComponent;
	// Visual test only, skip collision cooking
	DynamicMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AErosionTestActor::BeginPlay()
{
	Super::BeginPlay();
	GenerateTerrain();
}

#if WITH_EDITOR
void AErosionTestActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Every editable property affects the terrain, so regenerate on any committed change
	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		GenerateTerrain();
	}
}
#endif

FVector3d AErosionTestActor::GetHeightAndSlope(const FVector2D& Position) const
{
	// Work in normalized space where the patch is 1 unit wide, as in the original shader
	const FVector2d P = Position / Size + NoiseOffset;
	const FVector3d Noise = FErosionFilter::FractalNoise(P, NoiseFrequency, HeightOctaves, 2.0, HeightGain) * HeightAmplitude;

	// Back to cm: height scales with Size, slope (height / distance) stays the same
	return FVector3d(Noise.X * Size, Noise.Y, Noise.Z);
}

FVector AErosionTestActor::GetNormal(const FVector3d& HeightAndSlope) const
{
	// Up minus slope: perpendicular to the surface
	return FVector(-HeightAndSlope.Y, -HeightAndSlope.Z, 1.0).GetSafeNormal();
}

void AErosionTestActor::GenerateTerrain()
{
	FRandomStream RandomStream(NoiseSeed);
	NoiseOffset = FVector2D(RandomStream.FRandRange(-1000.0f, 1000.0f), RandomStream.FRandRange(-1000.0f, 1000.0f));

	FDynamicMesh3 Mesh;
	BuildMesh(Mesh);

	DynamicMeshComponent->SetMesh(MoveTemp(Mesh));
	DynamicMeshComponent->SetMaterial(0, Material);
}

void AErosionTestActor::BuildMesh(UE::Geometry::FDynamicMesh3& Mesh) const
{
	// EnableAttributes() only adds UV and normal layers, color must be enabled separately
	Mesh.EnableAttributes();
	Mesh.Attributes()->EnablePrimaryColors();
	UE::Geometry::FDynamicMeshUVOverlay* UVOverlay = Mesh.Attributes()->PrimaryUV();
	UE::Geometry::FDynamicMeshNormalOverlay* NormalOverlay = Mesh.Attributes()->PrimaryNormals();
	UE::Geometry::FDynamicMeshColorOverlay* ColorOverlay = Mesh.Attributes()->PrimaryColors();

	const double StepSize = Size / Resolution;
	const double HalfSize = Size * 0.5;

	// Vertex and all overlay elements are appended in lockstep, so they share index y * (Resolution + 1) + x
	for (int32 y = 0; y <= Resolution; ++y)
	{
		for (int32 x = 0; x <= Resolution; ++x)
		{
			const FVector2D Position(-HalfSize + x * StepSize, -HalfSize + y * StepSize);
			const FVector3d HeightAndSlope = GetHeightAndSlope(Position);

			Mesh.AppendVertex(FVector3d(Position.X, Position.Y, HeightAndSlope.X));
			UVOverlay->AppendElement(FVector2f(x, y));
			NormalOverlay->AppendElement(FVector3f(GetNormal(HeightAndSlope)));
			const FLinearColor Color = bDebugPhacelle ? GetPhacelleDebugColor(Position, HeightAndSlope) : GetHeightColor(HeightAndSlope.X);
			ColorOverlay->AppendElement(FVector4f(Color));
		}
	}

	for (int32 y = 0; y < Resolution; ++y)
	{
		for (int32 x = 0; x < Resolution; ++x)
		{
			const int32 BottomLeft = y * (Resolution + 1) + x;
			const int32 BottomRight = BottomLeft + 1;
			const int32 TopLeft = BottomLeft + Resolution + 1;
			const int32 TopRight = TopLeft + 1;

			// Winding order makes the front side face +Z
			const UE::Geometry::FIndex3i Triangles[2] = {
				UE::Geometry::FIndex3i(BottomLeft, TopLeft, BottomRight),
				UE::Geometry::FIndex3i(BottomRight, TopLeft, TopRight)
			};
			for (const UE::Geometry::FIndex3i& Triangle : Triangles)
			{
				const int32 TriangleID = Mesh.AppendTriangle(Triangle);
				UVOverlay->SetTriangle(TriangleID, Triangle);
				NormalOverlay->SetTriangle(TriangleID, Triangle);
				ColorOverlay->SetTriangle(TriangleID, Triangle);
			}
		}
	}
}

FLinearColor AErosionTestActor::GetHeightColor(float Height) const
{
	const float MaxHeight = HeightAmplitude * Size;
	if (MaxHeight <= 0.0f)
	{
		return FLinearColor(0.5f, 0.5f, 0.5f);
	}
	const float Alpha = FMath::Clamp(Height / MaxHeight * 0.5f + 0.5f, 0.0f, 1.0f);
	return FMath::Lerp(FLinearColor(0.15f, 0.15f, 0.15f), FLinearColor::White, Alpha);
}

FLinearColor AErosionTestActor::GetPhacelleDebugColor(const FVector2D& Position, const FVector3d& HeightAndSlope) const
{
	// Author's defaults: erosion scale 0.15, cell scale 0.7, so cells are about 1/10 of the patch
	const double CellScale = 0.7;
	const double CellFrequency = 1.0 / (0.15 * CellScale);
	const FVector2d P = Position / Size * CellFrequency;

	// Stripes should run downhill, the opposite of the slope (which points uphill)
	const FVector2d Downhill = -FVector2d(HeightAndSlope.Y, HeightAndSlope.Z);
	const FVector4d Phacelle = FErosionFilter::PhacelleNoise(P, Downhill.GetSafeNormal(), CellScale, 0.25, 0.5);

	// Cosine wave from [-1, 1] to [0, 1]
	const float Gray = Phacelle.X * 0.5 + 0.5;
	return FLinearColor(Gray, Gray, Gray);
}
