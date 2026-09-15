// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GlobeOrbitComponent.generated.h"

class USceneComponent;
class USpringArmComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROGRAMMING_API UGlobeOrbitComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGlobeOrbitComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	USceneComponent* OrbitPivot = nullptr;

	UPROPERTY()
	USpringArmComponent* SpringArm = nullptr;
};
