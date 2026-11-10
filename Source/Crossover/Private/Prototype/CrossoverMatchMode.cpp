#include "Prototype/CrossoverMatchMode.h"

#include "Camera/CameraActor.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Prototype/CrossoverArcadeAudio.h"
#include "Prototype/CrossoverMatchHUD.h"
#include "Prototype/CrossoverPracticeCourt.h"
#include "Prototype/CrossoverPracticePlayer.h"
#include "Scalability.h"
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
constexpr float BoostDuration = 7.0f;
constexpr float BotShotDelay[] = {3.1f, 2.35f, 1.85f};
constexpr float BotShotSpread[] = {0.27f, 0.20f, 0.13f};
constexpr int32 PerfWidth = 1920;
constexpr int32 PerfHeight = 1080;

Scalability::FQualityLevels FullRegulationQuality()
{
    Scalability::FQualityLevels Quality(false);
    Quality.SetFromSingleQualityLevel(0);
    // UE's default Low preset scales resolution down. Keep the 3D scene at 1080p.
    Quality.ResolutionQuality = 100.0f;
    return Quality;
}

void SetPerfCVar(const TCHAR* Name, int32 Value)
{
    if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
    {
        Variable->Set(Value, ECVF_SetByConsole);
    }
}

bool PerfCVarEquals(const TCHAR* Name, int32 Value)
{
    const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
    return Variable && Variable->GetFloat() == static_cast<float>(Value);
}

Team TeamOf(int32 Index)
{
    return Index < TeamSize ? Team::Home : Team::Away;
}
}

ACrossoverMatchMode::ACrossoverMatchMode()
{
    HUDClass = ACrossoverMatchHUD::StaticClass();
    ArcadeAudio = CreateDefaultSubobject<UCrossoverArcadeAudio>(TEXT("ArcadeAudio"));
}

void ACrossoverMatchMode::StartPlay()
{
    Super::StartPlay();
    const bool bSmoke = FParse::Param(FCommandLine::Get(), TEXT("CrossoverMatchSmoke"));
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
            for (int32 Index = 0; Index < Roster.Num(); ++Index)
            {
                Roster[Index]->SetAthleteVariant(Index);
            }
            InitializeArcadePresentation();
            ResetMatch();
            bAtTitle = !bSmoke && !FParse::Param(FCommandLine::Get(), TEXT("CrossoverQuickPlay"))
                && !FParse::Param(FCommandLine::Get(), TEXT("CrossoverMatchPerf"));
            SetMenuMovement(bAtTitle);
        }
    }

    if (bSmoke)
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
        PerfSampleTarget = FMath::Max(1, PerfSampleTarget);
        bPerfFullRegulation = FParse::Param(FCommandLine::Get(), TEXT("PerfFullRegulation"));
        if (bPerfFullRegulation)
        {
            ConfigureFullRegulationPerf();
            return;
        }
        UE_LOG(LogTemp, Display, TEXT("CrossoverMatchPerf enabled: warmup=%d target_samples=%d"),
            PerfWarmupFrames, PerfSampleTarget);
    }
}

void ACrossoverMatchMode::SetMenuMovement(bool bBlocked)
{
    if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
    {
        Controller->SetIgnoreMoveInput(bBlocked);
    }
    if (bBlocked)
    {
        bCharging = false;
        for (ACrossoverPracticePlayer* Player : Roster)
        {
            Player->GetCharacterMovement()->StopMovementImmediately();
        }
    }
}

const TCHAR* ACrossoverMatchMode::GetDifficultyName() const
{
    constexpr const TCHAR* Names[] = {TEXT("ROOKIE"), TEXT("STREET"), TEXT("LEGEND")};
    return Names[Difficulty];
}

