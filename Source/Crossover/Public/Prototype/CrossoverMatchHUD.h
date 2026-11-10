#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CrossoverMatchHUD.generated.h"

class UFont;

UCLASS()
class CROSSOVER_API ACrossoverMatchHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UFont> DisplayFont;
};
