#include "Prototype/CrossoverMatchMode.h"

#include "Camera/CameraActor.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Prototype/CrossoverMatchHUD.h"
#include "Prototype/CrossoverPracticeCourt.h"
#include "Prototype/CrossoverPracticePlayer.h"
#include "TimerManager.h"

using namespace Crossover::Rules;

namespace
{
constexpr int32 TeamSize = 3;
constexpr int32 PlayerCount = 2 * TeamSize;
constexpr double StealRange = 150.0;
constexpr double DunkRange = 180.0;
constexpr float PassDuration = 0.22f;
constexpr float CheckInDelay = 0.55f;

Team TeamOf(int32 Index)
{
    return Index < TeamSize ? Team::Home : Team::Away;
}
}

ACrossoverMatchMode::ACrossoverMatchMode()
{
    HUDClass = ACrossoverMatchHUD::StaticClass();
}

void ACrossoverMatchMode::StartPlay()
{
    Super::StartPlay();
    APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    if (Controller && GetPracticePlayer() && Court && CourtCamera && Ball)
    {
        Controller->bAutoManageActiveCameraTarget = false;
        Roster.Add(GetPracticePlayer());

        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        for (int32 Index = 1; Index < PlayerCount; ++Index)
        {
            ACrossoverPracticePlayer* Bot = GetWorld()->SpawnActor<ACrossoverPracticePlayer>(
                FVector(-500.0, 150.0 * (Index - TeamSize), 96.0), FRotator::ZeroRotator, Spawn);
            if (!Bot)
            {
                break;
            }
            Roster.Add(Bot);
        }
        if (Roster.Num() == PlayerCount)
        {
            ResetMatch();
        }
    }

    if (FParse::Param(FCommandLine::Get(), TEXT("CrossoverMatchSmoke")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &ACrossoverMatchMode::RunMatchSmoke);
    }
    else if (FParse::Param(FCommandLine::Get(), TEXT("CrossoverMatchPerf")))
    {
        bPerfMode = true;
        PerfWarmupFrames = 120;
        PerfSampleTarget = 720;
        FParse::Value(FCommandLine::Get(), TEXT("PerfWarmup="), PerfWarmupFrames);
        FParse::Value(FCommandLine::Get(), TEXT("PerfSamples="), PerfSampleTarget);
        UE_LOG(LogTemp, Display, TEXT("CrossoverMatchPerf enabled: warmup=%d target_samples=%d"),
            PerfWarmupFrames, PerfSampleTarget);
    }
}

void ACrossoverMatchMode::RunMatchSmoke()
{
    if (Crossover::MatchScenario::Run(*this))
    {
        UE_LOG(LogTemp, Display, TEXT("CROSSOVER_MATCH_SMOKE_READY"));
        FPlatformMisc::RequestExit(false, TEXT("Crossover match smoke completed"));
        return;
    }
    FPlatformMisc::RequestExitWithStatus(false, 1, TEXT("Crossover match smoke failed"));
}