float ACrossoverMatchMode::GetBoostFill() const
{
    return IsBoostActive() ? BoostSeconds / BoostDuration : BoostMeter / 100.0f;
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

void ACrossoverMatchMode::ConfigureFullRegulationPerf()
{
    if (!ALLOW_DEBUG_FILES || Roster.Num() != PlayerCount
        || FParse::Param(FCommandLine::Get(), TEXT("NullRHI")))
    {
        bPerfMode = false;
        UE_LOG(LogTemp, Error, TEXT("Full regulation benchmark needs a rendered Development build and six athletes"));
        FPlatformMisc::RequestExitWithStatus(false, 1, TEXT("Full regulation benchmark unavailable"));
        return;
    }

    UGameUserSettings::RequestResolutionChange(PerfWidth, PerfHeight, EWindowMode::Windowed, false);
    Scalability::SetQualityLevels(FullRegulationQuality());
    SetPerfCVar(TEXT("r.ScreenPercentage"), 100);
    SetPerfCVar(TEXT("r.SecondaryScreenPercentage.GameViewport"), 100);
    SetPerfCVar(TEXT("r.DynamicRes.OperationMode"), 0);
    SetPerfCVar(TEXT("r.VSync"), 0);
    SetPerfCVar(TEXT("t.MaxFPS"), 0);
    SetPerfCVar(TEXT("t.FPSChart.ExcludeIdleTime"), 0);
    SetPerfCVar(TEXT("t.FPSChart.OpenFolderOnDump"), 0);
    PerfWarmupFrames = FMath::Max(PerfWarmupFrames, 120);
    UE_LOG(LogTemp, Display, TEXT("Full regulation benchmark warming up for %d frames"), PerfWarmupFrames);
}

bool ACrossoverMatchMode::VerifyFullRegulationDisplay() const
{
    const UGameViewportClient* Viewport = GEngine ? GEngine->GameViewport.Get() : nullptr;
    FVector2D Size = FVector2D::ZeroVector;
    if (Viewport)
    {
        Viewport->GetViewportSize(Size);
    }
    const bool bLow = Scalability::GetQualityLevels() == FullRegulationQuality()
        && PerfCVarEquals(TEXT("r.ScreenPercentage"), 100)
        && PerfCVarEquals(TEXT("r.SecondaryScreenPercentage.GameViewport"), 100)
        && PerfCVarEquals(TEXT("r.DynamicRes.OperationMode"), 0);
    const bool bWindowed = Viewport && !Viewport->IsFullScreenViewport();
    UE_LOG(LogTemp, Display, TEXT("CROSSOVER_MATCH_PERF_DISPLAY: viewport=%dx%d low_native=%d windowed=%d"),
        static_cast<int32>(Size.X), static_cast<int32>(Size.Y), bLow, bWindowed);
    return Size == FVector2D(PerfWidth, PerfHeight) && bLow && bWindowed;
}

void ACrossoverMatchMode::TickPerf(float DeltaSeconds)
{
    if (PerfWarmupFrames > 0)
    {
        --PerfWarmupFrames;
    }
    else if (!bPerfFullRegulation)
    {
        PerfSamples.Add(DeltaSeconds * 1000.0f);
    }
    else if (PerfStartedAt == 0.0)
    {
        if (!VerifyFullRegulationDisplay())
        {
            bPerfMode = false;
            FPlatformMisc::RequestExitWithStatus(false, 1, TEXT("Full regulation benchmark display mismatch"));
            return;
        }
        // Reach the regulation limit even if a team scores more than 11 points.
        MatchConfig Config;
        Config.TargetScore = MAX_int32;
        Rules = MatchRules(Config);
        ResetMatch();
        Feedback = TEXT("Full regulation benchmark");
        GEngine->StartFPSChart(TEXT("CrossoverFullRegulation"), true);
        PerfStartedAt = PerfLastFrameAt = FPlatformTime::Seconds();
        return;
    }
    else
    {
        const double Now = FPlatformTime::Seconds();
        PerfSamples.Add(static_cast<float>((Now - PerfLastFrameAt) * 1000.0));
        PerfLastFrameAt = Now;
    }

    if (bPerfFullRegulation)
    {
        if (PerfStartedAt > 0.0 && Rules.GetState().RegulationRemainingMilliseconds == 0)
        {
            GEngine->StopFPSChart(TEXT("CrossoverNightShift"));
            FinishPerfCapture();
        }
    }
    else if (PerfSamples.Num() >= PerfSampleTarget || Rules.GetState().Phase == MatchPhase::Finished)
    {
        FinishPerfCapture();
    }
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

    const bool bDisplayPassed = !bPerfFullRegulation || VerifyFullRegulationDisplay();
    const bool bPassed = bDisplayPassed && (MedianMs <= 16.7f) && (P95Ms <= 22.0f);

    UE_LOG(LogTemp, Display,
        TEXT("CROSSOVER_MATCH_PERF_SUMMARY: samples=%d min=%.2fms max=%.2fms avg=%.2fms median=%.2fms p95=%.2fms p99=%.2fms passed=%s"),
        Sorted.Num(), MinMs, MaxMs, AvgMs, MedianMs, P95Ms, P99Ms, bPassed ? TEXT("true") : TEXT("false"));

    const FString ProfilingDir = FPaths::ProjectSavedDir() / TEXT("Profiling");
    IFileManager::Get().MakeDirectory(*ProfilingDir, true);
    const FString ReportPath = ProfilingDir /
        (bPerfFullRegulation ? TEXT("perf-full-regulation.json") : TEXT("perf-report.json"));
    FString RegulationReport;
    if (bPerfFullRegulation)
    {
        const int32 RegulationMs = MatchConfig{}.RegulationMilliseconds - Rules.GetState().RegulationRemainingMilliseconds;
        const double WallSeconds = PerfLastFrameAt - PerfStartedAt;
        RegulationReport = FString::Printf(
            TEXT("  \"regulation_ms\": %d,\n")
            TEXT("  \"wall_seconds\": %.3f,\n")
            TEXT("  \"full_regulation_display_verified\": %s,\n"),
            RegulationMs, WallSeconds, bDisplayPassed ? TEXT("true") : TEXT("false"));
        UE_LOG(LogTemp, Display,
            TEXT("CROSSOVER_MATCH_PERF_REGULATION: regulation_ms=%d wall_seconds=%.3f samples=%d"),
            RegulationMs, WallSeconds, PerfSamples.Num());
    }

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
        TEXT("%s  \"passed\": %s\n")
        TEXT("}\n"),
        Sorted.Num(), MinMs, MaxMs, AvgMs, MedianMs, P95Ms, P99Ms, *RegulationReport,
        bPassed ? TEXT("true") : TEXT("false"));

    if (!FFileHelper::SaveStringToFile(JsonContent, *ReportPath))
    {
        FPlatformMisc::RequestExitWithStatus(false, 1, TEXT("Could not save performance report"));
        return;
    }
    UE_LOG(LogTemp, Display, TEXT("CROSSOVER_MATCH_PERF_READY: Report saved to %s"), *ReportPath);

    FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1, TEXT("Crossover perf capture finished"));
}

