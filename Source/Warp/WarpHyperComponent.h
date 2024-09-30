// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/UObjectGlobals.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "Camera/CameraComponent.h"
#include "Warp.h"
#include "WarpCharacter.h"
#include <algorithm>
#include "WarpHyperComponent.generated.h"

using namespace WarpMath;

UCLASS()
class WARP_API AWarpHyperComponent : public AActor
{
	GENERATED_BODY()

    TArray<UMaterialInstanceDynamic*> dynMaterials;
    TMap<int32, GyroVectorD> localGVByPos;

	TArray<UStaticMeshComponent*> mcomp;

    FWarpGameModule* mainModule;

    FName tag = TEXT("Hyperbolic");

    FName hyp0 = TEXT("hyperRot0");
    FName hyp1 = TEXT("hyperRot1");
    FName hyp2 = TEXT("hyperRot2");
    FName hyp3 = TEXT("hyperRot3");

    GyroVectorD worldGV = GyroVectorD(FVector4(0,0,0,0));
    GyroVectorD localGV = GyroVectorD(FVector4(0, 0, 0, 0));
    GyroVectorD composedGV = GyroVectorD(FVector4(0, 0, 0, 0));

    AWarpCharacter* actor;

	float SENSITIVITY_LOOK = 1.0f;

	float rotationX = 0.0f;
	float rotationZ = 0.0f;
	float lockedRotationX = 0.0f;
	float lockedRotationZ = 0.0f;

	float smoothRotationX = 0.0f;
	float smoothRotationZ = 0.0f;
	
	float LAG_MOVE = 0.05f;

	float walkingSpeed = 2.0f;
	float height = 0.1f;
	float GRAVITY = -4.0f;

	FQuat xzQuaternion = FQuat(0, 0, 0, 0);
	float LOCKED_MAX_X = 10.0f;
	float LOCKED_MAX_Z = 10.0f;

	FVector velocity = FVector(0, 0, 0);
	FVector inputDelta = FVector(0, 0, 0);

	float targetRotationX = 0.0f;
	float targetRotationZ = 0.0f;
	FQuat targetQuaternion = FQuat(0, 0, 0, 0);

	float camHeight = 0.0f;

	bool isLocked = false;

public:	
	// Sets default values for this component's properties
	AWarpHyperComponent(const FObjectInitializer& ObjectInitializer);
	bool IsLocked() { return isLocked; };
	void Lock();
	void Unlock();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

		
};
