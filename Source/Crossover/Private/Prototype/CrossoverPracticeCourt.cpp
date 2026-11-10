#include "Prototype/CrossoverPracticeCourt.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
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
const FName TintParameter(TEXT("Color"));

enum EPalette
{
    Concrete, CourtTrim, KeyPaint, Lines, Steel, Amber, Teal, Backboard,
    FarBuildings, NearBuildings, Windows, SkyPurple, SkyViolet, SkyRose, SkyGold,
};

struct FPaletteEntry
{
    const TCHAR* Name;
    FColor Tint;
};

const FPaletteEntry Palette[] = {
    {TEXT("Concrete"), FColor(47, 53, 77)},
    {TEXT("CourtTrim"), FColor(102, 116, 140)},
    {TEXT("KeyPaint"), FColor(43, 105, 121)},
    {TEXT("Lines"), FColor(239, 224, 185)},
    {TEXT("Steel"), FColor(45, 59, 83)},
    {TEXT("Amber"), FColor(242, 100, 55)},
    {TEXT("Teal"), FColor(54, 179, 167)},
    {TEXT("Backboard"), FColor(168, 190, 201)},
    {TEXT("FarBuildings"), FColor(89, 67, 111)},
    {TEXT("NearBuildings"), FColor(52, 51, 78)},
    {TEXT("Windows"), FColor(244, 169, 94)},
    {TEXT("SkyPurple"), FColor(73, 54, 106)},
    {TEXT("SkyViolet"), FColor(124, 74, 134)},
    {TEXT("SkyRose"), FColor(183, 96, 130)},
    {TEXT("SkyGold"), FColor(225, 131, 118)},
};

void Tint(UStaticMeshComponent& Mesh, UMaterialInterface* Material, FColor Color)
{
    if (Material)
    {
        if (UMaterialInstanceDynamic* Instance = Mesh.CreateDynamicMaterialInstance(0, Material))
        {
            Instance->SetVectorParameterValue(TintParameter, FLinearColor::FromSRGBColor(Color));
        }
    }
}

void AddBox(UInstancedStaticMeshComponent& Mesh, const FVector& Center, const FVector& Size,
    const FRotator& Rotation = FRotator::ZeroRotator)
{
    Mesh.AddInstance(FTransform(Rotation, Center, Size / CubeSize), false);
}

void AddStrip(UInstancedStaticMeshComponent& Mesh, const FVector& Start, const FVector& End,
    double Width, double Thickness)
{
    const FVector Delta = End - Start;
    AddBox(Mesh, (Start + End) * 0.5, FVector(Delta.Size() + Width * 0.45, Width, Thickness), Delta.Rotation());
}

FVector CirclePoint(const FVector& Center, double Radius, double Angle)
{
    return Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0) * Radius;
}

void AddArc(UInstancedStaticMeshComponent& Mesh, const FVector& Center, double Radius,
    double FirstAngle, double Sweep, int32 Segments, double Width, double Thickness)
{
    for (int32 Index = 0; Index < Segments; ++Index)
    {
        const double Angle = FirstAngle + Sweep * Index / Segments;
        const double Next = FirstAngle + Sweep * (Index + 1) / Segments;
        AddStrip(Mesh, CirclePoint(Center, Radius, Angle), CirclePoint(Center, Radius, Next), Width, Thickness);
    }
}
}

