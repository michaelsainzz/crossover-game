#include "Prototype/CrossoverArcadeAudio.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RandomStream.h"
#include "Sound/SoundWaveProcedural.h"

namespace
{
constexpr int32 SampleRate = 24000;
constexpr float MusicVolume = 0.20f;
constexpr float Tau = 2.0f * UE_PI;

struct FCueShape
{
    float Duration;
    float Volume;
};

constexpr FCueShape CueShapes[] =
{
    {0.20f, 0.64f}, // Bounce
    {0.21f, 0.56f}, // Pass
    {0.20f, 0.54f}, // Release
    {0.52f, 0.85f}, // Basket
    {0.72f, 0.91f}, // Dunk
    {0.29f, 0.68f}, // Rim
    {0.35f, 0.77f}, // Steal
    {0.27f, 0.55f}, // Clear
    {0.48f, 0.83f}, // Buzzer
    {1.13f, 0.78f}, // Win
};
static_assert(UE_ARRAY_COUNT(CueShapes) == static_cast<uint8>(ECrossoverArcadeCue::Count));

float Osc(float Cycles)
{
    return FMath::Sin(Tau * Cycles);
}

float Punch(float Time, float Decay)
{
    return FMath::Min(Time * 280.0f, 1.0f) * FMath::Exp(-Time * Decay);
}

float Chime(float Time, float Frequency, float Decay)
{
    if (Time < 0.0f)
    {
        return 0.0f;
    }
    const float Phase = Frequency * Time;
    return Punch(Time, Decay) * (Osc(Phase) + 0.22f * Osc(2.0f * Phase) + 0.09f * Osc(3.0f * Phase));
}

float CueSample(ECrossoverArcadeCue Cue, float Time, float Noise)
{
    switch (Cue)
    {
    case ECrossoverArcadeCue::Bounce:
        return Punch(Time, 27.0f) * (0.78f * Osc(Time * 99.0f + 2.5f * (1.0f - FMath::Exp(-Time * 51.0f)))
            + 0.15f * Osc(Time * 242.0f) + 0.11f * Noise);
    case ECrossoverArcadeCue::Pass:
        return Punch(Time, 17.0f) * (0.23f * Noise + 0.37f * Osc(Time * (480.0f - 1300.0f * Time)));
    case ECrossoverArcadeCue::Release:
        return Punch(Time, 20.0f) * (0.25f * Noise + 0.36f * Osc(Time * (310.0f + 1750.0f * Time)));
    case ECrossoverArcadeCue::Basket:
        return 0.30f * Noise * Punch(Time, 18.0f) + 0.25f * Chime(Time, 523.25f, 9.0f)
            + 0.32f * Chime(Time - 0.075f, 783.99f, 9.0f) + 0.28f * Chime(Time - 0.15f, 1046.50f, 10.0f);
    case ECrossoverArcadeCue::Dunk:
        return Punch(Time, 11.0f) * (0.68f * Osc(Time * 64.0f + 2.8f * (1.0f - FMath::Exp(-Time * 40.0f)))
            + 0.33f * Noise) + 0.27f * Chime(Time - 0.090f, 293.66f, 6.0f)
            + 0.33f * Chime(Time - 0.150f, 587.33f, 7.0f);
    case ECrossoverArcadeCue::Rim:
        return Punch(Time, 15.0f) * (0.27f * Osc(Time * 397.0f) + 0.22f * Osc(Time * 637.0f)
            + 0.13f * Osc(Time * 982.0f) + 0.22f * Noise * FMath::Exp(-Time * 28.0f));
    case ECrossoverArcadeCue::Steal:
        return 0.38f * Chime(Time, 392.0f, 11.0f) + 0.42f * Chime(Time - 0.066f, 659.25f, 13.0f)
            + 0.17f * Noise * Punch(Time, 35.0f);
    case ECrossoverArcadeCue::Clear:
        return 0.28f * Chime(Time, 349.23f, 15.0f) + 0.32f * Chime(Time - 0.062f, 523.25f, 13.0f);
    case ECrossoverArcadeCue::Buzzer:
    {
        const float Gate = FMath::Min(Time * 230.0f, 1.0f) * FMath::Clamp((0.44f - Time) * 23.0f, 0.0f, 1.0f);
        const float Phase = Time * 142.0f;
        return Gate * (0.38f * Osc(Phase) + 0.25f * Osc(2.0f * Phase) + 0.12f * Osc(3.0f * Phase)
            + 0.09f * Osc(Time * 149.0f));
    }
    case ECrossoverArcadeCue::Win:
        return 0.34f * Chime(Time, 392.0f, 5.0f) + 0.31f * Chime(Time - 0.11f, 493.88f, 5.0f)
            + 0.27f * Chime(Time - 0.22f, 587.33f, 5.0f) + 0.34f * Chime(Time - 0.34f, 783.99f, 4.5f);
    case ECrossoverArcadeCue::Count:
        return 0.0f;
    }
    return 0.0f;
}

int16 ToPCM(float Sample)
{
    return static_cast<int16>(FMath::RoundToInt(FMath::Clamp(Sample, -1.0f, 1.0f) * 32767.0f));
}

TArray<int16> RenderCue(ECrossoverArcadeCue Cue, float Duration)
{
    TArray<int16> Samples;
    Samples.SetNumUninitialized(FMath::CeilToInt(SampleRate * Duration));
    FRandomStream Random(2273 + static_cast<uint8>(Cue) * 743);
    for (int32 Index = 0; Index < Samples.Num(); ++Index)
    {
        const float Time = static_cast<float>(Index) / SampleRate;
        const float Tail = FMath::Clamp((Duration - Time) * 95.0f, 0.0f, 1.0f);
        Samples[Index] = ToPCM(Tail * CueSample(Cue, Time, Random.FRandRange(-1.0f, 1.0f)));
    }
    return Samples;
}

TArray<int16> RenderMusic()
{
    constexpr int32 Steps = 32;
    constexpr int32 SamplesPerStep = 3400;
    constexpr int32 KickSteps[] = {0, 7, 8, 14, 16, 22, 24, 30};
    constexpr float Bass[] = {55.00f, 55.00f, 65.41f, 55.00f, 58.27f, 58.27f, 49.00f, 65.41f};
    constexpr float Lead[] = {220.00f, 261.63f, 329.63f, 0.0f, 233.08f, 349.23f, 293.66f, 261.63f};
    TArray<int16> Samples;
    Samples.SetNumUninitialized(Steps * SamplesPerStep);
    FRandomStream Random(1501);
    float PreviousNoise = 0.0f;
    for (int32 Index = 0; Index < Samples.Num(); ++Index)
    {
        const int32 Step = Index / SamplesPerStep;
        const float Time = static_cast<float>(Index % SamplesPerStep) / SampleRate;
        const float Pulse = FMath::Clamp(Time * 150.0f, 0.0f, 1.0f)
            * FMath::Clamp((static_cast<float>(SamplesPerStep) / SampleRate - Time) * 100.0f, 0.0f, 1.0f);
        const float Noise = Random.FRandRange(-1.0f, 1.0f);
        const float Hat = (Noise - PreviousNoise) * (Step % 2 ? 0.07f : 0.10f) * FMath::Exp(-Time * 88.0f);
        PreviousNoise = Noise;
        float Kick = 0.0f;
        for (const int32 KickStep : KickSteps)
        {
            if (Step == KickStep)
            {
                Kick = 0.50f * FMath::Exp(-Time * 25.0f)
                    * Osc(Time * 47.0f + 2.2f * (1.0f - FMath::Exp(-Time * 37.0f)));
                break;
            }
        }
        const float Clap = Step % 8 == 4 ? 0.29f * Noise * FMath::Exp(-Time * 47.0f) : 0.0f;
        const float BassPhase = Time * Bass[Step / 4];
        const float BassVoice = (0.22f * Osc(BassPhase) + 0.09f * Osc(2.0f * BassPhase))
            * FMath::Exp(-Time * (Step % 2 ? 22.0f : 8.0f));
        const float Melody = Step % 4 == 2 ? 0.13f * Chime(Time, Lead[Step / 4], 12.0f) : 0.0f;
        Samples[Index] = ToPCM(Pulse * (Hat + Kick + Clap + BassVoice + Melody));
    }
    return Samples;
}

USoundWaveProcedural* MakeWave(UObject* Outer, float Duration, bool bLooping)
{
    USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(Outer);
    Wave->SetSampleRate(SampleRate);
    Wave->NumChannels = 1;
    Wave->Duration = Duration;
    Wave->bLooping = bLooping;
    return Wave;
}
}

