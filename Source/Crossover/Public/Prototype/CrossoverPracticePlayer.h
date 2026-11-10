#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CrossoverPracticePlayer.generated.h"

class UInputAction;
class UInputMappingContext;
class UStaticMeshComponent;
struct FInputActionValue;

UCLASS()
class CROSSOVER_API ACrossoverPracticePlayer : public ACharacter
{
    GENERATED_BODY()

public:
    ACrossoverPracticePlayer();
    virtual void PawnClientRestart() override;
    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
    void Move(const FInputActionValue& Value);
    void StartSprint();
    void StopSprint();
    void StartShot();
    void ReleaseShot();
    void CancelShot();

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY()
    TObjectPtr<UInputMappingContext> PracticeInput;

    UPROPERTY()
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY()
    TObjectPtr<UInputAction> SprintAction;

    UPROPERTY()
    TObjectPtr<UInputAction> ShootAction;
};
