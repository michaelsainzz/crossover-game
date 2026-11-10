#include "Prototype/CrossoverPracticeMode.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "HAL/PlatformMisc.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Prototype/CrossoverPracticeCourt.h"
#include "Prototype/CrossoverPracticePlayer.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr float ChargeDuration = 1.0f;
constexpr double DunkDistance = 180.0;
const FVector StartPosition(-600.0, 0.0, 100.0);
}

ACrossoverPracticeMode::ACrossoverPracticeMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = ACrossoverPracticePlayer::StaticClass();
    HUDClass = ACrossoverPracticeHUD::StaticClass();

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    BallMesh = Sphere.Object;
}

AActor* ACrossoverPracticeMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (AActor* ExistingStart = Super::ChoosePlayerStart_Implementation(Player))
    {
        return ExistingStart;
    }
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    return GetWorld()->SpawnActor<APlayerStart>(StartPosition, FRotator::ZeroRotator, Spawn);
}

void ACrossoverPracticeMode::StartPlay()
{
    UWorld* World = GetWorld();
    Court = World->SpawnActor<ACrossoverPracticeCourt>();
    const FVector CameraLocation(0.0, 2300.0, 1650.0);
    const FRotator CameraRotation = (FVector(0.0, 0.0, 100.0) - CameraLocation).Rotation();
    CourtCamera = World->SpawnActor<ACameraActor>(CameraLocation, CameraRotation);
    if (CourtCamera)
    {
        CourtCamera->GetCameraComponent()->SetFieldOfView(55.0f);
    }

    Ball = World->SpawnActor<AStaticMeshActor>();
    if (Ball)
    {
        UStaticMeshComponent* Mesh = Ball->GetStaticMeshComponent();
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetStaticMesh(BallMesh);
        Ball->SetActorScale3D(FVector(0.24));
        if (UMaterialInstanceDynamic* Material = Mesh->CreateDynamicMaterialInstance(0))
        {
            Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.95f, 0.25f, 0.025f));
        }
    }

    Super::StartPlay();

    if (APlayerController* Controller = World->GetFirstPlayerController())
    {
        if (!Controller->GetPawn())
        {
            RestartPlayer(Controller);
        }
        if (CourtCamera)
        {
            Controller->SetViewTarget(CourtCamera);
        }
    }

    if (FParse::Param(FCommandLine::Get(), TEXT("CrossoverSmoke")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &ACrossoverPracticeMode::VerifySmokeStart);
    }
}

void ACrossoverPracticeMode::VerifySmokeStart()
{
    APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    if (Controller && IsValid(Court.Get()) && IsValid(Ball.Get()) && IsValid(BallMesh.Get())
        && IsValid(CourtCamera.Get()) && GetPracticePlayer() && Controller->GetViewTarget() == CourtCamera.Get())
    {
        UE_LOG(LogTemp, Display, TEXT("CROSSOVER_SMOKE_READY"));
        FPlatformMisc::RequestExit(false, TEXT("Crossover practice smoke completed"));
        return;
    }

    UE_LOG(LogTemp, Error, TEXT("Crossover practice smoke could not set up the court, ball, camera, or player."));
    FPlatformMisc::RequestExitWithStatus(false, 1, TEXT("Crossover practice smoke failed"));
}

ACrossoverPracticePlayer* ACrossoverPracticeMode::GetPracticePlayer() const
{
    const APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    return Controller ? Cast<ACrossoverPracticePlayer>(Controller->GetPawn()) : nullptr;
}

FVector ACrossoverPracticeMode::GetHeldBallPosition(const ACrossoverPracticePlayer& Player) const
{
    return Player.GetActorLocation() + Player.GetActorRightVector() * 40.0 + FVector(0.0, 0.0, 30.0);
}

void ACrossoverPracticeMode::StartShot(const ACrossoverPracticePlayer& Player)
{
    if (&Player == GetPracticePlayer() && Court && Ball && !bBallInFlight)
    {
        bCharging = true;
        ChargeSeconds = 0.0f;
        ShotFeedback.Reset();
    }
}

