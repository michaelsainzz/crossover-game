#include "Prototype/CrossoverPracticePlayer.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Prototype/CrossoverMatchMode.h"
#include "Prototype/CrossoverPracticeMode.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr float WalkSpeed = 540.0f;
constexpr float SprintSpeed = 760.0f;
constexpr float BotSpeed = 440.0f;
constexpr double BotSlowdownDistance = 120.0;
const FName PlayerColorParameter(TEXT("Color"));
}

ACrossoverPracticePlayer::ACrossoverPracticePlayer()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessAI = EAutoPossessAI::Disabled;
    GetCapsuleComponent()->InitCapsuleSize(38.0f, 88.0f);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    bUseControllerRotationYaw = false;

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlockoutBody"));
    Body->SetupAttachment(GetCapsuleComponent());
    Body->SetRelativeScale3D(FVector(0.58, 0.58, 1.6));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ControlMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ControlMarker"));
    ControlMarker->SetupAttachment(GetCapsuleComponent());
    ControlMarker->SetRelativeLocation(FVector(0.0, 0.0, 109.0));
    ControlMarker->SetRelativeScale3D(FVector(0.36, 0.36, 0.1));
    ControlMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ControlMarker->SetCastShadow(false);
    ControlMarker->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Shape.Succeeded())
    {
        Body->SetStaticMesh(Shape.Object);
        ControlMarker->SetStaticMesh(Shape.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Tint(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    MatchMaterial = Tint.Object;
}

void ACrossoverPracticePlayer::PawnClientRestart()
{
    RemovePlayerMapping();
    Super::PawnClientRestart();

    const APlayerController* ControllerOwner = Cast<APlayerController>(GetController());
    if (ControllerOwner && PracticeInput)
    {
        UEnhancedInputLocalPlayerSubsystem* Input =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(ControllerOwner->GetLocalPlayer());
        if (Input)
        {
            // Boolean actions wait for release. A held pass or shot cannot fire again
            // on the new pawn, while the axis action can continue moving.
            Input->AddMappingContext(PracticeInput, 0);
            MappedInputSubsystem = Input;
        }
    }
}

void ACrossoverPracticePlayer::UnPossessed()
{
    RemovePlayerMapping();
    StopSprint();
    Super::UnPossessed();
}

void ACrossoverPracticePlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RemovePlayerMapping();
    Super::EndPlay(EndPlayReason);
}

void ACrossoverPracticePlayer::RemovePlayerMapping()
{
    if (UEnhancedInputLocalPlayerSubsystem* Input = MappedInputSubsystem.Get())
    {
        Input->RemoveMappingContext(PracticeInput);
        MappedInputSubsystem.Reset();
    }
}

void ACrossoverPracticePlayer::SetMatchAppearance(bool bHome, bool bHuman)
{
    GetCharacterMovement()->MaxWalkSpeed = bHuman ? WalkSpeed : BotSpeed;
    if (MatchMaterial)
    {
        if (!TeamMaterial)
        {
            TeamMaterial = Body->CreateDynamicMaterialInstance(0, MatchMaterial);
        }
        if (TeamMaterial)
        {
            TeamMaterial->SetVectorParameterValue(PlayerColorParameter,
                bHome ? FLinearColor(0.04f, 0.72f, 0.68f) : FLinearColor(1.0f, 0.28f, 0.05f));
        }
        if (bHuman && !Cast<UMaterialInstanceDynamic>(ControlMarker->GetMaterial(0)))
        {
            if (UMaterialInstanceDynamic* MarkerMaterial = ControlMarker->CreateDynamicMaterialInstance(0, MatchMaterial))
            {
                MarkerMaterial->SetVectorParameterValue(PlayerColorParameter, FLinearColor(1.0f, 0.8f, 0.04f));
            }
        }
    }
    ControlMarker->SetVisibility(bHuman);
}

void ACrossoverPracticePlayer::DriveBotTowards(const FVector& Target, float StopRadius)
{
    if (IsPlayerControlled())
    {
        return;
    }

    const FVector Offset = Target - GetActorLocation();
    const double Distance = Offset.Size2D();
    if (Distance > StopRadius)
    {
        const double Throttle = FMath::Clamp((Distance - StopRadius) / BotSlowdownDistance, 0.0, 1.0);
        AddMovementInput(Offset.GetSafeNormal2D(), static_cast<float>(Throttle), true);
    }
}

void ACrossoverPracticePlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!Input)
    {
        return;
    }

    if (!PracticeInput)
    {
        PracticeInput = NewObject<UInputMappingContext>(this);
        MoveAction = NewObject<UInputAction>(this);
        MoveAction->ValueType = EInputActionValueType::Axis2D;
        MoveAction->AccumulationBehavior = EInputActionAccumulationBehavior::Cumulative;
        SprintAction = NewObject<UInputAction>(this);
        ShootAction = NewObject<UInputAction>(this);
        PassAction = NewObject<UInputAction>(this);
        ContestAction = NewObject<UInputAction>(this);
        RematchAction = NewObject<UInputAction>(this);

        auto BindMoveKey = [this](FKey Key, bool bVertical, bool bNegative)
        {
            FEnhancedActionKeyMapping& Mapping = PracticeInput->MapKey(MoveAction, Key);
            if (bVertical)
            {
                UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(PracticeInput);
                Swizzle->Order = EInputAxisSwizzle::YXZ;
                Mapping.Modifiers.Add(Swizzle);
            }
            if (bNegative)
            {
                Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(PracticeInput));
            }
        };

        BindMoveKey(EKeys::W, true, false);
        BindMoveKey(EKeys::S, true, true);
        BindMoveKey(EKeys::A, false, true);
        BindMoveKey(EKeys::D, false, false);
        FEnhancedActionKeyMapping& Stick = PracticeInput->MapKey(MoveAction, EKeys::Gamepad_Left2D);
        Stick.Modifiers.Add(NewObject<UInputModifierDeadZone>(PracticeInput));
        PracticeInput->MapKey(SprintAction, EKeys::LeftShift);
        PracticeInput->MapKey(SprintAction, EKeys::Gamepad_LeftShoulder);
        PracticeInput->MapKey(ShootAction, EKeys::SpaceBar);
        PracticeInput->MapKey(ShootAction, EKeys::Gamepad_FaceButton_Bottom);
        PracticeInput->MapKey(PassAction, EKeys::E);
        PracticeInput->MapKey(PassAction, EKeys::Gamepad_FaceButton_Left);
        PracticeInput->MapKey(ContestAction, EKeys::F);
        PracticeInput->MapKey(ContestAction, EKeys::Gamepad_FaceButton_Right);
        PracticeInput->MapKey(RematchAction, EKeys::R);
        PracticeInput->MapKey(RematchAction, EKeys::Gamepad_Special_Right);
    }

    Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACrossoverPracticePlayer::Move);
    Input->BindAction(SprintAction, ETriggerEvent::Started, this, &ACrossoverPracticePlayer::StartSprint);
    Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &ACrossoverPracticePlayer::StopSprint);
    Input->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ACrossoverPracticePlayer::StopSprint);
    Input->BindAction(ShootAction, ETriggerEvent::Started, this, &ACrossoverPracticePlayer::StartShot);
    Input->BindAction(ShootAction, ETriggerEvent::Completed, this, &ACrossoverPracticePlayer::ReleaseShot);
    Input->BindAction(ShootAction, ETriggerEvent::Canceled, this, &ACrossoverPracticePlayer::CancelShot);
    Input->BindAction(PassAction, ETriggerEvent::Started, this, &ACrossoverPracticePlayer::PassOrSwitch);
    Input->BindAction(ContestAction, ETriggerEvent::Started, this, &ACrossoverPracticePlayer::Contest);
    Input->BindAction(RematchAction, ETriggerEvent::Started, this, &ACrossoverPracticePlayer::Rematch);
}

