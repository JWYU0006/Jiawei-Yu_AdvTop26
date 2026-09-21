// Fill out your copyright notice in the Description page of Project Settings.

#include "CubePlanetActor.h"

#include <ThirdParty/ShaderConductor/ShaderConductor/External/DirectXShaderCompiler/include/dxc/DXIL/DxilConstants.h>

#include "SkeletonTreeBuilder.h"
#include "DynamicMesh/DynamicMesh3.h"

// Sets default values
ACubePlanetActor::ACubePlanetActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	DynamicMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(
		TEXT("DynamicMeshComponent")
	);

	RootComponent = DynamicMeshComponent;
}

// Called when the game starts or when spawned
void ACubePlanetActor::BeginPlay()
{
	Super::BeginPlay();

	GenerateCubeFace();
}

// Called every frame
void ACubePlanetActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACubePlanetActor::GenerateCubeFace()
{
	TArray<FVector> Vertices;
	TArray<int32> TrianglesVerticesIndex;

	//Vertices need to be one more than the resolution, so use "<=" in for loops
	//`++x` is the standard practice and is preferable to `x++`.
	for (int32 y = 0; y <= CubeFaceResolution; ++y)
	{
		for (int32 x = 0; x <= CubeFaceResolution; ++x)
		{
			//`static_cast` is the standard approach; in this simple use case, using `(float)` directly makes no difference.
			float u = static_cast<float>(x) / CubeFaceResolution;
			float v = static_cast<float>(y) / CubeFaceResolution;
			//Multiply 'PlanetSize'
			FVector Vertex{0.0f, (u - 0.5f) * PlanetSize, (v - 0.5f) * PlanetSize};
			Vertices.Add(Vertex);
		}
	}

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

	FDynamicMesh3 FaceMesh;
	//Standard coding practice: use `const` to make the value constant and `&` for only pass-by-reference.
	for (const FVector& Vertex : Vertices)
	{
		FaceMesh.AppendVertex(FVector3d(Vertex));
	}

	//Three indexes are retrieved from the `TrianglesVerticesIndex` array to serve as the vertices of a triangle
	//using a `for` loop with an increment of 3 allows to reach the indexes for the next triangle
	for (int32 i = 0; i < TrianglesVerticesIndex.Num(); i += 3)
	{
		FaceMesh.AppendTriangle(TrianglesVerticesIndex[i], TrianglesVerticesIndex[i + 1],
		                        TrianglesVerticesIndex[i + 2]);
	}

	//void SetMesh ( UE::Geometry::FDynamicMesh3&& MoveMesh) -- UE Document -- UDynamicMesh
	DynamicMeshComponent->SetMesh(MoveTemp(FaceMesh));
}
