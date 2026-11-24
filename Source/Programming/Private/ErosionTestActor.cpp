#include "ErosionTestActor.h"
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

float AErosionTestActor::GetHeight(const FVector2D& Position) const
{
	// Noise space: frequency sets how much of the noise is shown across the patch, offset (from seed) picks where
	const FVector2D NormalizedPosition = Position / Size;
	const FVector2D SamplePoint = NormalizedPosition * NoiseFrequency + NoiseOffset;
	return FMath::PerlinNoise2D(SamplePoint) * HeightScale;
}

FVector AErosionTestActor::GetNormal(const FVector2D& Position) const
{
	// Small relative to vertex spacing, so the slope reflects the surface between vertices
	const double Epsilon = Size / Resolution * 0.25;
	const FVector2D StepX(Epsilon, 0.0);
	const FVector2D StepY(0.0, Epsilon);

	// Central difference: height change across a small step on each side
	const double SlopeX = (GetHeight(Position + StepX) - GetHeight(Position - StepX)) / (2.0 * Epsilon);
	const double SlopeY = (GetHeight(Position + StepY) - GetHeight(Position - StepY)) / (2.0 * Epsilon);

	// Up minus slope: perpendicular to the surface
	return FVector(-SlopeX, -SlopeY, 1.0).GetSafeNormal();
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
			const float Height = GetHeight(Position);

			Mesh.AppendVertex(FVector3d(Position.X, Position.Y, Height));
			UVOverlay->AppendElement(FVector2f(x, y));
			NormalOverlay->AppendElement(FVector3f(GetNormal(Position)));
			ColorOverlay->AppendElement(FVector4f(GetHeightColor(Height)));
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
	// Map height from [-HeightScale, HeightScale] to [0, 1]
	const float Alpha = FMath::Clamp(Height / HeightScale * 0.5f + 0.5f, 0.0f, 1.0f);
	return FMath::Lerp(FLinearColor(0.15f, 0.15f, 0.15f), FLinearColor::White, Alpha);
}