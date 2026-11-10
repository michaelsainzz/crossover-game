#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CrossoverPracticeCourt.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMeshComponent;

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
    TObjectPtr<UStaticMeshComponent> Floor;

    UPROPERTY(VisibleAnywhere, Category = "Court")
    TObjectPtr<UStaticMeshComponent> Sun;

    UPROPERTY(VisibleAnywhere, Category = "Court")
    TArray<TObjectPtr<UInstancedStaticMeshComponent>> PaletteMeshes;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> ColorMaterial;
};