void ACrossoverPracticePlayer::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const FVector Position = GetActorLocation();
    const bool bMatch = GetWorld()->GetAuthGameMode<ACrossoverMatchMode>() != nullptr;
    const double LimitX = bMatch ? 1095.0 : 1050.0;
    const double LimitY = bMatch ? 745.0 : 700.0;
    const FVector Clamped(FMath::Clamp(Position.X, -LimitX, LimitX),
        FMath::Clamp(Position.Y, -LimitY, LimitY), Position.Z);
    if (Position != Clamped)
    {
        SetActorLocation(Clamped);
    }
}

void ACrossoverPracticePlayer::Move(const FInputActionValue& Value)
{
    FVector2D Axis = Value.Get<FVector2D>();
    if (Axis.SizeSquared() > 1.0)
    {
        Axis.Normalize();
    }
    AddMovementInput(FVector::ForwardVector, Axis.X);
    AddMovementInput(-FVector::RightVector, Axis.Y);
}

void ACrossoverPracticePlayer::StartSprint()
{
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void ACrossoverPracticePlayer::StopSprint()
{
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ACrossoverPracticePlayer::StartShot()
{
    if (ACrossoverMatchMode* Match = GetWorld()->GetAuthGameMode<ACrossoverMatchMode>())
    {
        Match->StartShot(*this);
    }
    else if (ACrossoverPracticeMode* Mode = GetWorld()->GetAuthGameMode<ACrossoverPracticeMode>())
    {
        Mode->StartShot(*this);
    }
}

void ACrossoverPracticePlayer::ReleaseShot()
{
    if (ACrossoverMatchMode* Match = GetWorld()->GetAuthGameMode<ACrossoverMatchMode>())
    {
        Match->ReleaseShot(*this);
    }
    else if (ACrossoverPracticeMode* Mode = GetWorld()->GetAuthGameMode<ACrossoverPracticeMode>())
    {
        Mode->ReleaseShot(*this);
    }
}

void ACrossoverPracticePlayer::CancelShot()
{
    if (ACrossoverMatchMode* Match = GetWorld()->GetAuthGameMode<ACrossoverMatchMode>())
    {
        Match->CancelShot(*this);
    }
    else if (ACrossoverPracticeMode* Mode = GetWorld()->GetAuthGameMode<ACrossoverPracticeMode>())
    {
        Mode->CancelShot(*this);
    }
}

void ACrossoverPracticePlayer::PassOrSwitch()
{
    if (ACrossoverMatchMode* Match = GetWorld()->GetAuthGameMode<ACrossoverMatchMode>())
    {
        Match->PassOrSwitch(*this);
    }
}

void ACrossoverPracticePlayer::Contest()
{
    if (ACrossoverMatchMode* Match = GetWorld()->GetAuthGameMode<ACrossoverMatchMode>())
    {
        Match->Contest(*this);
    }
}

void ACrossoverPracticePlayer::Rematch()
{
    if (ACrossoverMatchMode* Match = GetWorld()->GetAuthGameMode<ACrossoverMatchMode>())
    {
        Match->Rematch(*this);
    }
}
