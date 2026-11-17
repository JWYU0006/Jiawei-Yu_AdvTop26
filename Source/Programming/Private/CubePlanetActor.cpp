// Fill out your copyright notice in the Description page of Project Settings.

#include "CubePlanetActor.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DrawDebugHelpers.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

// Sets default values
ACubePlanetActor::ACubePlanetActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DynamicMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(
		TEXT("DynamicMeshComponent")
	);
	RootComponent = DynamicMeshComponent;

	// Set collision, accept mouse click
	DynamicMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DynamicMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	DynamicMeshComponent->EnableComplexAsSimpleCollision();
}

// Update in Engine Editor
#if WITH_EDITOR
void ACubePlanetActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Skip while dragging the slider, only regenerate when the value is committed
	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ACubePlanetActor, CubeFaceResolution) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACubePlanetActor, PlanetRadiusSize) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACubePlanetActor, NoiseFrequency) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACubePlanetActor, NoiseAmplitude) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ACubePlanetActor, NoiseSeed))
	{
		GeneratePlanet();
	}
}
#endif

// Called when the game starts or when spawned
void ACubePlanetActor::BeginPlay()
{
	Super::BeginPlay();
	GeneratePlanet();
}

void ACubePlanetActor::GeneratePlanet()
{
	// Same seed always gives the same offset, so the same planet by using RandomStream
	FRandomStream RandomStream(NoiseSeed);
	NoiseOffset = FVector(RandomStream.FRandRange(-1000.0f, 1000.0f),
	                      RandomStream.FRandRange(-1000.0f, 1000.0f),
	                      RandomStream.FRandRange(-1000.0f, 1000.0f));
	// Declare FaceMesh and generate all 6 cube faces
	FDynamicMesh3 FaceMesh;
	// Enable UV and normal overlays
	FaceMesh.EnableAttributes();
	GenerateCubeFace(ECubePlanetFace::CubePlanetFacePositiveX, FaceMesh);
	GenerateCubeFace(ECubePlanetFace::CubePlanetFaceNegativeX, FaceMesh);
	GenerateCubeFace(ECubePlanetFace::CubePlanetFacePositiveY, FaceMesh);
	GenerateCubeFace(ECubePlanetFace::CubePlanetFaceNegativeY, FaceMesh);
	GenerateCubeFace(ECubePlanetFace::CubePlanetFacePositiveZ, FaceMesh);
	GenerateCubeFace(ECubePlanetFace::CubePlanetFaceNegativeZ, FaceMesh);
	// void SetMesh ( UE::Geometry::FDynamicMesh3&& MoveMesh) -- UE Document -- UDynamicMesh
	DynamicMeshComponent->SetMesh(MoveTemp(FaceMesh));
	DynamicMeshComponent->SetMaterial(0, PlanetMaterial);
	DynamicMeshComponent->UpdateCollision(false);
}

// Called every frame
/*void ACubePlanetActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	GEngine->AddOnScreenDebugMessage(0, 0.0f, FColor::White, TEXT("Tick Running"));

	// Custom hit test
	if (!PlayerController) return;
	if (FHitResult HitResult; PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		GEngine->AddOnScreenDebugMessage(
			1, 0.0f, FColor::Green,
			FString::Printf(
				TEXT("Hit: %s"),
				*GetNameSafe(HitResult.GetActor())
			)
		);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Red, TEXT("No Hit"));
	}
}*/

