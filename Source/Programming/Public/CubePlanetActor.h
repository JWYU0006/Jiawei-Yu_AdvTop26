// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/DynamicMeshComponent.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CubePlanetActor.generated.h"

// This enum needs to be before 'class PROGRAMMING_API ACubePlanetActor : public AActor'
enum class ECubePlanetFace // ECubeFace conflicts with an existing UE enumeration
{
	CubePlanetFacePositiveX, CubePlanetFaceNegativeX,
	CubePlanetFacePositiveY, CubePlanetFaceNegativeY,
	CubePlanetFacePositiveZ, CubePlanetFaceNegativeZ
};

UCLASS()
class PROGRAMMING_API ACubePlanetActor : public AActor
{
	GENERATED_BODY()

	// -- Initialization --
	// Add a DynamicMeshComponent to this Actor
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDynamicMeshComponent> DynamicMeshComponent;

	// -- Customizable settings --
	UPROPERTY(EditAnywhere, category = "CubePlanet")
	int32 CubeFaceResolution = 16;
	// Length of the edge of the cube and diameter of the planet
	UPROPERTY(EditAnywhere, category = "CubePlanet")
	// planet radius
	float PlanetSize = 500.0f;

	// -- Event --
	UFUNCTION()
	void OnCubeClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed) const;

public:
	// Sets default values for this actor's properties
	ACubePlanetActor();

	// Only for collision test
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Declare a global PlayerController variable
	UPROPERTY()
	APlayerController* PlayerController = nullptr;

	// Function that generate a cube face based on CubeFaceResolution
	void GenerateCubeFace(ECubePlanetFace Face, FDynamicMesh3& FaceMesh) const;
};
