#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CrossoverPracticePlayer.generated.h"

class UInputAction;
class UInputMappingContext;
class UEnhancedInputLocalPlayerSubsystem;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;
class USceneComponent;
struct FInputActionValue;

UCLASS()
class CROSSOVER_API ACrossoverPracticePlayer : public ACharacter
{
    GENERATED_BODY()

public:
    ACrossoverPracticePlayer();
    virtual void PawnClientRestart() override;
    virtual void UnPossessed() override;
    virtual void Tick(float DeltaSeconds) override;

    void SetMatchAppearance(bool bHome, bool bHuman);
    void SetAthleteVariant(int32 Index);
    void SetCarryingBall(bool bCarrying) { bCarryingBall = bCarrying; }
    void PlayShotAnimation(bool bDunk);
    void ResetAthleteMotion();
    void SetArcadeBoost(bool bEnabled);
    FVector GetBallCarryLocation() const;
    FVector GetShotReleaseLocation() const;
    void DriveBotTowards(const FVector& Target, float StopRadius);

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void RemovePlayerMapping();
    void Move(const FInputActionValue& Value);
    void StartSprint();
    void StopSprint();
    void StartShot();
    void ReleaseShot();
    void CancelShot();
    void PassOrSwitch();
    void Contest();
    void Rematch();
    void ActivateBoost();
    void BuildAthlete();
    void UpdateAthlete(float DeltaSeconds);
    void UpdateSpeed();
    void UpdateAthleteMaterials();
    float GetDribbleHeight() const;

    UPROPERTY()
    TObjectPtr<USceneComponent> AthleteRoot;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> Hair;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> Headband;

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> UpperLegs;

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> LowerLegs;

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> Shoes;

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> Soles;

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> UpperArms;

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> Forearms;

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> Hands;

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> JerseyPieces;

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> AccentPieces;

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> SkinPieces;

    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> DarkPieces;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> ControlMarker;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> MatchMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> TeamMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> AccentMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> SkinMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> DarkMaterial;

    int32 AthleteVariant = 0;
    bool bHomeTeam = true;
    bool bHumanPlayer = false;
    bool bSprinting = false;
    bool bArcadeBoost = false;
    bool bCarryingBall = false;
    bool bDunkVisual = false;
    float StridePhase = 0.0f;
    float GaitWeight = 0.0f;
    float DribblePhase = 0.0f;
    float ShotVisualTime = 0.0f;
    float IdlePhase = 0.0f;

    UPROPERTY()
    TObjectPtr<UInputMappingContext> PracticeInput;

    TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> MappedInputSubsystem;

    UPROPERTY()
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY()
    TObjectPtr<UInputAction> SprintAction;

    UPROPERTY()
    TObjectPtr<UInputAction> ShootAction;

    UPROPERTY()
    TObjectPtr<UInputAction> PassAction;

    UPROPERTY()
    TObjectPtr<UInputAction> ContestAction;

    UPROPERTY()
    TObjectPtr<UInputAction> RematchAction;

    UPROPERTY()
    TObjectPtr<UInputAction> BoostAction;
};