void ACrossoverPracticeMode::ReleaseShot(const ACrossoverPracticePlayer& Player)
{
    if (&Player != GetPracticePlayer() || !bCharging || !Court || !Ball)
    {
        return;
    }
    bCharging = false;

    FlightStart = GetHeldBallPosition(Player);
    FlightEnd = Court->GetRimLocation();
    const double Distance = FVector::Dist2D(Player.GetActorLocation(), FlightEnd);
    const float Charge = GetChargeFraction();
    const bool bDunk = Distance <= DunkDistance;
    const bool bMade = bDunk || (Charge >= MakeWindowStart && Charge <= MakeWindowEnd);
    PendingPoints = bMade ? (Court->IsOutsideArc(Player.GetActorLocation()) ? 2 : 1) : 0;
    if (!bMade)
    {
        FlightEnd.X += Charge < MakeWindowStart ? -150.0 : 135.0;
    }

    ShotFeedback = bDunk ? TEXT("Dunk!") : (bMade ? TEXT("Good release!") : TEXT("Missed release"));
    FlightDuration = bDunk ? 0.32f : FMath::Clamp(static_cast<float>(Distance / 1250.0), 0.45f, 1.3f);
    FlightArc = bDunk ? 100.0f : 255.0f;
    FlightSeconds = 0.0f;
    bBallInFlight = true;
}

void ACrossoverPracticeMode::CancelShot(const ACrossoverPracticePlayer& Player)
{
    if (&Player == GetPracticePlayer())
    {
        bCharging = false;
    }
}

float ACrossoverPracticeMode::GetChargeFraction() const
{
    return FMath::Clamp(ChargeSeconds / ChargeDuration, 0.0f, 1.0f);
}

void ACrossoverPracticeMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Ball)
    {
        return;
    }
    if (bCharging)
    {
        ChargeSeconds = FMath::Min(ChargeSeconds + DeltaSeconds, ChargeDuration);
    }
    if (bBallInFlight)
    {
        FlightSeconds += DeltaSeconds;
        const float Alpha = FMath::Min(FlightSeconds / FlightDuration, 1.0f);
        FVector Position = FMath::Lerp(FlightStart, FlightEnd, Alpha);
        Position.Z += 4.0f * FlightArc * Alpha * (1.0f - Alpha);
        Ball->SetActorLocation(Position);
        if (Alpha >= 1.0f)
        {
            PracticeScore += PendingPoints;
            bBallInFlight = false;
        }
    }
    else if (const ACrossoverPracticePlayer* Player = GetPracticePlayer())
    {
        Ball->SetActorLocation(GetHeldBallPosition(*Player));
    }
}

void ACrossoverPracticeHUD::DrawHUD()
{
    Super::DrawHUD();
    const ACrossoverPracticeMode* Mode = GetWorld()->GetAuthGameMode<ACrossoverPracticeMode>();
    if (!Canvas || !Mode)
    {
        return;
    }

    UFont* Font = GEngine->GetSmallFont();
    DrawText(FString::Printf(TEXT("CROSSOVER  /  PRACTICE     POINTS  %d"), Mode->GetPracticeScore()),
        FLinearColor::White, 24.0f, 20.0f, Font, 1.5f);
    DrawText(TEXT("Move: WASD / Left stick   Sprint: Shift / LB   Hold + release shot: Space / Bottom face button"),
        FLinearColor::White, 24.0f, 48.0f, Font);
    DrawText(Mode->GetShotFeedback(), FLinearColor::White, 24.0f, 72.0f, Font, 1.25f);

    if (Mode->IsChargingShot())
    {
        constexpr float Width = 220.0f;
        constexpr float Left = 24.0f;
        constexpr float Top = 102.0f;
        DrawRect(FLinearColor(0.05f, 0.05f, 0.05f, 0.9f), Left, Top, Width, 16.0f);
        DrawRect(FLinearColor(0.12f, 0.7f, 0.28f),
            Left + Width * ACrossoverPracticeMode::MakeWindowStart, Top,
            Width * (ACrossoverPracticeMode::MakeWindowEnd - ACrossoverPracticeMode::MakeWindowStart), 16.0f);
        DrawRect(FLinearColor::White, Left + Width * Mode->GetChargeFraction(), Top, 3.0f, 16.0f);
    }
}
