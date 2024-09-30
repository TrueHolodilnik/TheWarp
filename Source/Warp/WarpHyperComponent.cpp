// Fill out your copyright notice in the Description page of Project Settings.


#include "WarpHyperComponent.h"

// Sets default values for this component's properties
AWarpHyperComponent::AWarpHyperComponent(const FObjectInitializer& ObjectInitializer)
{

	PrimaryActorTick.bCanEverTick = true;
	
}

//Lock and unlock rotations
void AWarpHyperComponent::Lock() 
{
	isLocked = true;
	LOCKED_MAX_X = 10.0f;
	LOCKED_MAX_Z = 10.0f;
	lockedRotationX = 0.0f;
	lockedRotationZ = 0.0f;
	targetRotationX = rotationX;
	targetRotationZ = rotationZ;
	targetQuaternion = xzQuaternion;
}

void AWarpHyperComponent::Unlock()
{
	isLocked = false;
}

// Called when the game starts
void AWarpHyperComponent::BeginPlay()
{
	Super::BeginPlay();

	mainModule = &FModuleManager::GetModuleChecked<FWarpGameModule>("Warp");

	TArray<AActor*> objects;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), tag, objects);

	TArray<FVector> objPositions;

	for (AActor* obj : objects)
	{
		TArray<UStaticMeshComponent*> Components;
		obj->GetComponents<UStaticMeshComponent>(Components);
		
		for (int32 i = 0; i < Components.Num(); i++)
		{
			UStaticMeshComponent* StaticMeshComponent = Components[i];
			UMaterialInterface* StaticMaterial = StaticMeshComponent->GetMaterial(i);
			UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(StaticMaterial, nullptr);
			StaticMeshComponent->SetMaterial(i, DynMaterial);
			objPositions.Add(StaticMeshComponent->GetComponentLocation());
			dynMaterials.Add(DynMaterial);
			mcomp.Add(StaticMeshComponent);
		
		}
	}

	TArray<WorldTile>* tiles = mainModule->GetTilemap();

	float CW = mainModule->GetCellW();

	//Apply position shift
	for (int i = 0; i < objPositions.Num(); i++)
	{
		FVector pos = objPositions[i] / 1000;
		bool is_found = false;
		for (WorldTile tile : *tiles)
		{

			if ((tile.xz.X <= pos.X && pos.X < tile.xz.X + CW) && (tile.xz.Y <= pos.Z && pos.Z < tile.xz.Y + CW)) {
				localGVByPos.Add(i, tile.gv);
				is_found = true;
				break;
			}
		}
		if (!is_found) {
			localGVByPos.Add(i, GyroVectorD());
		}
	}

	height *= mainModule->GetKlein() / 0.5774f;
	
}

float ModPi(float a, float b) {
	if (a - b > 180.0f) {
		a -= 360.0f;
	}
	else if (a - b < -180.0f) {
		a += 360.0f;
	}
	return a;
}