void ACrossoverMatchMode::FinishPerfCapture()
{
    bPerfMode = false;
    if (PerfSamples.Num() == 0)
    {
        FPlatformMisc::RequestExitWithStatus(false, 1, TEXT("No perf samples collected"));
        return;
    }

    TArray<float> Sorted = PerfSamples;
    Sorted.Sort();

    float Sum = 0.0f;
    const float MinMs = Sorted[0];
    const float MaxMs = Sorted[Sorted.Num() - 1];
    for (float Sample : Sorted)
    {
        Sum += Sample;
    }
    const float AvgMs = Sum / Sorted.Num();

    auto Percentile = [&Sorted](float Pct) -> float
    {
        const int32 Index = FMath::Clamp(FMath::RoundToInt32((Pct / 100.0f) * (Sorted.Num() - 1)), 0, Sorted.Num() - 1);
        return Sorted[Index];
    };

    const float MedianMs = Percentile(50.0f);
    const float P95Ms = Percentile(95.0f);
    const float P99Ms = Percentile(99.0f);

    const bool bPassed = (MedianMs <= 16.7f) && (P95Ms <= 22.0f);

    UE_LOG(LogTemp, Display,
        TEXT("CROSSOVER_MATCH_PERF_SUMMARY: samples=%d min=%.2fms max=%.2fms avg=%.2fms median=%.2fms p95=%.2fms p99=%.2fms passed=%s"),
        Sorted.Num(), MinMs, MaxMs, AvgMs, MedianMs, P95Ms, P99Ms, bPassed ? TEXT("true") : TEXT("false"));

    const FString ProfilingDir = FPaths::ProjectSavedDir() / TEXT("Profiling");
    IFileManager::Get().MakeDirectory(*ProfilingDir, true);
    const FString ReportPath = ProfilingDir / TEXT("perf-report.json");

    const FString JsonContent = FString::Printf(
        TEXT("{\n")
        TEXT("  \"samples\": %d,\n")
        TEXT("  \"min_ms\": %.3f,\n")
        TEXT("  \"max_ms\": %.3f,\n")
        TEXT("  \"avg_ms\": %.3f,\n")
        TEXT("  \"median_ms\": %.3f,\n")
        TEXT("  \"p95_ms\": %.3f,\n")
        TEXT("  \"p99_ms\": %.3f,\n")
        TEXT("  \"target_median_ms\": 16.7,\n")
        TEXT("  \"target_p95_ms\": 22.0,\n")
        TEXT("  \"passed\": %s\n")
        TEXT("}\n"),
        Sorted.Num(), MinMs, MaxMs, AvgMs, MedianMs, P95Ms, P99Ms, bPassed ? TEXT("true") : TEXT("false"));

    FFileHelper::SaveStringToFile(JsonContent, *ReportPath);
    UE_LOG(LogTemp, Display, TEXT("CROSSOVER_MATCH_PERF_READY: Report saved to %s"), *ReportPath);

    FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1, TEXT("Crossover perf capture finished"));
}

void ACrossoverMatchMode::ResetMatch()
{
    Rules.Reset();
    BotRandom.Initialize(587);
    FractionalMilliseconds = 0.0;
    StealCooldown = 0.0f;
    TransferControl(0);
    SetUpCheckIn();
    Feedback = TEXT("First to 11. Home ball.");
}

void ACrossoverMatchMode::SetUpCheckIn()
{
    const bool bHomeBall = Rules.GetState().Possession == Team::Home;
    CarrierIndex = bHomeBall ? HumanIndex : TeamSize;
    Shot = InvalidShot;
    PassReceiver = INDEX_NONE;
    bLooseBall = false;
    bBotPassed = false;
    bCharging = false;
    ChargeSeconds = 0.0f;
    BotHoldSeconds = 0.0f;
    ContestRemaining = 0.0f;
    CheckInRemaining = CheckInDelay;

    for (int32 Index = 0; Index < Roster.Num(); ++Index)
    {
        ACrossoverPracticePlayer* Player = Roster[Index];
        Player->GetCharacterMovement()->StopMovementImmediately();
        const bool bOffense = TeamOf(Index) == Rules.GetState().Possession;
        const int32 First = bOffense ? CarrierIndex : (bHomeBall ? TeamSize : HumanIndex);
        const int32 Slot = (Index - First + TeamSize) % TeamSize;
        const double X = bOffense ? (Slot == 0 ? 100.0 : 320.0) : (Slot == 0 ? 305.0 : 490.0);
        const double Y = Slot == 0 ? 0.0 : (Slot == 1 ? -390.0 : 390.0);
        Player->SetActorLocationAndRotation(FVector(X, Y, 96.0), FRotator::ZeroRotator, false,
            nullptr, ETeleportType::TeleportPhysics);
    }
    Ball->SetActorLocation(GetHeldBallPosition(*Roster[CarrierIndex]));
}

void ACrossoverMatchMode::TransferControl(int32 Index)
{
    APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    if (!Controller || Index < 0 || Index >= TeamSize || !Roster.IsValidIndex(Index))
    {
        return;
    }
    if (Controller->GetPawn() != Roster[Index].Get())
    {
        Controller->Possess(Roster[Index].Get());
    }
    HumanIndex = Index;
    for (int32 PlayerIndex = 0; PlayerIndex < Roster.Num(); ++PlayerIndex)
    {
        Roster[PlayerIndex]->SetMatchAppearance(TeamOf(PlayerIndex) == Team::Home, PlayerIndex == HumanIndex);
    }
    Controller->SetViewTarget(CourtCamera.Get());
    bCharging = false;
}

