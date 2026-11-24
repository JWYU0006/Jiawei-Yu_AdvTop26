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
	
	// X = height (cm), Y/Z = slope (dimensionless), at a position (cm) relative to the actor
	FVector3d GetHeightAndSlope(const FVector2D& Position) const;
	FVector GetNormal(const FVector3d& HeightAndSlope) const;
	void BuildMesh(UE::Geometry::FDynamicMesh3& Mesh) const;
	FLinearColor GetHeightColor(float Height) const;
	FLinearColor GetPhacelleDebugColor(const FVector2D& Position, const FVector3d& HeightAndSlope) const;

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
	float NoiseFrequency = 3.0f;	// Height noise amplitude, relative to Size (author's default 0.125 * 0.5)
	UPROPERTY(EditAnywhere, Category = "ErosionTest", meta = (ClampMin = "0.0"))
	float HeightAmplitude = 0.0625f;
	UPROPERTY(EditAnywhere, Category = "ErosionTest", meta = (ClampMin = "1", ClampMax = "8"))
	int32 HeightOctaves = 3;
	// Amplitude multiplier per octave, low value keeps the input smooth for erosion
	UPROPERTY(EditAnywhere, Category = "ErosionTest", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HeightGain = 0.1f;
	UPROPERTY(EditAnywhere, Category = "ErosionTest")
	int32 NoiseSeed = 0;
	UPROPERTY(EditAnywhere, Category = "ErosionTest")
	TObjectPtr<UMaterialInterface> Material;
	// Show the raw Phacelle stripes as vertex color instead of height color
	UPROPERTY(EditAnywhere, Category = "ErosionTest")
	bool bDebugPhacelle = false;

	// Derived from NoiseSeed in GenerateTerrain(), not editable
	FVector2D NoiseOffset = FVector2D::ZeroVector;
};