ACrossoverPracticeCourt::ACrossoverPracticeCourt()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent* Origin = CreateDefaultSubobject<USceneComponent>(TEXT("CourtOrigin"));
    SetRootComponent(Origin);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseColor(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    ColorMaterial = BaseColor.Object;

    Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
    Floor->SetupAttachment(Origin);
    Floor->SetStaticMesh(Cube.Object);
    Floor->SetRelativeLocation(FVector(0.0, 0.0, -12.0));
    Floor->SetRelativeScale3D(FVector(HalfLength * 2.0, HalfWidth * 2.0, 24.0) / CubeSize);
    Floor->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    Floor->SetCastShadow(false);

    for (const FPaletteEntry& Entry : Palette)
    {
        UInstancedStaticMeshComponent* Mesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(FName(Entry.Name));
        Mesh->SetupAttachment(Origin);
        Mesh->SetStaticMesh(Cube.Object);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false);
        PaletteMeshes.Add(Mesh);
    }

    Sun = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SunDisc"));
    Sun->SetupAttachment(Origin);
    Sun->SetStaticMesh(Cylinder.Object);
    Sun->SetRelativeLocation(FVector(-2250.0, -6750.0, -1460.0));
    Sun->SetRelativeRotation(FRotator(0.0, 0.0, 90.0));
    Sun->SetRelativeScale3D(FVector(8.0, 8.0, 0.04));
    Sun->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Sun->SetCastShadow(false);

    const auto MakeSign = [this, Origin](FName Name, const TCHAR* Label, const FVector& Center,
        const FRotator& Rotation, float Size, FColor Color)
    {
        UTextRenderComponent* Sign = CreateDefaultSubobject<UTextRenderComponent>(Name);
        Sign->SetupAttachment(Origin);
        Sign->SetRelativeLocation(Center);
        Sign->SetRelativeRotation(Rotation);
        Sign->SetHorizontalAlignment(EHTA_Center);
        Sign->SetVerticalAlignment(EVRTA_TextCenter);
        Sign->SetWorldSize(Size);
        Sign->SetTextRenderColor(Color);
        Sign->SetText(FText::FromString(Label));
        Sign->SetCastShadow(false);
    };

    MakeSign(TEXT("DistrictSign"), TEXT("DUSK DISTRICT"), FVector(-690.0, -841.0, 243.0),
        FRotator(0.0, 90.0, 0.0), 38.0f, FColor(255, 222, 168));
    MakeSign(TEXT("CourtWordmark"), TEXT("C R O S S O V E R"), FVector(-465.0, 800.0, -2.0),
        FRotator(90.0, 90.0, 0.0), 29.0f, FColor(225, 233, 230));
    MakeSign(TEXT("CourtNumber"), TEXT("COURT 01"), FVector(720.0, 800.0, -2.0),
        FRotator(90.0, 90.0, 0.0), 27.0f, FColor(255, 189, 124));

    UDirectionalLightComponent* Key = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("WarmCourtLight"));
    Key->SetupAttachment(Origin);
    Key->SetMobility(EComponentMobility::Movable);
    Key->SetRelativeRotation(FRotator(-51.0, 26.0, 0.0));
    Key->SetLightColor(FLinearColor(1.0f, 0.83f, 0.69f));
    Key->SetIntensity(5.5f);

    UDirectionalLightComponent* Fill = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("CoolCourtFill"));
    Fill->SetupAttachment(Origin);
    Fill->SetMobility(EComponentMobility::Movable);
    Fill->SetRelativeRotation(FRotator(-34.0, -95.0, 0.0));
    Fill->SetLightColor(FLinearColor(0.69f, 0.79f, 1.0f));
    Fill->SetIntensity(3.25f);
    Fill->SetCastShadows(false);
}

