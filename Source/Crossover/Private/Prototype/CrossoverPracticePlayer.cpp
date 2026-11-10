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
#include "Prototype/CrossoverPracticeMode.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr float WalkSpeed = 540.0f;
constexpr float SprintSpeed = 760.0f;
}

ACrossoverPracticePlayer::ACrossoverPracticePlayer()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(38.0f, 88.0f);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    bUseControllerRotationYaw = false;

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlockoutBody"));
    Body->SetupAttachment(GetCapsuleComponent());
    Body->SetRelativeScale3D(FVector(0.58, 0.58, 1.6));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Shape.Succeeded())
    {
        Body->SetStaticMesh(Shape.Object);
    }
}

void ACrossoverPracticePlayer::PawnClientRestart()
{
    Super::PawnClientRestart();
    if (UMaterialInstanceDynamic* Material = Body->CreateDynamicMaterialInstance(0))
    {
        Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.01f, 0.55f, 0.8f));
    }

    const APlayerController* ControllerOwner = Cast<APlayerController>(GetController());
    if (ControllerOwner && PracticeInput)
    {
        UEnhancedInputLocalPlayerSubsystem* Input =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(ControllerOwner->GetLocalPlayer());
        if (Input)
        {
            Input->RemoveMappingContext(PracticeInput);
            Input->AddMappingContext(PracticeInput, 0);
        }
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
    }

    Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACrossoverPracticePlayer::Move);
    Input->BindAction(SprintAction, ETriggerEvent::Started, this, &ACrossoverPracticePlayer::StartSprint);
    Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &ACrossoverPracticePlayer::StopSprint);
    Input->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ACrossoverPracticePlayer::StopSprint);
    Input->BindAction(ShootAction, ETriggerEvent::Started, this, &ACrossoverPracticePlayer::StartShot);
    Input->BindAction(ShootAction, ETriggerEvent::Completed, this, &ACrossoverPracticePlayer::ReleaseShot);
    Input->BindAction(ShootAction, ETriggerEvent::Canceled, this, &ACrossoverPracticePlayer::CancelShot);
}

void ACrossoverPracticePlayer::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const FVector Position = GetActorLocation();
    const FVector Clamped(FMath::Clamp(Position.X, -1050.0, 1050.0),
        FMath::Clamp(Position.Y, -700.0, 700.0), Position.Z);
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
    if (ACrossoverPracticeMode* Mode = GetWorld()->GetAuthGameMode<ACrossoverPracticeMode>())
    {
        Mode->StartShot(*this);
    }
}

void ACrossoverPracticePlayer::ReleaseShot()
{
    if (ACrossoverPracticeMode* Mode = GetWorld()->GetAuthGameMode<ACrossoverPracticeMode>())
    {
        Mode->ReleaseShot(*this);
    }
}

void ACrossoverPracticePlayer::CancelShot()
{
    if (ACrossoverPracticeMode* Mode = GetWorld()->GetAuthGameMode<ACrossoverPracticeMode>())
    {
        Mode->CancelShot(*this);
    }
}