FVector ACrossoverMatchMode::GetBallWorldPosition() const
{
    return Ball ? Ball->GetActorLocation() : FVector::ZeroVector;
}

bool ACrossoverMatchMode::CanPlayBall() const
{
    return Rules.GetState().Ball == BallPhase::Live && Roster.IsValidIndex(CarrierIndex)
        && PassReceiver == INDEX_NONE && !bLooseBall;
}

void ACrossoverMatchMode::StartShot(ACrossoverPracticePlayer& Player)
{
    if (CanPlayBall() && CarrierIndex == HumanIndex && Roster[HumanIndex].Get() == &Player
        && !Rules.GetState().RequiresClear)
    {
        bCharging = true;
        ChargeSeconds = 0.0f;
    }
}

void ACrossoverMatchMode::ReleaseShot(ACrossoverPracticePlayer& Player)
{
    if (bCharging && Roster[HumanIndex].Get() == &Player)
    {
        bCharging = false;
        if (CanPlayBall() && CarrierIndex == HumanIndex)
        {
            TakeShot(HumanIndex, GetChargeFraction());
        }
    }
}

void ACrossoverMatchMode::CancelShot(ACrossoverPracticePlayer& Player)
{
    if (Roster.IsValidIndex(HumanIndex) && Roster[HumanIndex].Get() == &Player)
    {
        bCharging = false;
    }
}

void ACrossoverMatchMode::PassOrSwitch(ACrossoverPracticePlayer& Player)
{
    if (!Roster.IsValidIndex(HumanIndex) || Roster[HumanIndex].Get() != &Player
        || Rules.GetState().Phase == MatchPhase::Finished)
    {
        return;
    }
    if (CanPlayBall() && CarrierIndex == HumanIndex)
    {
        const int32 Next = (HumanIndex + 1) % TeamSize;
        const int32 Other = (HumanIndex + 2) % TeamSize;
        const FVector Direction = Player.GetLastMovementInputVector();
        const FVector Origin = Player.GetActorLocation();
        const double NextAim = FVector::DotProduct(Direction, (Roster[Next]->GetActorLocation() - Origin).GetSafeNormal2D());
        const double OtherAim = FVector::DotProduct(Direction, (Roster[Other]->GetActorLocation() - Origin).GetSafeNormal2D());
        StartPass(OtherAim > NextAim ? Other : Next);
    }
    else if (Rules.GetState().Possession == Team::Away || bLooseBall)
    {
        const FVector Target = GetBallWorldPosition();
        const int32 Next = (HumanIndex + 1) % TeamSize;
        const int32 Other = (HumanIndex + 2) % TeamSize;
        TransferControl(FVector::DistSquared2D(Target, Roster[Other]->GetActorLocation())
            < FVector::DistSquared2D(Target, Roster[Next]->GetActorLocation()) ? Other : Next);
        Feedback = bLooseBall ? TEXT("Player switched") : TEXT("Defender switched");
    }
}

void ACrossoverMatchMode::StartPass(int32 Receiver)
{
    PassReceiver = Receiver;
    FlightStart = GetHeldBallPosition(*Roster[CarrierIndex]);
    FlightSeconds = 0.0f;
    FlightDuration = PassDuration;
    FlightArc = 32.0f;
    bCharging = false;
    Feedback = TEXT("Pass");
}

void ACrossoverMatchMode::Contest(ACrossoverPracticePlayer& Player)
{
    if (!CanPlayBall() || Roster[HumanIndex].Get() != &Player || Rules.GetState().Possession != Team::Away
        || StealCooldown > 0.0f)
    {
        return;
    }
    StealCooldown = 0.75f;
    const double Distance = FVector::Dist2D(Player.GetActorLocation(), Roster[CarrierIndex]->GetActorLocation());
    if (Distance <= StealRange && Rules.Recover(Team::Home))
    {
        CarrierIndex = HumanIndex;
        BotHoldSeconds = 0.0f;
        Feedback = TEXT("Steal! Clear the arc");
    }
    else if (Distance <= 300.0)
    {
        ContestRemaining = 0.85f;
        Feedback = TEXT("Shot contested");
    }
    else
    {
        Feedback = TEXT("Get closer to contest");
    }
}

