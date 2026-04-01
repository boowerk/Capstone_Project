// Fill out your copyright notice in the Description page of Project Settings.


// Sets default values

#include "Actors/GP_WaterPuddle.h"

AGP_WaterPuddle::AGP_WaterPuddle()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AGP_WaterPuddle::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGP_WaterPuddle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