void ACrossoverMatchMode::ResetMatch()
{
    Rules.Reset();
    BotRandom.Initialize(587);
    FractionalMilliseconds = 0.0;
    StealCooldown = 0.0f;
    BoostMeter = 0;
    BoostSeconds = 0.0f;
    ScoreFlash = 0.0f;
    BounceDelay = 0.0f;
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
        Player->ResetAthleteMotion();
        Player->SetArcadeBoost(Index == HumanIndex && IsBoostActive());
        Player->SetCarryingBall(Index == CarrierIndex);
        const bool bOffense = TeamOf(Index) == Rules.GetState().Possession;
        const int32 First = bOffense ? CarrierIndex : (bHomeBall ? TeamSize : HumanIndex);
        const int32 Slot = (Index - First + TeamSize) % TeamSize;
        const double X = bOffense ? (Slot == 0 ? 100.0 : 320.0) : (Slot == 0 ? 305.0 : 490.0);
        const double Y = Slot == 0 ? 0.0 : (Slot == 1 ? -390.0 : 390.0);
        Player->SetActorLocationAndRotation(FVector(X, Y, 96.0), FRotator::ZeroRotator, false,
            nullptr, ETeleportType::TeleportPhysics);
    }
    PlaceHeldBall();
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
        Roster[PlayerIndex]->SetArcadeBoost(PlayerIndex == HumanIndex && IsBoostActive());
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
    return !bAtTitle && !bMatchPaused && Rules.GetState().Ball == BallPhase::Live && Roster.IsValidIndex(CarrierIndex)
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
    if (bAtTitle && Roster.IsValidIndex(HumanIndex) && Roster[HumanIndex].Get() == &Player)
    {
        bAtTitle = false;
        SetMenuMovement(false);
        ArcadeAudio->Play(ECrossoverArcadeCue::Clear);
        return;
    }
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
    if (bAtTitle)
    {
        Difficulty = (Difficulty + 1) % 3;
        ArcadeAudio->Play(ECrossoverArcadeCue::Pass);
        return;
    }
    if (bMatchPaused)
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
    FlightStart = Roster[CarrierIndex]->GetBallCarryLocation();
    FlightSeconds = 0.0f;
    FlightDuration = PassDuration;
    FlightArc = 32.0f;
    bCharging = false;
    ContestRemaining = 0.0f;
    Feedback = TEXT("Pass");
    ArcadeAudio->Play(ECrossoverArcadeCue::Pass);
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
        BoostMeter = FMath::Min(100, BoostMeter + 35);
        Feedback = TEXT("Steal! Clear the arc");
        ArcadeAudio->Play(ECrossoverArcadeCue::Steal);
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
    if (bAtTitle || !Roster.IsValidIndex(HumanIndex) || Roster[HumanIndex].Get() != &Player)
    {
        return;
    }
    if (Rules.GetState().Phase == MatchPhase::Finished)
    {
        ResetMatch();
        return;
    }
    bMatchPaused = !bMatchPaused;
    SetMenuMovement(bMatchPaused);
}