void ACrossoverMatchMode::Rematch(ACrossoverPracticePlayer& Player)
{
    if (Rules.GetState().Phase == MatchPhase::Finished && Roster[HumanIndex].Get() == &Player)
    {
        ResetMatch();
    }
}

bool ACrossoverMatchMode::IsShotContested(int32 Shooter, const FVector& ShotOrigin, const FVector& TargetRim) const
{
    if (TeamOf(Shooter) == Team::Away && ContestRemaining > 0.0f)
    {
        return true;
    }

    const Team ShooterTeam = TeamOf(Shooter);
    const FVector ToRim = (TargetRim - ShotOrigin).GetSafeNormal2D();
    constexpr double MaxContestDistSq = 220.0 * 220.0;
    constexpr double CloseContestDistSq = 110.0 * 110.0;

    for (int32 Index = 0; Index < Roster.Num(); ++Index)
    {
        if (TeamOf(Index) == ShooterTeam || !Roster.IsValidIndex(Index) || !Roster[Index].Get())
        {
            continue;
        }

        const FVector DefPos = Roster[Index]->GetActorLocation();
        const double DistSq = FVector::DistSquared2D(ShotOrigin, DefPos);
        if (DistSq <= MaxContestDistSq)
        {
            if (DistSq <= CloseContestDistSq)
            {
                return true;
            }
            const FVector ToDef = (DefPos - ShotOrigin).GetSafeNormal2D();
            if (FVector::DotProduct(ToRim, ToDef) > 0.25)
            {
                return true;
            }
        }
    }
    return false;
}

void ACrossoverMatchMode::TakeShot(int32 Shooter, float Charge)
{
    const FVector Position = Roster[Shooter]->GetActorLocation();
    const bool bOutside = Court->IsOutsideArc(Position);
    Shot = Rules.BeginShot(TeamOf(Shooter), bOutside ? ShotRange::OutsideArc : ShotRange::InsideArc);
    if (Shot == InvalidShot)
    {
        return;
    }

    FlightStart = GetHeldBallPosition(*Roster[Shooter]);
    FlightEnd = Court->GetRimLocation();
    const double Distance = FVector::Dist2D(Position, FlightEnd);
    const bool bDunk = Distance <= DunkRange;
    const bool bContested = IsShotContested(Shooter, Position, FlightEnd);
    const float WindowInset = bContested ? 0.07f : 0.0f;
    bShotMade = (bDunk && !bContested) || (Charge >= MakeWindowStart + WindowInset && Charge <= MakeWindowEnd - WindowInset);
    if (!bShotMade)
    {
        FlightEnd.X += Charge < MakeWindowStart ? -150.0 : 110.0;
        FlightEnd.Y += Position.Y >= 0.0 ? 125.0 : -125.0;
        FlightEnd.Z = 28.0;
    }
    FlightSeconds = 0.0f;
    FlightDuration = bDunk ? 0.32f : FMath::Clamp(static_cast<float>(Distance / 1250.0), 0.45f, 1.3f);
    FlightArc = bDunk ? 110.0f : 270.0f;
    Feedback = bShotMade
        ? (bDunk ? TEXT("Dunk!") : (bContested ? TEXT("Tough shot made!") : TEXT("Good release!")))
        : (bContested ? TEXT("Contested miss") : TEXT("Missed release"));
}

void ACrossoverMatchMode::FinishFlight()
{
    if (PassReceiver != INDEX_NONE)
    {
        CarrierIndex = PassReceiver;
        PassReceiver = INDEX_NONE;
        BotHoldSeconds = 0.0f;
        if (TeamOf(CarrierIndex) == Team::Home)
        {
            TransferControl(CarrierIndex);
        }
        return;
    }

    Rules.ResolveShot(Shot, bShotMade);
    Shot = InvalidShot;
    if (!bShotMade && Rules.GetState().Ball == BallPhase::Live)
    {
        LoosePosition = FlightEnd;
        CarrierIndex = INDEX_NONE;
        bLooseBall = true;
    }
}