void ACubePlanetActor::GenerateCubeFace(ECubePlanetFace Face, FDynamicMesh3& FaceMesh) const
{
	FVector FaceNormal, AxisA, AxisB;
	switch (Face)
	{
	case ECubePlanetFace::CubePlanetFacePositiveX:
		FaceNormal = FVector(1, 0, 0);
		AxisA = FVector(0, 1, 0);
		AxisB = FVector(0, 0, 1);
		break;
	case ECubePlanetFace::CubePlanetFaceNegativeX:
		FaceNormal = FVector(-1, 0, 0);
		AxisA = FVector(0, -1, 0);
		AxisB = FVector(0, 0, 1);
		break;
	case ECubePlanetFace::CubePlanetFacePositiveY:
		FaceNormal = FVector(0, 1, 0);
		AxisA = FVector(-1, 0, 0);
		AxisB = FVector(0, 0, 1);
		break;
	case ECubePlanetFace::CubePlanetFaceNegativeY:
		FaceNormal = FVector(0, -1, 0);
		AxisA = FVector(1, 0, 0);
		AxisB = FVector(0, 0, 1);
		break;
	case ECubePlanetFace::CubePlanetFacePositiveZ:
		FaceNormal = FVector(0, 0, 1);
		AxisA = FVector(0, 1, 0);
		AxisB = FVector(-1, 0, 0);
		break;
	case ECubePlanetFace::CubePlanetFaceNegativeZ:
		FaceNormal = FVector(0, 0, -1);
		AxisA = FVector(0, 1, 0);
		AxisB = FVector(1, 0, 0);
		break;
	}

	TArray<FVector> Vertices;
	TArray<FVector2f> UVs;
	TArray<FVector> Normals;
	for (int32 y = 0; y <= CubeFaceResolution; ++y)
	{
		for (int32 x = 0; x <= CubeFaceResolution; ++x)
		{
			float u = static_cast<float>(x) / CubeFaceResolution * 2.0f - 1.0f;
			float v = static_cast<float>(y) / CubeFaceResolution * 2.0f - 1.0f;
			// Tangent warp: make the angle (not the position on the cube) evenly spaced, so the sphere is more uniform
			u = FMath::Tan(u * UE_PI / 4.0f);
			v = FMath::Tan(v * UE_PI / 4.0f);
			// When axis points toward the camera, the vertices are always arranged from bottom-right to top-left.
			const FVector Direction = (FaceNormal + AxisA * u + AxisB * v).GetSafeNormal();
			FVector Vertex = Direction * (PlanetRadiusSize + GetTerrainHeight(Direction));
			Vertices.Add(Vertex);
			// One grid cell per quad: UV of vertex (x, y) is (x, y), no need to normalize to 0-1.
			UVs.Add(FVector2f(x, y));
			Normals.Add(GetTerrainNormal(Direction));
		}
	}

	TArray<int32> TrianglesVerticesIndex;
	for (int32 y = 0; y < CubeFaceResolution; ++y)
	{
		for (int32 x = 0; x < CubeFaceResolution; ++x)
		{
			int32 i = y * (CubeFaceResolution + 1) + x;
			TrianglesVerticesIndex.Add(i);
			TrianglesVerticesIndex.Add(i + CubeFaceResolution + 1);
			TrianglesVerticesIndex.Add(i + 1);
			TrianglesVerticesIndex.Add(i + 1);
			TrianglesVerticesIndex.Add(i + CubeFaceResolution + 1);
			TrianglesVerticesIndex.Add(i + CubeFaceResolution + 2);
		}
	}

	// Without vertex indexing, 'AppendTriangle' starts from zero each time it runs, resulting in the same face being generated six times.
	TArray<int32> MeshVerticesIndexes;
	UE::Geometry::FDynamicMeshUVOverlay* UVOverlay = FaceMesh.Attributes()->PrimaryUV();
	UE::Geometry::FDynamicMeshNormalOverlay* NormalOverlay = FaceMesh.Attributes()->PrimaryNormals();
	TArray<int32> UVElementIndexes;
	TArray<int32> NormalElementIndexes;
	// change to this type of for loop to add uv and normal
	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		MeshVerticesIndexes.Add(FaceMesh.AppendVertex(Vertices[i]));
		UVElementIndexes.Add(UVOverlay->AppendElement(UVs[i]));
		// NormalElementIndexes.Add(NormalOverlay->AppendElement(FVector3f(Vertices[i].GetSafeNormal())));
		NormalElementIndexes.Add(NormalOverlay->AppendElement(FVector3f(Normals[i].GetSafeNormal())));
	}

	// Three indexes are retrieved from the `TrianglesVerticesIndex` array to serve as the vertices of a triangle
	// Using a `for` loop with an increment of 3 allows to reach the indexes for the next triangle
	for (int32 i = 0; i < TrianglesVerticesIndex.Num(); i += 3)
	{
		const int32 A = TrianglesVerticesIndex[i];
		const int32 B = TrianglesVerticesIndex[i + 1];
		const int32 C = TrianglesVerticesIndex[i + 2];
		const int32 TriangleID = FaceMesh.AppendTriangle(MeshVerticesIndexes[A], MeshVerticesIndexes[B], MeshVerticesIndexes[C]);

		UVOverlay->SetTriangle(TriangleID, UE::Geometry::FIndex3i(UVElementIndexes[A], UVElementIndexes[B], UVElementIndexes[C]));
		NormalOverlay->SetTriangle(TriangleID, UE::Geometry::FIndex3i(NormalElementIndexes[A], NormalElementIndexes[B], NormalElementIndexes[C]));
	}
}

float ACubePlanetActor::GetTerrainHeight(const FVector& Direction) const
{
	const FVector SamplePoint = Direction * NoiseFrequency + NoiseOffset;
	return FMath::PerlinNoise3D(SamplePoint) * NoiseAmplitude;
}

FVector ACubePlanetActor::GetTerrainNormal(const FVector& Direction) const
{
	// Step size for finite difference, in unit-sphere space
	constexpr double Epsilon = 0.001;
	const FVector StepX(Epsilon, 0.0, 0.0);
	const FVector StepY(0.0, Epsilon, 0.0);
	const FVector StepZ(0.0, 0.0, Epsilon);
	// 1. 3D gradient of the height, by central difference along each axis
	const FVector Gradient(
		(GetTerrainHeight(Direction + StepX) - GetTerrainHeight(Direction - StepX)) / (2.0 * Epsilon),
		(GetTerrainHeight(Direction + StepY) - GetTerrainHeight(Direction - StepY)) / (2.0 * Epsilon),
		(GetTerrainHeight(Direction + StepZ) - GetTerrainHeight(Direction - StepZ)) / (2.0 * Epsilon)
	);
	// 2. Remove the radial part, keep only the slope along the surface
	const FVector SurfaceGradient = Gradient - FVector::DotProduct(Gradient, Direction) * Direction;
	// 3. Gradient is height change per radian, divide by radius to get height change per cm  
	const double Radius = PlanetRadiusSize + GetTerrainHeight(Direction);
	// 4. Normal = up - slope
	return (Direction - SurfaceGradient / Radius).GetSafeNormal();
}

void ACubePlanetActor::OnCubeClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Cube clicked"));
	if (PlayerController)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("PlayerController found"));

		PlayerController->bShowMouseCursor = true;
		PlayerController->bEnableClickEvents = true;
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("PlayerController NOT found"));
		return;
	}
	FHitResult HitResult;
	if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 25, 12, FColor::Red, false, 2);
	}
}