UCrossoverArcadeAudio::UCrossoverArcadeAudio()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCrossoverArcadeAudio::BeginPlay()
{
    Super::BeginPlay();
    UWorld* World = GetWorld();
    bEnabled = World && World->AllowAudioPlayback() && World->GetAudioDeviceRaw();
    if (!bEnabled)
    {
        return;
    }

    const auto Samples = MakeShared<const TArray<int16>, ESPMode::ThreadSafe>(RenderMusic());
    USoundWaveProcedural* Wave = MakeWave(this, INDEFINITELY_LOOPING_DURATION, true);
    Wave->VirtualizationMode = EVirtualizationMode::PlayWhenSilent;
    Wave->OnSoundWaveProceduralUnderflow.BindLambda(
        [Samples, Cursor = 0](USoundWaveProcedural* Procedural, int32 Requested) mutable
        {
            while (Requested > 0)
            {
                const int32 Count = FMath::Min(Requested, Samples->Num() - Cursor);
                Procedural->QueueAudio(reinterpret_cast<const uint8*>(Samples->GetData() + Cursor), Count * sizeof(int16));
                Requested -= Count;
                Cursor = (Cursor + Count) % Samples->Num();
            }
        });
    MusicComponent = UGameplayStatics::SpawnSound2D(this, Wave, bMuted ? 0.0f : MusicVolume);
}

void UCrossoverArcadeAudio::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEnabled = false;
    if (MusicComponent)
    {
        MusicComponent->Stop();
        MusicComponent = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}

void UCrossoverArcadeAudio::Play(ECrossoverArcadeCue Cue)
{
    const uint8 Index = static_cast<uint8>(Cue);
    if (!bEnabled || bMuted || Index >= UE_ARRAY_COUNT(CueShapes))
    {
        return;
    }

    const FCueShape& Shape = CueShapes[Index];
    TArray<int16>& Samples = CueSamples[Index];
    if (Samples.IsEmpty())
    {
        Samples = RenderCue(Cue, Shape.Duration);
    }
    USoundWaveProcedural* Wave = MakeWave(this, Shape.Duration, false);
    Wave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
    if (UAudioComponent* Voice = UGameplayStatics::SpawnSound2D(this, Wave, Shape.Volume))
    {
        // A drained procedural wave outputs silence until stopped.
        Voice->StopDelayed(Shape.Duration + 0.10f);
    }
}

void UCrossoverArcadeAudio::SetMuted(bool bNewMuted)
{
    bMuted = bNewMuted;
    if (MusicComponent)
    {
        MusicComponent->SetVolumeMultiplier(bMuted ? 0.0f : MusicVolume);
    }
}