void ACrossoverPracticeCourt::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    Tint(*Floor, ColorMaterial, FColor(49, 75, 96));
    Tint(*Sun, ColorMaterial, FColor(255, 199, 132));
    for (int32 Index = 0; Index < PaletteMeshes.Num(); ++Index)
    {
        PaletteMeshes[Index]->ClearInstances();
        Tint(*PaletteMeshes[Index], ColorMaterial, Palette[Index].Tint);
    }

    const auto Box = [&](EPalette Layer, const FVector& Center, const FVector& Size)
    {
        AddBox(*PaletteMeshes[Layer], Center, Size);
    };

    // Deck and setbacks leave the entire playable rectangle unchanged.
    Box(Concrete, FVector(0.0, 0.0, -195.0), FVector(2640.0, 1760.0, 378.0));
    Box(CourtTrim, FVector(0.0, 0.0, -35.0), FVector(2610.0, 1750.0, 62.0));
    Box(Teal, FVector(0.0, 876.0, -45.0), FVector(2630.0, 9.0, 16.0));
    Box(Amber, FVector(0.0, -873.0, -19.0), FVector(2560.0, 10.0, 16.0));
    Box(KeyPaint, FVector((480.0 + EdgeX) * 0.5, 0.0, 0.15),
        FVector(EdgeX - 480.0, 370.0, 0.3));
    Box(KeyPaint, FVector(-792.0, 0.0, 0.12), FVector(570.0, 1472.0, 0.24));

    UInstancedStaticMeshComponent& Paint = *PaletteMeshes[Lines];
    const auto Line = [&](double StartX, double StartY, double EndX, double EndY)
    {
        AddStrip(Paint, FVector(StartX, StartY, 0.65), FVector(EndX, EndY, 0.65), 7.0, 0.7);
    };
    Line(-EdgeX, -EdgeY, EdgeX, -EdgeY);
    Line(-EdgeX, EdgeY, EdgeX, EdgeY);
    Line(-EdgeX, -EdgeY, -EdgeX, EdgeY);
    Line(EdgeX, -EdgeY, EdgeX, EdgeY);
    Line(480.0, -185.0, EdgeX, -185.0);
    Line(480.0, 185.0, EdgeX, 185.0);
    Line(480.0, -185.0, 480.0, 185.0);
    AddArc(Paint, FVector(480.0, 0.0, 0.65), 185.0, 0.5 * UE_PI, UE_PI, 18, 6.0, 0.7);
    AddArc(Paint, FVector(RimCenter.X, 0.0, 0.65), ArcRadius, 0.5 * UE_PI, UE_PI, 36, 7.0, 0.7);
    Line(RimCenter.X, -ArcRadius, EdgeX, -ArcRadius);
    Line(RimCenter.X, ArcRadius, EdgeX, ArcRadius);
    for (double Side : {-1.0, 1.0})
    {
        for (int32 Tick = 0; Tick < 3; ++Tick)
        {
            const double X = 590.0 + 100.0 * Tick;
            Line(X, Side * 185.0, X, Side * 215.0);
        }
    }

    // Three painted chevrons act as a court crest on the far side of the arc.
    for (int32 Chevron = 0; Chevron < 3; ++Chevron)
    {
        const double X = -945.0 + 110.0 * Chevron;
        UInstancedStaticMeshComponent& Crest = *PaletteMeshes[Chevron == 2 ? Amber : Lines];
        AddStrip(Crest, FVector(X, -123.0, 0.85), FVector(X + 78.0, 0.0, 0.85), 20.0, 0.65);
        AddStrip(Crest, FVector(X + 78.0, 0.0, 0.85), FVector(X, 123.0, 0.85), 20.0, 0.65);
    }

    // Hoop and contrasting backboard hardware. All decorations remain nonblocking.
    Box(Backboard, FVector(940.0, 0.0, 355.0), FVector(12.0, 180.0, 105.0));
    Box(Steel, FVector(1040.0, 0.0, 168.0), FVector(24.0, 24.0, 336.0));
    Box(Amber, FVector(1040.0, 0.0, 64.0), FVector(40.0, 42.0, 128.0));
    Box(Steel, FVector(992.0, 0.0, 328.0), FVector(105.0, 20.0, 16.0));
    Box(Amber, FVector(931.0, 0.0, RimCenter.Z), FVector(22.0, 11.0, 5.0));
    Box(Lines, FVector(933.7, 0.0, 375.0), FVector(1.5, 64.0, 4.0));
    Box(Lines, FVector(933.7, 0.0, 320.0), FVector(1.5, 64.0, 4.0));
    for (double Side : {-1.0, 1.0})
    {
        Box(Lines, FVector(933.7, Side * 31.0, 347.5), FVector(1.5, 4.0, 57.0));
        Box(Teal, FVector(940.0, Side * 91.0, 355.0), FVector(17.0, 5.0, 110.0));
    }
    AddArc(*PaletteMeshes[Amber], RimCenter, 23.0, 0.0, 2.0 * UE_PI, 20, 4.5, 4.5);
    const FVector NetBottom = RimCenter - FVector(0.0, 0.0, 40.0);
    for (int32 Strand = 0; Strand < 10; ++Strand)
    {
        const double Angle = 2.0 * UE_PI * Strand / 10;
        const FVector Top = CirclePoint(RimCenter, 22.0, Angle);
        AddStrip(Paint, Top, CirclePoint(NetBottom, 12.0, Angle + 0.30), 1.6, 1.6);
        AddStrip(Paint, Top, CirclePoint(NetBottom, 12.0, Angle - 0.30), 1.6, 1.6);
    }
    AddArc(Paint, NetBottom, 12.0, 0.0, 2.0 * UE_PI, 10, 1.6, 1.6);

    // Back fence and left return frame the court without putting rails in the camera's way.
    constexpr double FenceY = -846.0;
    for (int32 Post = 0; Post <= 12; ++Post)
    {
        const double X = -1240.0 + 205.0 * Post;
        Box(Steel, FVector(X, FenceY, 143.0), FVector(13.0, 17.0, 302.0));
        Box(Teal, FVector(X, FenceY, 301.0), FVector(16.0, 20.0, 12.0));
    }
    for (double Height : {24.0, 113.0, 205.0, 291.0})
    {
        Box(Steel, FVector(-10.0, FenceY, Height), FVector(2485.0, 11.0, 8.0));
    }
    for (int32 Panel = 0; Panel < 12; ++Panel)
    {
        const double X = -1240.0 + 205.0 * Panel;
        for (int32 Wire = 1; Wire < 5; ++Wire)
        {
            Box(Steel, FVector(X + Wire * 41.0, FenceY, 155.0), FVector(3.0, 5.0, 265.0));
        }
    }
    for (int32 Section = 0; Section <= 4; ++Section)
    {
        Box(Steel, FVector(-1240.0, FenceY + 185.0 * Section, 119.0), FVector(15.0, 15.0, 252.0));
    }
    Box(Steel, FVector(-1240.0, -476.0, 243.0), FVector(10.0, 753.0, 9.0));
    Box(Steel, FVector(-690.0, FenceY - 1.0, 243.0), FVector(360.0, 7.0, 70.0));
    Box(Amber, FVector(-690.0, FenceY + 4.0, 278.0), FVector(360.0, 2.0, 4.0));
    Box(Teal, FVector(-690.0, FenceY + 4.0, 208.0), FVector(360.0, 2.0, 4.0));

    // Two towers suggest real floodlights while sharing the existing directional lighting.
    for (double X : {-1120.0, 1120.0})
    {
        Box(Steel, FVector(X, -828.0, 245.0), FVector(21.0, 21.0, 500.0));
        Box(Steel, FVector(X, -828.0, 501.0), FVector(118.0, 34.0, 27.0));
        for (int32 Lamp = -1; Lamp <= 1; ++Lamp)
        {
            Box(Lines, FVector(X + 36.0 * Lamp, -809.5, 501.0), FVector(28.0, 3.0, 19.0));
        }
    }

    // Broad bands and three layers of silhouette hold together at either aspect ratio.
    Box(SkyGold, FVector(0.0, -7220.0, -4220.0), FVector(17000.0, 30.0, 2200.0));
    Box(SkyRose, FVector(0.0, -7220.0, -2520.0), FVector(17000.0, 30.0, 1200.0));
    Box(SkyViolet, FVector(0.0, -7220.0, -1520.0), FVector(17000.0, 30.0, 800.0));
    Box(SkyPurple, FVector(0.0, -7220.0, 1840.0), FVector(17000.0, 30.0, 5920.0));

    FRandomStream Layout(703);
    for (int32 Index = -11; Index <= 11; ++Index)
    {
        const double X = 485.0 * Index;
        const double Roof = Layout.FRandRange(-1820.0f, -1140.0f) + (Index % 4 == 0 ? 340.0 : 0.0);
        Box(FarBuildings, FVector(X, -5450.0, (Roof - 4000.0) * 0.5),
            FVector(Layout.FRandRange(360.0f, 490.0f), 390.0, Roof + 4000.0));
        if (Index % 3 == 0)
        {
            Box(FarBuildings, FVector(X - 74.0, -5450.0, Roof + 107.0), FVector(9.0, 9.0, 214.0));
        }
    }
    for (int32 Index = -8; Index <= 8; ++Index)
    {
        const double X = 395.0 * Index + 90.0;
        const double Roof = Layout.FRandRange(-1050.0f, -510.0f) + (Index % 4 == 0 ? 150.0 : 0.0);
        const double Width = Layout.FRandRange(280.0f, 365.0f);
        Box(NearBuildings, FVector(X, -3420.0, (Roof - 2850.0) * 0.5), FVector(Width, 310.0, Roof + 2850.0));
        Box(FarBuildings, FVector(X, -3420.0, Roof), FVector(Width + 24.0, 330.0, 18.0));
        if (Index % 2 == 0)
        {
            Box(NearBuildings, FVector(X + 60.0, -3420.0, Roof + 37.0), FVector(100.0, 95.0, 76.0));
        }
        for (int32 Row = 0; Row < 5; ++Row)
        {
            for (int32 Column = -1; Column <= 1; ++Column)
            {
                if (Layout.FRand() > 0.43f)
                {
                    Box(Windows, FVector(X + Column * 71.0, -3264.0, Roof - 75.0 - Row * 100.0),
                        FVector(30.0, 2.0, 43.0));
                }
            }
        }
    }
    for (int32 Index = -5; Index <= 5; ++Index)
    {
        const double X = Index * 510.0 - 80.0;
        const double Roof = Layout.FRandRange(-730.0f, -445.0f);
        Box(NearBuildings, FVector(X, -1960.0, (Roof - 1750.0) * 0.5), FVector(437.0, 318.0, Roof + 1750.0));
        Box(Concrete, FVector(X, -1960.0, Roof), FVector(460.0, 347.0, 20.0));
        if (Index % 2 != 0)
        {
            Box(Steel, FVector(X - 92.0, -1960.0, Roof + 47.0), FVector(84.0, 64.0, 96.0));
        }
        for (int32 Column = -2; Column <= 2; ++Column)
        {
            if (Layout.FRand() > 0.28f)
            {
                Box(Windows, FVector(X + Column * 70.0, -1800.0, Roof - 91.0), FVector(37.0, 2.0, 45.0));
            }
        }
    }
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
