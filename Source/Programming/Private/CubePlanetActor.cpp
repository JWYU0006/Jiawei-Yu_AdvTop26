// Fill out your copyright notice in the Description page of Project Settings.

#include "CubePlanetActor.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DrawDebugHelpers.h"
#include "ShaderCompiler.h"
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
		PropertyName == GET_MEMBER_NAME_CHECKED(ACubePlanetActor, PlanetRadiusSize))
	{
		GeneratePlanet();
	}
}
#endif

void ACubePlanetActor::GeneratePlanet()
{
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

// Called when the game starts or when spawned
void ACubePlanetActor::BeginPlay()
{
	Super::BeginPlay();

	GeneratePlanet();

	// Bind event OnCubeClicked to this Actor
	// DynamicMeshComponent->OnClicked.AddDynamic(this, &ACubePlanetActor::OnCubeClicked);

	// Code below should be in a dedicated PlayerController, here is for quick test.
	// PlayerController = GetWorld()->GetFirstPlayerController();
	// if (PlayerController)
	// {
	// 	PlayerController->bShowMouseCursor = true;
	// 	PlayerController->bEnableClickEvents = true;
	// }
}

// Called every frame
void ACubePlanetActor::Tick(float DeltaTime)
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

	// Draw a planetary reference debug sphere
	// DrawDebugSphere(GetWorld(), FVector::Zero(), PlanetRadiusSize, 32, FColor::Green, true);
}

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
	// Vertices need to be one more than the resolution, so use "<=" in for loops
	// `++x` is the standard practice and is preferable to `x++`.
	for (int32 y = 0; y <= CubeFaceResolution; ++y)
	{
		for (int32 x = 0; x <= CubeFaceResolution; ++x)
		{
			// `static_cast` is the standard approach; in this simple use case, using `(float)` directly makes no difference.
			// t in [-1, 1], evenly spaced
			float u = static_cast<float>(x) / CubeFaceResolution * 2.0f - 1.0f;
			float v = static_cast<float>(y) / CubeFaceResolution * 2.0f - 1.0f;
			// Tangent warp: make the angle (not the position on the cube) evenly spaced, so the sphere is more uniform
			u = FMath::Tan(u * UE_PI / 4.0f);
			v = FMath::Tan(v * UE_PI / 4.0f);
			// When an axis is used as the normal and the positive direction of the axis points toward the camera,
			// -the vertices are always arranged from bottom-right to top-left.
			FVector Vertex = FaceNormal + AxisA * u + AxisB * v;
			// Normalize vertex to spherize
			Vertex = Vertex.GetSafeNormal() * PlanetRadiusSize;
			Vertices.Add(Vertex);
			// One grid cell per quad: UV of vertex (x, y) is (x, y), no need to normalize to 0-1.
			UVs.Add(FVector2f(x, y));
		}
	}

	TArray<int32> TrianglesVerticesIndex;
	// 0 1 2 3 4 // Triangles are (0, 1, 5), (1, 6, 5), etc.
	// 5 6 7 8 9 //
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

	UE::Geometry::FDynamicMeshUVOverlay* UVOverlay = FaceMesh.Attributes()->PrimaryUV();
	UE::Geometry::FDynamicMeshNormalOverlay* NormalOverlay = FaceMesh.Attributes()->PrimaryNormals();
	TArray<int32> UVElementIndexes;
	TArray<int32> NormalElementIndexes;

	// Without vertex indexing, 'AppendTriangle' starts from zero each time it runs, resulting in the same face being generated six times.
	TArray<int32> MeshVerticesIndexes;

	// Standard coding practice: use `const` to make the value constant and `&` for only pass-by-reference.
	// for (const FVector& Vertex : Vertices)
	// {
	// 	int32 VertexIndex = FaceMesh.AppendVertex(FVector3d(Vertex));
	// 	MeshVerticesIndexes.Add(VertexIndex);
	// }

	// change to this type of for loop to add uv and normal
	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		MeshVerticesIndexes.Add(FaceMesh.AppendVertex(Vertices[i]));
		UVElementIndexes.Add(UVOverlay->AppendElement(UVs[i]));
		NormalElementIndexes.Add(NormalOverlay->AppendElement(FVector3f(Vertices[i].GetSafeNormal())));
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
