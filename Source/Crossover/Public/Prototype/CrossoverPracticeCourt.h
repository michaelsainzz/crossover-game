#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CrossoverPracticeCourt.generated.h"

class UInstancedStaticMeshComponent;

UCLASS()
class CROSSOVER_API ACrossoverPracticeCourt : public AActor
{
    GENERATED_BODY()

public:
    ACrossoverPracticeCourt();

    virtual void OnConstruction(const FTransform& Transform) override;

    FVector GetRimLocation() const;
    bool IsOutsideArc(const FVector& WorldPosition) const;
    bool IsOutOfBounds(const FVector& WorldPosition) const;

private:
    UPROPERTY(VisibleAnywhere, Category = "Court")
    TObjectPtr<UInstancedStaticMeshComponent> Markings;

    UPROPERTY(VisibleAnywhere, Category = "Court")
    TObjectPtr<UInstancedStaticMeshComponent> Rim;
};
