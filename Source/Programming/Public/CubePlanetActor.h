// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/DynamicMeshComponent.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CubePlanetActor.generated.h"

// This enum needs to be before 'class PROGRAMMING_API ACubePlanetActor : public AActor'
// ECubeFace conflicts with an existing UE enumeration
enum class ECubePlanetFace
{
	CubePlanetFacePositiveX, CubePlanetFaceNegativeX,
	CubePlanetFacePositiveY, CubePlanetFaceNegativeY,
	CubePlanetFacePositiveZ, CubePlanetFaceNegativeZ
};

UCLASS()
class PROGRAMMING_API ACubePlanetActor : public AActor
{
	GENERATED_BODY()

	// Add a DynamicMeshComponent to this Actor
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDynamicMeshComponent> DynamicMeshComponent;

	// Customizable settings
	UPROPERTY(EditAnywhere, category = "CubePlanet")
	int32 CubeFaceResolution = 4;
	UPROPERTY(EditAnywhere, category = "CubePlanet")
	float PlanetSize = 1000.0f;

public:
	// Sets default values for this actor's properties
	ACubePlanetActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Function that generate a cube face based on CubeFaceResolution
	void GenerateCubeFace(ECubePlanetFace Face, FDynamicMesh3& FaceMesh);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