void ACrossoverMatchMode::ActivateBoost(ACrossoverPracticePlayer& Player)
{
    if (!CanPlayBall() || Roster[HumanIndex].Get() != &Player || BoostMeter < 100 || IsBoostActive())
    {
        return;
    }
    BoostMeter = 0;
    BoostSeconds = BoostDuration;
    Player.SetArcadeBoost(true);
    Feedback = TEXT("OVERTAKE!  Speed and shot window up");
    ArcadeAudio->Play(ECrossoverArcadeCue::Clear);
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

float ACrossoverMatchMode::GetShotWindowInset(int32 Shooter, bool bContested) const
{
    if (bContested)
    {
        return 0.07f;
    }
    return Shooter == HumanIndex && IsBoostActive() ? -0.09f : 0.0f;
}

float ACrossoverMatchMode::GetChargeWindowInset() const
{
    if (!Roster.IsValidIndex(HumanIndex) || !Court)
    {
        return 0.0f;
    }
    const FVector Position = Roster[HumanIndex]->GetActorLocation();
    return GetShotWindowInset(HumanIndex, IsShotContested(HumanIndex, Position, Court->GetRimLocation()));
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

    FlightEnd = Court->GetRimLocation();
    const double Distance = FVector::Dist2D(Position, FlightEnd);
    const bool bDunk = Distance <= DunkRange;
    const bool bContested = IsShotContested(Shooter, Position, FlightEnd);
    bCurrentShotDunk = bDunk;
    Roster[Shooter]->PlayShotAnimation(bDunk);
    FlightStart = Roster[Shooter]->GetShotReleaseLocation();
    ArcadeAudio->Play(ECrossoverArcadeCue::Release);
    const float WindowInset = GetShotWindowInset(Shooter, bContested);
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

    const Team ShootingTeam = Rules.GetState().Possession;
    const auto ScoreBefore = ShootingTeam == Team::Home ? Rules.GetState().HomeScore : Rules.GetState().AwayScore;
    Rules.ResolveShot(Shot, bShotMade);
    if (bShotMade)
    {
        bHomeScoreFlash = ShootingTeam == Team::Home;
        const auto ScoreAfter = bHomeScoreFlash ? Rules.GetState().HomeScore : Rules.GetState().AwayScore;
        LastScorePoints = static_cast<int32>(ScoreAfter - ScoreBefore);
        bLastScoreDunk = bCurrentShotDunk;
        ScoreFlash = 1.2f;
        if (bHomeScoreFlash)
        {
            BoostMeter = FMath::Min(100, BoostMeter + (bCurrentShotDunk ? 50 : 40));
        }
        ArcadeAudio->Play(bCurrentShotDunk ? ECrossoverArcadeCue::Dunk : ECrossoverArcadeCue::Basket);
        if (Rules.GetState().Phase == MatchPhase::Finished)
        {
            ArcadeAudio->Play(ECrossoverArcadeCue::Win);
        }
    }
    else
    {
        ArcadeAudio->Play(ECrossoverArcadeCue::Rim);
    }
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
    if (!bAtTitle && !bMatchPaused)
    {
        TickMatch(DeltaSeconds, true);
        UpdateArcadePresentation(DeltaSeconds);
    }

    if (bPerfMode)
    {
        TickPerf(DeltaSeconds);
    }
}

void ACrossoverMatchMode::TickMatch(float DeltaSeconds, bool bDriveBots)
{
    if (Roster.Num() != PlayerCount || Rules.GetState().Phase == MatchPhase::Finished)
    {
        return;
    }

    if (IsBoostActive())
    {
        BoostSeconds = FMath::Max(0.0f, BoostSeconds - DeltaSeconds);
        if (!IsBoostActive())
        {
            Roster[HumanIndex]->SetArcadeBoost(false);
        }
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
        PlaceHeldBall();
        return;
    }

    const double Milliseconds = FractionalMilliseconds + static_cast<double>(DeltaSeconds) * 1000.0;
    const int32 WholeMilliseconds = static_cast<int32>(Milliseconds);
    FractionalMilliseconds = Milliseconds - WholeMilliseconds;
    Rules.AdvanceTime(WholeMilliseconds);
    if (Rules.GetState().Ball == BallPhase::AwaitingCheckIn)
    {
        Feedback = TEXT("Shot clock turnover");
        ArcadeAudio->Play(ECrossoverArcadeCue::Buzzer);
        SetUpCheckIn();
        return;
    }
    if (Rules.GetState().Phase == MatchPhase::Finished)
    {
        bCharging = false;
        ArcadeAudio->Play(ECrossoverArcadeCue::Win);
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
        const FVector Target = PassReceiver == INDEX_NONE ? FlightEnd : Roster[PassReceiver]->GetBallCarryLocation();
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
        PlaceHeldBall();
        if (Court->IsOutOfBounds(Carrier.GetActorLocation()) && Rules.OutOfBounds(TeamOf(CarrierIndex)))
        {
            Feedback = TEXT("Out of bounds");
            ArcadeAudio->Play(ECrossoverArcadeCue::Buzzer);
            SetUpCheckIn();
            return;
        }
        if (Court->IsOutsideArc(Carrier.GetActorLocation()) && Rules.ClearArc(TeamOf(CarrierIndex)))
        {
            Feedback = TEXT("Ball cleared");
            ArcadeAudio->Play(ECrossoverArcadeCue::Clear);
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
            if (!Rules.GetState().RequiresClear && BotHoldSeconds >= BotShotDelay[Difficulty]
                && (FVector::DistSquared2D(Target, Bot.GetActorLocation()) < FMath::Square(125.0) || BotHoldSeconds >= 4.0f))
            {
                const float Spread = BotShotSpread[Difficulty];
                TakeShot(Index, BotRandom.FRandRange(0.62f - Spread, 0.62f + Spread));
            }
        }
        else if (bOffense)
        {
            const int32 RelativeSlot = (Index - CarrierIndex + TeamSize) % TeamSize;
            const double Side = RelativeSlot == 1 ? -1.0 : 1.0;
            const double Swing = FMath::Sin(Rules.GetState().ShotClockRemainingMilliseconds * 0.0008) * 85.0;
            Bot.DriveBotTowards(FVector(500.0 + Side * Swing, Side * 480.0, 0.0), 85.0f);
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
                ArcadeAudio->Play(ECrossoverArcadeCue::Steal);
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
