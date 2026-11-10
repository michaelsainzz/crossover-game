#include "Prototype/CrossoverMatchMode.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Prototype/CrossoverArcadeAudio.h"
#include "Prototype/CrossoverPracticePlayer.h"

namespace
{
constexpr int32 TrailCount = 7;
}

void ACrossoverMatchMode::InitializeArcadePresentation()
{
    CameraOrigin = CourtCamera->GetActorLocation();
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(
        nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (!Material)
    {
        return;
    }
    auto Tint = [Material](UStaticMeshComponent& Mesh, FLinearColor Color)
    {
        if (UMaterialInstanceDynamic* Instance = Mesh.CreateDynamicMaterialInstance(0, Material))
        {
            Instance->SetVectorParameterValue(TEXT("Color"), Color);
        }
    };

    Tint(*Ball->GetStaticMeshComponent(), FLinearColor(1.0f, 0.28f, 0.04f));
    Ball->SetActorScale3D(FVector(0.28));
    for (int32 Index = 0; Index < TrailCount; ++Index)
    {
        AStaticMeshActor* Ghost = GetWorld()->SpawnActor<AStaticMeshActor>();
        if (!Ghost)
        {
            break;
        }
        UStaticMeshComponent* Mesh = Ghost->GetStaticMeshComponent();
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetStaticMesh(BallMesh);
        Mesh->SetCastShadow(false);
        Tint(*Mesh, FLinearColor(1.0f, 0.68f, 0.18f));
        Ghost->SetActorScale3D(FVector(FMath::Lerp(0.17, 0.025, Index / 6.0)));
        Ghost->SetActorHiddenInGame(true);
        BallTrail.Add(Ghost);
    }
}

void ACrossoverMatchMode::PlaceHeldBall()
{
    if (Roster.IsValidIndex(CarrierIndex))
    {
        Ball->SetActorLocation(Roster[CarrierIndex]->GetBallCarryLocation());
    }
}

void ACrossoverMatchMode::UpdateArcadePresentation(float DeltaSeconds)
{
    if (Roster.Num() != 6)
    {
        return;
    }
    ScoreFlash = FMath::Max(0.0f, ScoreFlash - DeltaSeconds);
    const bool bFlight = Shot != Crossover::Rules::InvalidShot || PassReceiver != INDEX_NONE;
    const FVector BallPosition = Ball->GetActorLocation();
    const FVector CameraTarget = CameraOrigin + FVector(FMath::Clamp(BallPosition.X * 0.10, -100.0, 100.0), 0.0, 0.0);
    CourtCamera->SetActorLocation(FMath::VInterpTo(CourtCamera->GetActorLocation(), CameraTarget, DeltaSeconds, 3.5f));
    for (int32 Index = BallTrail.Num() - 1; Index >= 0; --Index)
    {
        AStaticMeshActor* Ghost = BallTrail[Index];
        Ghost->SetActorHiddenInGame(!bFlight);
        if (bFlight)
        {
            const FVector Target = Index == 0 ? BallPosition : BallTrail[Index - 1]->GetActorLocation();
            Ghost->SetActorLocation(bWasFlight
                ? FMath::VInterpTo(Ghost->GetActorLocation(), Target, DeltaSeconds, 27.0f) : FlightStart);
        }
    }
    bWasFlight = bFlight;

    for (int32 Index = 0; Index < Roster.Num(); ++Index)
    {
        Roster[Index]->SetCarryingBall(Index == CarrierIndex && !bFlight && !bLooseBall);
    }
    BounceDelay -= DeltaSeconds;
    if (!bFlight && !bLooseBall && Rules.GetState().Ball == Crossover::Rules::BallPhase::Live
        && Roster.IsValidIndex(CarrierIndex) && Roster[CarrierIndex]->GetVelocity().SizeSquared2D() > FMath::Square(70.0)
        && BounceDelay <= 0.0f)
    {
        ArcadeAudio->Play(ECrossoverArcadeCue::Bounce);
        BounceDelay = 0.36f;
    }
}