void ACrossoverMatchMode::ClaimLooseBall()
{
    int32 Closest = INDEX_NONE;
    double Best = FMath::Square(92.0);
    for (int32 Index = 0; Index < Roster.Num(); ++Index)
    {
        const double Distance = FVector::DistSquared2D(LoosePosition, Roster[Index]->GetActorLocation());
        if (Distance < Best)
        {
            Best = Distance;
            Closest = Index;
        }
    }
    if (Closest == INDEX_NONE)
    {
        return;
    }
    const Team Recovering = TeamOf(Closest);
    if (Recovering != Rules.GetState().Possession)
    {
        if (!Rules.Recover(Recovering))
        {
            return;
        }
        bBotPassed = false;
    }
    CarrierIndex = Closest;
    bLooseBall = false;
    BotHoldSeconds = 0.0f;
    Feedback = Rules.GetState().RequiresClear ? TEXT("Rebound. Clear the arc.") : TEXT("Offensive rebound");
    if (Recovering == Team::Home)
    {
        TransferControl(Closest);
    }
}

void ACrossoverMatchMode::Tick(float DeltaSeconds)
{
    AGameModeBase::Tick(DeltaSeconds);
    TickMatch(DeltaSeconds, true);

    if (bPerfMode && Rules.GetState().Phase != MatchPhase::Finished)
    {
        if (PerfWarmupFrames > 0)
        {
            --PerfWarmupFrames;
        }
        else
        {
            PerfSamples.Add(DeltaSeconds * 1000.0f);
            if (PerfSamples.Num() >= PerfSampleTarget)
            {
                FinishPerfCapture();
            }
        }
    }
}

void ACrossoverMatchMode::TickMatch(float DeltaSeconds, bool bDriveBots)
{
    if (Roster.Num() != PlayerCount || Rules.GetState().Phase == MatchPhase::Finished)
    {
        return;
    }

    StealCooldown = FMath::Max(0.0f, StealCooldown - DeltaSeconds);
    ContestRemaining = FMath::Max(0.0f, ContestRemaining - DeltaSeconds);
    if (Rules.GetState().Ball == BallPhase::AwaitingCheckIn)
    {
        if (CheckInRemaining <= 0.0f)
        {
            SetUpCheckIn();
        }
        CheckInRemaining -= DeltaSeconds;
        if (CheckInRemaining <= 0.0f)
        {
            Rules.CheckIn(Rules.GetState().Possession);
        }
        Ball->SetActorLocation(GetHeldBallPosition(*Roster[CarrierIndex]));
        return;
    }

    const double Milliseconds = FractionalMilliseconds + static_cast<double>(DeltaSeconds) * 1000.0;
    const int32 WholeMilliseconds = static_cast<int32>(Milliseconds);
    FractionalMilliseconds = Milliseconds - WholeMilliseconds;
    Rules.AdvanceTime(WholeMilliseconds);
    if (Rules.GetState().Ball == BallPhase::AwaitingCheckIn)
    {
        Feedback = TEXT("Shot clock turnover");
        SetUpCheckIn();
        return;
    }
    if (Rules.GetState().Phase == MatchPhase::Finished)
    {
        if (bPerfMode)
        {
            ResetMatch();
            return;
        }
        bCharging = false;
        return;
    }
    if (bCharging)
    {
        ChargeSeconds = FMath::Min(ChargeSeconds + DeltaSeconds, 1.0f);
    }

    if (Shot != InvalidShot || PassReceiver != INDEX_NONE)
    {
        FlightSeconds += DeltaSeconds;
        const float Alpha = FMath::Min(FlightSeconds / FlightDuration, 1.0f);
        const FVector Target = PassReceiver == INDEX_NONE ? FlightEnd : GetHeldBallPosition(*Roster[PassReceiver]);
        FVector Position = FMath::Lerp(FlightStart, Target, Alpha);
        Position.Z += 4.0f * FlightArc * Alpha * (1.0f - Alpha);
        Ball->SetActorLocation(Position);
        if (Alpha >= 1.0f)
        {
            FinishFlight();
        }
    }
    else if (bLooseBall)
    {
        ClaimLooseBall();
    }
    else if (Roster.IsValidIndex(CarrierIndex))
    {
        const ACrossoverPracticePlayer& Carrier = *Roster[CarrierIndex];
        Ball->SetActorLocation(GetHeldBallPosition(Carrier));
        if (Court->IsOutOfBounds(Carrier.GetActorLocation()) && Rules.OutOfBounds(TeamOf(CarrierIndex)))
        {
            Feedback = TEXT("Out of bounds");
            SetUpCheckIn();
            return;
        }
        if (Court->IsOutsideArc(Carrier.GetActorLocation()) && Rules.ClearArc(TeamOf(CarrierIndex)))
        {
            Feedback = TEXT("Ball cleared");
        }
    }

    if (bDriveBots && Rules.GetState().Ball == BallPhase::Live)
    {
        DriveBots(DeltaSeconds);
    }
}

