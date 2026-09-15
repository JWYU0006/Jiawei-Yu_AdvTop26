// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobeOrbitComponent.h"
#include "components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"

// Called when the game starts
void UGlobeOrbitComponent::BeginPlay()
{
	Super::BeginPlay();
	const FDateTime Now = FDateTime::Now();
	UE_LOG(
		LogTemp, Warning, TEXT("Game Run | %s"), *Now.ToString(TEXT("%Y-%m-%d %H:%M:%S"))
	);

	AActor* Owner = GetOwner();

	TArray<USceneComponent*> SceneComponents;
	Owner->GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* sc : SceneComponents)
	{
		if (sc->ComponentHasTag(TEXT("OrbitPivot")))
		{
			OrbitPivot = sc;
			break;
		}
	}

	TArray<USpringArmComponent*> SpringArmComponents;
	Owner->GetComponents<USpringArmComponent>(SpringArmComponents);

	for (USpringArmComponent* sac : SpringArmComponents)
	{
		if (sac->ComponentHasTag(TEXT("OrbitSpringArm")))
		{
			SpringArm = sac;
			break;
		}
	}

	if (OrbitPivot)
	{
		UE_LOG(LogTemp, Warning, TEXT("OrbitPivot found"));
	}

	if (SpringArm)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpringArm found"));
	}
}
