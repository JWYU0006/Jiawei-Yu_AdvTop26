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
	UPROPERTY(EditAnywhere, category = "CubePlanet", meta = (ClampMin = "2", ClampMax = "256"))
	int32 CubeFaceResolution = 16;
	UPROPERTY(EditAnywhere, category = "CubePlanet", meta = (ClampMin = "1.0"))
	float PlanetRadiusSize = 5000.0f;

	// -- Event --
	UFUNCTION()
	void OnCubeClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);

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

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(CallInEditor, Category = "CubePlanet")
	void GeneratePlanet();
};
