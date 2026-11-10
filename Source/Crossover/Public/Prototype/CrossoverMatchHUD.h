#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CrossoverMatchHUD.generated.h"

UCLASS()
class CROSSOVER_API ACrossoverMatchHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};
