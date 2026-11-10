#pragma once

#include "Prototype/CrossoverPracticeMode.h"
#include "Rules/MatchRules.h"
#include "CrossoverMatchMode.generated.h"

class ACrossoverMatchMode;

namespace Crossover::MatchScenario
{
bool Run(ACrossoverMatchMode& Mode);
}

UCLASS()
class CROSSOVER_API ACrossoverMatchMode : public ACrossoverPracticeMode
{
    GENERATED_BODY()

public:
    ACrossoverMatchMode();
    virtual void StartPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    void StartShot(ACrossoverPracticePlayer& Player);
    void ReleaseShot(ACrossoverPracticePlayer& Player);
    void CancelShot(ACrossoverPracticePlayer& Player);
    void PassOrSwitch(ACrossoverPracticePlayer& Player);
    void Contest(ACrossoverPracticePlayer& Player);
    void Rematch(ACrossoverPracticePlayer& Player);

    const Crossover::Rules::MatchState& GetMatchState() const { return Rules.GetState(); }
    bool IsChargingShot() const { return bCharging; }
    float GetChargeFraction() const { return FMath::Min(ChargeSeconds, 1.0f); }
    const FString& GetFeedback() const { return Feedback; }
    FVector GetBallWorldPosition() const;
    bool IsShotContested(int32 Shooter, const FVector& ShotOrigin, const FVector& TargetRim) const;

private:
    friend bool Crossover::MatchScenario::Run(ACrossoverMatchMode& Mode);

    void RunMatchSmoke();
    void FinishPerfCapture();
    void ResetMatch();
    void SetUpCheckIn();
    void TickMatch(float DeltaSeconds, bool bDriveBots);
    void TransferControl(int32 Index);
    void StartPass(int32 Receiver);
    void TakeShot(int32 Shooter, float Charge);
    void FinishFlight();
    void ClaimLooseBall();
    void DriveBots(float DeltaSeconds);
    bool CanPlayBall() const;

    UPROPERTY()
    TArray<TObjectPtr<ACrossoverPracticePlayer>> Roster;

    Crossover::Rules::MatchRules Rules;
    Crossover::Rules::ShotId Shot = Crossover::Rules::InvalidShot;
    FRandomStream BotRandom{587};
    int32 HumanIndex = 0;
    int32 CarrierIndex = INDEX_NONE;
    int32 PassReceiver = INDEX_NONE;
    bool bCharging = false;
    bool bLooseBall = false;
    bool bShotMade = false;
    bool bBotPassed = false;
    float ChargeSeconds = 0.0f;
    float CheckInRemaining = 0.0f;
    float FlightSeconds = 0.0f;
    float FlightDuration = 0.0f;
    float FlightArc = 0.0f;
    float BotHoldSeconds = 0.0f;
    float StealCooldown = 0.0f;
    float ContestRemaining = 0.0f;
    double FractionalMilliseconds = 0.0;
    FVector FlightStart = FVector::ZeroVector;
    FVector FlightEnd = FVector::ZeroVector;
    FVector LoosePosition = FVector::ZeroVector;
    FString Feedback;
    bool bPerfMode = false;
    int32 PerfWarmupFrames = 120;
    int32 PerfSampleTarget = 720;
    TArray<float> PerfSamples;
};
