#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "CrossoverPracticeMode.generated.h"

class ACameraActor;
class ACrossoverPracticeCourt;
class ACrossoverPracticePlayer;
class AStaticMeshActor;
class UStaticMesh;

UCLASS()
class CROSSOVER_API ACrossoverPracticeHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};

UCLASS()
class CROSSOVER_API ACrossoverPracticeMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ACrossoverPracticeMode();
    virtual void StartPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

    void StartShot(const ACrossoverPracticePlayer& Player);
    void ReleaseShot(const ACrossoverPracticePlayer& Player);
    void CancelShot(const ACrossoverPracticePlayer& Player);

    int32 GetPracticeScore() const { return PracticeScore; }
    bool IsChargingShot() const { return bCharging; }
    float GetChargeFraction() const;
    FString GetShotFeedback() const { return ShotFeedback; }

    static constexpr float MakeWindowStart = 0.52f;
    static constexpr float MakeWindowEnd = 0.72f;

private:
    void VerifySmokeStart();
    ACrossoverPracticePlayer* GetPracticePlayer() const;
    FVector GetHeldBallPosition(const ACrossoverPracticePlayer& Player) const;

    UPROPERTY()
    TObjectPtr<ACrossoverPracticeCourt> Court;

    UPROPERTY()
    TObjectPtr<ACameraActor> CourtCamera;

    UPROPERTY()
    TObjectPtr<AStaticMeshActor> Ball;

    UPROPERTY()
    TObjectPtr<UStaticMesh> BallMesh;

    int32 PracticeScore = 0;
    int32 PendingPoints = 0;
    bool bCharging = false;
    bool bBallInFlight = false;
    float ChargeSeconds = 0.0f;
    float FlightSeconds = 0.0f;
    float FlightDuration = 1.0f;
    float FlightArc = 0.0f;
    FVector FlightStart = FVector::ZeroVector;
    FVector FlightEnd = FVector::ZeroVector;
    FString ShotFeedback;
};
