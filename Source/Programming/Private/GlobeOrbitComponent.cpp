// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobeOrbitComponent.h"

// Sets default values for this component's properties
UGlobeOrbitComponent::UGlobeOrbitComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

// Called when the game starts
void UGlobeOrbitComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}