// Called every frame
// Update parameters for materials
void AWarpHyperComponent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FString Output;
	FVector4 pos;

	if (actor) {

		//Update raw rotations
		if (IsLocked()) {
			//Update raw rotations
			FVector2D lookXY = FVector2D(actor->GetLastPitch(), actor->GetLastYaw());

			rotationZ += lookXY.Y * SENSITIVITY_LOOK;
			rotationX += lookXY.X * SENSITIVITY_LOOK;

			//Clamp raw rotations to valid ranges
			float lockedMinZ = std::max(-LOCKED_MAX_Z, -90.0f - rotationZ);
			float lockedMaxZ = std::min(LOCKED_MAX_Z, 90.0f - rotationZ);
			lockedRotationZ = clamp(lockedMinZ, lockedMaxZ, lockedRotationZ);
			lockedRotationX = clamp(-LOCKED_MAX_X, LOCKED_MAX_X, lockedRotationX);

			//Apply locked look smoothing (time dependent)
			rotationX = ModPi(rotationX, targetRotationX);
			float smooth_look_locked = 0.0f;

			rotationX = rotationX * smooth_look_locked + targetRotationX * (1 - smooth_look_locked);
			rotationZ = rotationZ * smooth_look_locked + targetRotationZ * (1 - smooth_look_locked);
		}
		else {

			//Update if there was any locked rotation
			rotationX += lockedRotationX;
			rotationZ += lockedRotationZ;
			lockedRotationX = 0.0f;
			lockedRotationZ = 0.0f;

			//Update raw rotations
			FVector2D lookXY = FVector2D(actor->GetLastPitch(), actor->GetLastYaw());
			rotationZ += lookXY.Y * SENSITIVITY_LOOK;
			rotationX += lookXY.X * SENSITIVITY_LOOK;

		}


		//Clamp raw rotations to valid ranges
		rotationZ = clamp(-90.0f, 90.0f, rotationZ);
		while (rotationX > 180.0f) { rotationX -= 360.0f; }
		while (rotationX < -180.0f) { rotationX += 360.0f; }

		//Apply smoothing (time dependent)
		smoothRotationX = ModPi(smoothRotationX, rotationX + lockedRotationX);
		float clampedRotationZ = clamp(-90.0f, 90.0f, rotationZ + lockedRotationZ);
		float smooth_look = pow(2.0f, -DeltaTime / 0.025f);
		smoothRotationX = smoothRotationX * smooth_look + (rotationX + lockedRotationX) * (1 - smooth_look);
		smoothRotationZ = smoothRotationZ * smooth_look + clampedRotationZ * (1 - smooth_look);

		//Get the rotation you will be at next as a Quaternion
		FQuat zQuaternion;
		FQuat xQuaternion;
		if (IsLocked()) {
			zQuaternion = Quaternion.AngleAxis(smoothRotationZ, Vector3.left);
			xQuaternion = Quaternion.AngleAxis(smoothRotationX, Vector3.up);
			xzQuaternion = Quaternion.Slerp(targetQuaternion, xzQuaternion, smooth_look);
			focusRot = xzQuaternion * xQuaternion * zQuaternion;
		}
		else {
			zQuaternion = Quaternion.AngleAxis(smoothRotationZ * Time.deltaTime * 100.0f, Vector3.left);
			xQuaternion = Quaternion.AngleAxis(smoothRotationX * Time.deltaTime * 100.0f, Vector3.up);
			xzQuaternion *= xQuaternion * zQuaternion;
			rotationX = 0.0f;
			rotationZ = 0.0f;
			focusRot = xzQuaternion;
		}
		
		xQuaternion = xzQuaternion;
		xzQuaternion = xQuaternion * zQuaternion;

		actor->AddControllerPitchInput(actor->GetLastPitch());
		actor->AddControllerYawInput(actor->GetLastYaw());

		//Always work to dampen movement, even when locked.
		float smooth_move = pow(2.0f, -DeltaTime / LAG_MOVE);
		velocity.X *= smooth_move;
		velocity.Z *= smooth_move;

		FVector displacement = FVector(0, 0, 0);
		if (!IsLocked()) {

			FVector2D move = FVector2D(actor->GetLastRight(), actor->GetLastForward());

			inputDelta = ClampMagnitude(FVector(move.X, move.Y, 0.0f), 1.0f);

			inputDelta = xQuaternion * inputDelta;

			inputDelta *= walkingSpeed * height;

			velocity += inputDelta * (1 - smooth_move);
			
			//Apply gravity
			velocity.Z += GRAVITY * height * DeltaTime;
			inputDelta = HyperTranslate(velocity * DeltaTime);

			displacement = inputDelta * 0.9f;

			velocity = FVector(0, 0, 0);

			velocity.Z = std::max(velocity.Z, 0.0f);

			//Map that world displacement to a hyperbolic one (in high precision since this only happens once per frame)
			FVector outputDelta = displacement;
			GyroVectorD gv = worldGV;
			gv = sub(gv, outputDelta);
			gv.vec.Z = std::min(gv.vec.Z, 0.0f);
			gv.AlignUpVector();
			worldGV = gv;

			float headDelta = 0.0f;

			camHeight = TanK(height + headDelta);

		}
	}
	else {
		TArray<AActor*> objects;
		TSubclassOf<AWarpCharacter> classToFind;
		classToFind = AWarpCharacter::StaticClass();
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), classToFind, objects);
	}
	
	//Update world gyrovector
	FVector4 displacement = FVector4(0, 0, 0, 0);
	FVector4 outputDelta = displacement;
	worldGV = sub(outputDelta, worldGV);
	worldGV.vec.Y = std::min(worldGV.vec.Y, 0.0f);
	worldGV.AlignUpVector();

	//Set parameters for each non-euqlidean material
	for (int32 i = 0; i < dynMaterials.Num(); i++)
	{
		localGV = localGVByPos[i];
		composedGV = add(localGV, worldGV);
		FMatrix mat = composedGV.ToMatrix();

		UMaterialInstanceDynamic* materialInstanceDynamic = dynMaterials[i];

		materialInstanceDynamic->SetVectorParameterValue(hyp0, FLinearColor(mat.M[0][0], mat.M[0][1], mat.M[0][2], mat.M[0][3]));
		materialInstanceDynamic->SetVectorParameterValue(hyp1, FLinearColor(mat.M[1][0], mat.M[1][1], mat.M[1][2], mat.M[1][3]));
		materialInstanceDynamic->SetVectorParameterValue(hyp2, FLinearColor(mat.M[2][0], mat.M[2][1], mat.M[2][2], mat.M[2][3]));
		materialInstanceDynamic->SetVectorParameterValue(hyp3, FLinearColor(mat.M[3][0], mat.M[3][1], mat.M[3][2], mat.M[3][3]));
		materialInstanceDynamic->SetScalarParameterValue("N", (float) mainModule->GetN());
		materialInstanceDynamic->SetScalarParameterValue("camHeight", camHeight);
	}
	
}

