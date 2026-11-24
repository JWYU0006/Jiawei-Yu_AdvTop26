#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/DynamicMeshComponent.h"
#include "ErosionTestActor.generated.h"

// A flat terrain patch for testing the 2D erosion filter before porting it to the planet
UCLASS()
class PROGRAMMING_API AErosionTestActor : public AActor
{
	GENERATED_BODY()

public:
	AErosionTestActor();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void BeginPlay() override;

	UFUNCTION(CallInEditor, Category = "ErosionTest")
	void GenerateTerrain();

	// Height (cm) at a position (cm) relative to the actor
	float GetHeight(const FVector2D& Position) const;
	FVector GetNormal(const FVector2D& Position) const;
	void BuildMesh(UE::Geometry::FDynamicMesh3& Mesh) const;
	FLinearColor GetHeightColor(float Height) const;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDynamicMeshComponent> DynamicMeshComponent;

	// Number of quads along each side
	UPROPERTY(EditAnywhere, Category = "ErosionTest", meta = (ClampMin = "2", ClampMax = "512"))
	int32 Resolution = 256;
	// Side length of the square patch (cm)
	UPROPERTY(EditAnywhere, Category = "ErosionTest", meta = (ClampMin = "1.0"))
	float Size = 10000.0f;
	// Roughly how many hills across the patch
	UPROPERTY(EditAnywhere, Category = "ErosionTest", meta = (ClampMin = "0.0"))
	float NoiseFrequency = 3.0f;
	UPROPERTY(EditAnywhere, Category = "ErosionTest", meta = (ClampMin = "0.0"))
	float HeightScale = 1500.0f;
	UPROPERTY(EditAnywhere, Category = "ErosionTest")
	int32 NoiseSeed = 0;
	UPROPERTY(EditAnywhere, Category = "ErosionTest")
	TObjectPtr<UMaterialInterface> Material;

	// Derived from NoiseSeed in GenerateTerrain(), not editable
	FVector2D NoiseOffset = FVector2D::ZeroVector;
};
