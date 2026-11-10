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
    void DriveBotTowards(const FVector& Target, float StopRadius);

protected:
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

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> ControlMarker;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> MatchMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> TeamMaterial;

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
};