void ACrossoverMatchMode::DriveBots(float DeltaSeconds)
{
    const Team Possession = Rules.GetState().Possession;
    const FVector Rim = Court->GetRimLocation();
    for (int32 Index = 0; Index < PlayerCount; ++Index)
    {
        if (Index == HumanIndex && !bPerfMode)
        {
            continue;
        }
        ACrossoverPracticePlayer& Bot = *Roster[Index];
        const bool bOffense = TeamOf(Index) == Possession;
        if (bLooseBall)
        {
            Bot.DriveBotTowards(LoosePosition, 45.0f);
        }
        else if (Index == CarrierIndex)
        {
            if (!CanPlayBall())
            {
                continue;
            }
            BotHoldSeconds += DeltaSeconds;
            const double Lane = (Index % TeamSize - 1) * 175.0;
            const FVector Target = Rules.GetState().RequiresClear ? FVector(170.0, Lane, 0.0) : FVector(480.0, Lane, 0.0);
            Bot.DriveBotTowards(Target, 55.0f);
            if (Possession == Team::Away && !Rules.GetState().RequiresClear && !bBotPassed && BotHoldSeconds >= 1.1f
                && Rules.GetState().ShotClockRemainingMilliseconds >= 5500)
            {
                bBotPassed = true;
                StartPass(TeamSize + (Index - TeamSize + 1) % TeamSize);
                return;
            }
            if (Possession == Team::Home && bPerfMode && !Rules.GetState().RequiresClear && !bBotPassed && BotHoldSeconds >= 1.1f
                && Rules.GetState().ShotClockRemainingMilliseconds >= 5500)
            {
                bBotPassed = true;
                StartPass((Index + 1) % TeamSize);
                return;
            }
            if (!Rules.GetState().RequiresClear && BotHoldSeconds >= 2.35f
                && (FVector::DistSquared2D(Target, Bot.GetActorLocation()) < FMath::Square(125.0) || BotHoldSeconds >= 4.0f))
            {
                TakeShot(Index, BotRandom.FRandRange(0.40f, 0.80f));
            }
        }
        else if (bOffense)
        {
            const int32 Slot = Index % TeamSize;
            const double Side = Slot == 0 ? -1.0 : 1.0;
            const double X = Slot == 2 ? 695.0 : 430.0;
            Bot.DriveBotTowards(FVector(X, Side * (Slot == 2 ? 355.0 : 490.0), 0.0), 95.0f);
        }
        else
        {
            const int32 Opponent = (Index + TeamSize) % PlayerCount;
            const FVector OpponentLocation = Roster[Opponent]->GetActorLocation();
            const FVector GuardPoint = OpponentLocation + (Rim - OpponentLocation).GetSafeNormal2D() * 145.0;
            Bot.DriveBotTowards(GuardPoint, 60.0f);
            if (TeamOf(Index) == Team::Away && CanPlayBall() && Opponent == CarrierIndex
                && StealCooldown <= 0.0f && !bCharging
                && FVector::DistSquared2D(OpponentLocation, Bot.GetActorLocation()) < FMath::Square(105.0)
                && Rules.Recover(Team::Away))
            {
                CarrierIndex = Index;
                bBotPassed = false;
                BotHoldSeconds = 0.0f;
                StealCooldown = 1.5f;
                Feedback = TEXT("Away steal");
                break;
            }
            else if (TeamOf(Index) == Team::Home && bPerfMode && CanPlayBall() && Opponent == CarrierIndex
                && StealCooldown <= 0.0f
                && FVector::DistSquared2D(OpponentLocation, Bot.GetActorLocation()) < FMath::Square(105.0)
                && Rules.Recover(Team::Home))
            {
                CarrierIndex = Index;
                bBotPassed = false;
                BotHoldSeconds = 0.0f;
                StealCooldown = 1.5f;
                Feedback = TEXT("Home steal! Clear the arc");
                TransferControl(Index);
                break;
            }
        }
    }
}
