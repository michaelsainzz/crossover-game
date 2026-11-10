#include "Prototype/CrossoverPracticePlayer.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr float ShotDuration = 0.58f;
constexpr float DunkDuration = 0.48f;
constexpr double FootHeight = -81.0;
const FName ColorParameter(TEXT("Color"));

void PoseLimb(UStaticMeshComponent& Mesh, const FVector& Start, const FVector& End, double Diameter)
{
    const FVector Axis = End - Start;
    Mesh.SetRelativeLocationAndRotation((Start + End) * 0.5,
        FQuat::FindBetweenNormals(FVector::UpVector, Axis.GetSafeNormal()));
    Mesh.SetRelativeScale3D(FVector(Diameter, Diameter, Axis.Size() + 2.0) / 100.0);
}
}

void ACrossoverPracticePlayer::BuildAthlete()
{
    AthleteRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AthleteVisualRoot"));
    AthleteRoot->SetupAttachment(GetCapsuleComponent());

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Tint(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    MatchMaterial = Tint.Object;

    const auto Part = [&](FName Name, UStaticMesh* Shape, const FVector& Location, const FVector& Size)
    {
        UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Mesh->SetupAttachment(AthleteRoot);
        Mesh->SetStaticMesh(Shape);
        Mesh->SetRelativeLocation(Location);
        Mesh->SetRelativeScale3D(Size / 100.0);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        return Mesh;
    };

    JerseyPieces.Add(Part(TEXT("Jersey"), Cube.Object, FVector(0, 0, 24), FVector(34, 46, 54)));
    JerseyPieces.Add(Part(TEXT("JerseyShoulders"), Sphere.Object, FVector(0, 0, 45), FVector(40, 59, 28)));
    DarkPieces.Add(Part(TEXT("Shorts"), Cube.Object, FVector(0, 0, -10), FVector(34, 44, 22)));
    AccentPieces.Add(Part(TEXT("JerseyHem"), Cube.Object, FVector(0, 0, -1), FVector(35, 47, 5)));
    AccentPieces.Add(Part(TEXT("JerseyStripe"), Cube.Object, FVector(17, 0, 28), FVector(3, 38, 8)));
    SkinPieces.Add(Part(TEXT("Neck"), Cylinder.Object, FVector(0, 0, 56), FVector(16, 16, 15)));
    SkinPieces.Add(Part(TEXT("Head"), Sphere.Object, FVector(2, 0, 76), FVector(33, 32, 38)));
    Hair = Part(TEXT("Hair"), Sphere.Object, FVector(-3, 0, 88), FVector(31, 32, 20));
    DarkPieces.Add(Hair);
    Headband = Part(TEXT("Headband"), Cylinder.Object, FVector(2, 0, 82), FVector(34, 34, 6));
    AccentPieces.Add(Headband);

    for (int32 Index = 0; Index < 2; ++Index)
    {
        const TCHAR* Side = Index == 0 ? TEXT("Left") : TEXT("Right");
        const auto Limb = [&](const TCHAR* Name, UStaticMesh* Shape, const FVector& Size)
        {
            return Part(FName(FString::Printf(TEXT("%s%s"), Side, Name)), Shape, FVector::ZeroVector, Size);
        };

        UpperLegs.Add(Limb(TEXT("Thigh"), Cylinder.Object, FVector(20, 20, 38)));
        LowerLegs.Add(Limb(TEXT("Shin"), Cylinder.Object, FVector(14, 14, 34)));
        UpperArms.Add(Limb(TEXT("UpperArm"), Cylinder.Object, FVector(15, 15, 24)));
        Forearms.Add(Limb(TEXT("Forearm"), Cylinder.Object, FVector(13, 13, 25)));
        Hands.Add(Limb(TEXT("Hand"), Sphere.Object, FVector(15, 13, 15)));
        Shoes.Add(Limb(TEXT("Shoe"), Cube.Object, FVector(30, 17, 12)));
        Soles.Add(Limb(TEXT("Sole"), Cube.Object, FVector(31, 18, 4)));

        DarkPieces.Add(UpperLegs[Index]);
        JerseyPieces.Add(Shoes[Index]);
        AccentPieces.Add(Soles[Index]);
        SkinPieces.Add(LowerLegs[Index]);
        SkinPieces.Add(UpperArms[Index]);
        SkinPieces.Add(Forearms[Index]);
        SkinPieces.Add(Hands[Index]);
    }

    ControlMarker = Part(TEXT("ControlMarker"), Cube.Object, FVector(0, 0, 119), FVector(20, 20, 8));
    ControlMarker->SetRelativeRotation(FRotator(0.0, 45.0, 0.0));
    ControlMarker->SetCastShadow(false);
    ControlMarker->SetVisibility(false);
}

void ACrossoverPracticePlayer::SetAthleteVariant(int32 Index)
{
    AthleteVariant = FMath::Clamp(Index, 0, 5);
    const bool bBald = AthleteVariant == 2;
    Hair->SetVisibility(!bBald);
    Hair->SetRelativeScale3D(AthleteVariant % 3 == 1 ? FVector(0.31, 0.32, 0.26) : FVector(0.31, 0.32, 0.20));
    Headband->SetVisibility(AthleteVariant % 2 == 0);
    UpdateAthleteMaterials();
}

void ACrossoverPracticePlayer::UpdateAthleteMaterials()
{
    if (!MatchMaterial)
    {
        return;
    }

    const auto Paint = [this](TObjectPtr<UMaterialInstanceDynamic>& Material,
        const TArray<TObjectPtr<UStaticMeshComponent>>& Pieces, const FLinearColor& Color)
    {
        if (!Material)
        {
            Material = UMaterialInstanceDynamic::Create(MatchMaterial, this);
            if (!Material)
            {
                return;
            }
            for (UStaticMeshComponent* Piece : Pieces)
            {
                Piece->SetMaterial(0, Material);
            }
        }
        Material->SetVectorParameterValue(ColorParameter, Color);
    };

    static const FLinearColor SkinTones[] = {
        FLinearColor(0.46f, 0.23f, 0.12f), FLinearColor(0.73f, 0.43f, 0.29f),
        FLinearColor(0.23f, 0.10f, 0.057f), FLinearColor(0.57f, 0.29f, 0.15f),
        FLinearColor(0.32f, 0.15f, 0.080f), FLinearColor(0.83f, 0.54f, 0.37f),
    };

    Paint(TeamMaterial, JerseyPieces,
        bHomeTeam ? FLinearColor(0.02f, 0.58f, 0.63f) : FLinearColor(0.94f, 0.20f, 0.025f));
    Paint(AccentMaterial, AccentPieces,
        bArcadeBoost ? FLinearColor(1.0f, 0.76f, 0.07f) : FLinearColor(0.8f, 0.83f, 0.74f));
    Paint(SkinMaterial, SkinPieces, SkinTones[AthleteVariant]);
    Paint(DarkMaterial, DarkPieces,
        bHomeTeam ? FLinearColor(0.025f, 0.065f, 0.10f) : FLinearColor(0.11f, 0.027f, 0.037f));

    if (!Cast<UMaterialInstanceDynamic>(ControlMarker->GetMaterial(0)))
    {
        if (UMaterialInstanceDynamic* MarkerMaterial = ControlMarker->CreateDynamicMaterialInstance(0, MatchMaterial))
        {
            MarkerMaterial->SetVectorParameterValue(ColorParameter, FLinearColor(1.0f, 0.77f, 0.04f));
        }
    }
}

float ACrossoverPracticePlayer::GetDribbleHeight() const
{
    const float Bounce = FMath::Abs(FMath::Cos(UE_PI * DribblePhase));
    return FMath::Lerp(-69.0f, 4.0f, Bounce);
}

FVector ACrossoverPracticePlayer::GetBallCarryLocation() const
{
    const double Height = bCarryingBall ? GetDribbleHeight() : -4.0;
    return GetActorTransform().TransformPosition(FVector(24, 37, Height));
}

FVector ACrossoverPracticePlayer::GetShotReleaseLocation() const
{
    return GetActorTransform().TransformPosition(FVector(20, bDunkVisual ? 24 : 0, bDunkVisual ? 137 : 108));
}

void ACrossoverPracticePlayer::PlayShotAnimation(bool bDunk)
{
    bDunkVisual = bDunk;
    ShotVisualTime = bDunk ? DunkDuration : ShotDuration;
}

void ACrossoverPracticePlayer::ResetAthleteMotion()
{
    bCarryingBall = false;
    bDunkVisual = false;
    StridePhase = 0.0f;
    GaitWeight = 0.0f;
    DribblePhase = 0.0f;
    ShotVisualTime = 0.0f;
    SetArcadeBoost(false);
    UpdateAthlete(0.0f);
}

void ACrossoverPracticePlayer::UpdateAthlete(float DeltaSeconds)
{
    const float Speed = static_cast<float>(GetVelocity().Size2D());
    const float Drive = FMath::Clamp(Speed / 760.0f, 0.0f, 1.0f);
    GaitWeight = FMath::FInterpTo(GaitWeight, FMath::Clamp(Speed / 165.0f, 0.0f, 1.0f), DeltaSeconds, 14.0f);
    const float Reach = FMath::Lerp(22.0f, 45.0f, Drive);
    StridePhase = FMath::Fmod(StridePhase + Speed * DeltaSeconds / (4.0f * Reach), 1.0f);
    DribblePhase = FMath::Fmod(DribblePhase + DeltaSeconds * (2.1f + Drive), 1.0f);
    IdlePhase = FMath::Fmod(IdlePhase + DeltaSeconds * 3.0f, 2.0f * UE_PI);

    const bool bShooting = ShotVisualTime > 0.0f;
    const float Duration = bDunkVisual ? DunkDuration : ShotDuration;
    const float ShotProgress = 1.0f - ShotVisualTime / Duration;
    const float Rise = bShooting ? FMath::Sin(UE_PI * ShotProgress) : 0.0f;
    ShotVisualTime = FMath::Max(0.0f, ShotVisualTime - DeltaSeconds);

    const float Bob = FMath::Sin(IdlePhase) * (1.0f - GaitWeight)
        + FMath::Abs(FMath::Sin(2.0f * UE_PI * StridePhase)) * GaitWeight * 4.0f;
    const float Jump = Rise * (bDunkVisual ? 43.0f : 25.0f);
    AthleteRoot->SetRelativeLocation(FVector(0, 0, Bob + Jump));
    AthleteRoot->SetRelativeRotation(FRotator(Drive * -10.0f * (1.0f - Rise), 0, 0));

    for (int32 Index = 0; Index < 2; ++Index)
    {
        const double Side = Index == 0 ? -1.0 : 1.0;
        const float Phase = FMath::Fmod(StridePhase + 0.5f * Index, 1.0f);
        const bool bSwing = Phase < 0.5f;
        const float Step = bSwing ? Phase * 2.0f : (Phase - 0.5f) * 2.0f;
        const float FootX = FMath::Lerp(-Reach, Reach, bSwing ? Step : 1.0f - Step) * GaitWeight;
        const float Lift = bSwing ? FMath::Sin(UE_PI * Step) * (15.0f + Drive * 12.0f) * GaitWeight : 0.0f;
        const FVector Hip(0, 13 * Side, -12);
        const FVector Foot(FootX + Rise * Side * 12.0, 14 * Side, FootHeight + Lift + Rise * 8.0);
        const FVector Ankle = Foot + FVector(-5, 0, 5);
        const FVector Knee = (Hip + Ankle) * 0.5 + FVector(10.0 + Lift * 0.2 + Rise * 11.0, Side * 1.5, 1);
        PoseLimb(*UpperLegs[Index], Hip, Knee, 20.0);
        PoseLimb(*LowerLegs[Index], Knee, Ankle, 14.0);
        Shoes[Index]->SetRelativeLocation(Foot + FVector(6, 0, 0));
        Soles[Index]->SetRelativeLocation(Foot + FVector(6, 0, -5));

        const FVector Shoulder(0, 27 * Side, 47);
        FVector Elbow(-FootX * 0.35 - 6.0, 35 * Side, 20 + Drive * 4);
        FVector Hand(-FootX * 0.60 + 10.0, 35 * Side, 3 + Drive * 7);
        if (bCarryingBall && Index == 1)
        {
            Elbow = FVector(9, 35, 23);
            Hand = FVector(25, 37, FMath::Max(-18.0f, GetDribbleHeight() + 8.0f));
        }
        if (bShooting)
        {
            const float ReachUp = FMath::Clamp(ShotProgress * 6.0f + 0.45f, 0.0f, 1.0f) * (1.0f - FMath::Square(ShotProgress));
            Elbow = FMath::Lerp(Elbow, FVector(16, 21 * Side, 72), ReachUp);
            Hand = FMath::Lerp(Hand, FVector(24, (bDunkVisual ? 22 : 11) * Side, 102), ReachUp);
        }
        PoseLimb(*UpperArms[Index], Shoulder, Elbow, 16.0);
        PoseLimb(*Forearms[Index], Elbow, Hand, 13.0);
        Hands[Index]->SetRelativeLocation(Hand);
    }
}
