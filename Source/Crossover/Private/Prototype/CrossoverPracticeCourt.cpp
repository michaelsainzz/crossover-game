#include "Prototype/CrossoverPracticeCourt.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr double HalfLength = 1100.0;
constexpr double HalfWidth = 750.0;
constexpr double EdgeX = HalfLength - 12.0;
constexpr double EdgeY = HalfWidth - 12.0;
constexpr double ArcRadius = 600.0;
constexpr double CubeSize = 100.0;
const FVector RimCenter(900.0, 0.0, 305.0);

void AddStrip(UInstancedStaticMeshComponent& Mesh, const FVector& Start, const FVector& End,
    double Width, double Thickness)
{
    const FVector Delta = End - Start;
    const FTransform Transform(
        Delta.Rotation(),
        (Start + End) * 0.5,
        FVector(Delta.Size() + Width * 0.5, Width, Thickness) / CubeSize);
    Mesh.AddInstance(Transform, false);
}

void AddArc(UInstancedStaticMeshComponent& Mesh, const FVector& Center, double Radius,
    double FirstAngle, double Sweep, int32 Segments, double Width, double Thickness)
{
    auto PointAt = [&](int32 Index)
    {
        const double Angle = FirstAngle + Sweep * Index / Segments;
        return Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0) * Radius;
    };

    for (int32 Index = 0; Index < Segments; ++Index)
    {
        AddStrip(Mesh, PointAt(Index), PointAt(Index + 1), Width, Thickness);
    }
}
}

ACrossoverPracticeCourt::ACrossoverPracticeCourt()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent* Origin = CreateDefaultSubobject<USceneComponent>(TEXT("CourtOrigin"));
    SetRootComponent(Origin);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    const auto PlaceCube = [&](FName Name, const FVector& Location, const FVector& Size)
    {
        UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Part->SetupAttachment(Origin);
        Part->SetStaticMesh(Cube.Object);
        Part->SetRelativeLocation(Location);
        Part->SetRelativeScale3D(Size / CubeSize);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        return Part;
    };

    UStaticMeshComponent* Floor = PlaceCube(TEXT("Floor"), FVector(0.0, 0.0, -12.0),
        FVector(HalfLength * 2.0, HalfWidth * 2.0, 24.0));
    Floor->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

    PlaceCube(TEXT("Backboard"), FVector(940.0, 0.0, 355.0), FVector(12.0, 180.0, 105.0));
    PlaceCube(TEXT("Post"), FVector(1040.0, 0.0, 168.0), FVector(24.0, 24.0, 336.0));
    PlaceCube(TEXT("BoardMount"), FVector(992.0, 0.0, 328.0), FVector(105.0, 20.0, 16.0));
    PlaceCube(TEXT("RimMount"), FVector(931.0, 0.0, RimCenter.Z), FVector(22.0, 11.0, 5.0));

    Markings = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Markings"));
    Rim = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Rim"));
    for (UInstancedStaticMeshComponent* Part : {Markings.Get(), Rim.Get()})
    {
        Part->SetupAttachment(Origin);
        Part->SetStaticMesh(Cube.Object);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    UDirectionalLightComponent* Light = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("CourtLight"));
    Light->SetupAttachment(Origin);
    Light->SetMobility(EComponentMobility::Movable);
    Light->SetRelativeRotation(FRotator(-42.0, 32.0, 0.0));
    Light->SetIntensity(12.0f);
}

void ACrossoverPracticeCourt::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    Markings->ClearInstances();
    constexpr double LineHeight = 1.5;
    const auto PaintLine = [&](double StartX, double StartY, double EndX, double EndY)
    {
        AddStrip(*Markings, FVector(StartX, StartY, LineHeight),
            FVector(EndX, EndY, LineHeight), 6.0, 2.0);
    };

    PaintLine(-EdgeX, -EdgeY, EdgeX, -EdgeY);
    PaintLine(-EdgeX, EdgeY, EdgeX, EdgeY);
    PaintLine(-EdgeX, -EdgeY, -EdgeX, EdgeY);
    PaintLine(EdgeX, -EdgeY, EdgeX, EdgeY);

    constexpr double KeyStart = 480.0;
    constexpr double KeyHalfWidth = 185.0;
    PaintLine(KeyStart, -KeyHalfWidth, EdgeX, -KeyHalfWidth);
    PaintLine(KeyStart, KeyHalfWidth, EdgeX, KeyHalfWidth);
    PaintLine(KeyStart, -KeyHalfWidth, KeyStart, KeyHalfWidth);

    AddArc(*Markings, FVector(RimCenter.X, 0.0, LineHeight), ArcRadius,
        0.5 * UE_PI, UE_PI, 24, 6.0, 2.0);
    PaintLine(RimCenter.X, -ArcRadius, EdgeX, -ArcRadius);
    PaintLine(RimCenter.X, ArcRadius, EdgeX, ArcRadius);

    Rim->ClearInstances();
    AddArc(*Rim, RimCenter, 23.0, 0.0, 2.0 * UE_PI, 16, 4.0, 4.0);
}

FVector ACrossoverPracticeCourt::GetRimLocation() const
{
    return GetActorTransform().TransformPosition(RimCenter);
}

bool ACrossoverPracticeCourt::IsOutsideArc(const FVector& WorldPosition) const
{
    const FVector LocalPosition = GetActorTransform().InverseTransformPosition(WorldPosition);
    const double ForwardDistance = FMath::Min(LocalPosition.X - RimCenter.X, 0.0);
    return FMath::Square(ForwardDistance) + FMath::Square(LocalPosition.Y) > FMath::Square(ArcRadius);
}

bool ACrossoverPracticeCourt::IsOutOfBounds(const FVector& WorldPosition) const
{
    const FVector LocalPosition = GetActorTransform().InverseTransformPosition(WorldPosition);
    return FMath::Abs(LocalPosition.X) > EdgeX || FMath::Abs(LocalPosition.Y) > EdgeY;
}
