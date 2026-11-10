#pragma once

#include "Components/ActorComponent.h"
#include "CrossoverArcadeAudio.generated.h"

class UAudioComponent;

enum class ECrossoverArcadeCue : uint8
{
    Bounce,
    Pass,
    Release,
    Basket,
    Dunk,
    Rim,
    Steal,
    Clear,
    Buzzer,
    Win,
    Count
};

UCLASS()
class CROSSOVER_API UCrossoverArcadeAudio : public UActorComponent
{
    GENERATED_BODY()

public:
    UCrossoverArcadeAudio();

    void Play(ECrossoverArcadeCue Cue);
    void SetMuted(bool bNewMuted);
    bool IsMuted() const { return bMuted; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> MusicComponent;

    TArray<int16> CueSamples[static_cast<uint8>(ECrossoverArcadeCue::Count)];
    bool bEnabled = false;
    bool bMuted = false;
};
