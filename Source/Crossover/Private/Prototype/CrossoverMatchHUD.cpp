#include "Prototype/CrossoverMatchHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Prototype/CrossoverMatchMode.h"
#include "Prototype/CrossoverPracticeMode.h"
#include "Rules/MatchRules.h"

namespace
{
using namespace Crossover::Rules;

const FLinearColor Panel(0.025f, 0.034f, 0.044f, 0.92f);
const FLinearColor Quiet(0.75f, 0.81f, 0.86f);
const FLinearColor HomeColor(0.20f, 0.90f, 0.85f);
const FLinearColor AwayColor(1.00f, 0.61f, 0.26f);
const FLinearColor WarningColor(1.00f, 0.43f, 0.32f);

int32 SecondsRemaining(int32 Milliseconds)
{
    return Milliseconds / 1000 + (Milliseconds % 1000 != 0);
}

FString DescribePossession(const MatchState& State)
{
    const TCHAR* Name = State.Possession == Team::Home ? TEXT("HOME") : TEXT("AWAY");
    if (State.Ball == BallPhase::AwaitingCheckIn)
    {
        return FString::Printf(TEXT("%s CHECK-IN"), Name);
    }
    if (State.RequiresClear)
    {
        return FString::Printf(TEXT("%s MUST CLEAR THE ARC"), Name);
    }
    return FString::Printf(TEXT("%s POSSESSION"), Name);
}
}

void ACrossoverMatchHUD::DrawHUD()
{
    Super::DrawHUD();
    const ACrossoverMatchMode* Mode = GetWorld()->GetAuthGameMode<ACrossoverMatchMode>();
    if (!Canvas || !Mode || !GEngine || Canvas->ClipX <= 0.0f || Canvas->ClipY <= 0.0f)
    {
        return;
    }

    const MatchState& State = Mode->GetMatchState();
    UFont* Font = GEngine->GetSmallFont();
    const float Scale = FMath::Min(Canvas->ClipX / 1280.0f, Canvas->ClipY / 720.0f);
    const float Center = Canvas->ClipX / (2.0f * Scale);
    const float Bottom = Canvas->ClipY / Scale;

    const auto Rect = [&](FLinearColor Color, float X, float Y, float Width, float Height)
    {
        DrawRect(Color, X * Scale, Y * Scale, Width * Scale, Height * Scale);
    };
    const auto Text = [&](const FString& Label, FLinearColor Color, float X, float Y, float Size)
    {
        float Width = 0.0f;
        float Height = 0.0f;
        GetTextSize(Label, Width, Height, Font, Size * Scale);
        DrawText(Label, Color, X * Scale - Width * 0.5f, Y * Scale, Font, Size * Scale);
    };

    constexpr float ScoreWidth = 536.0f;
    const float Left = Center - ScoreWidth * 0.5f;
    Rect(Panel, Left, 18.0f, ScoreWidth, 122.0f);
    Rect(HomeColor, Left, 18.0f, 4.0f, 80.0f);
    Rect(AwayColor, Left + ScoreWidth - 4.0f, 18.0f, 4.0f, 80.0f);
    Text(TEXT("HOME"), HomeColor, Center - 175.0f, 29.0f, 1.3f);
    Text(TEXT("AWAY"), AwayColor, Center + 175.0f, 29.0f, 1.3f);
    Text(FString::Printf(TEXT("%lld"), static_cast<long long>(State.HomeScore)),
        FLinearColor::White, Center - 175.0f, 52.0f, 2.5f);
    Text(FString::Printf(TEXT("%lld"), static_cast<long long>(State.AwayScore)),
        FLinearColor::White, Center + 175.0f, 52.0f, 2.5f);

    const int32 MatchSeconds = SecondsRemaining(State.RegulationRemainingMilliseconds);
    const FString Clock = FString::Printf(TEXT("%d:%02d"), MatchSeconds / 60, MatchSeconds % 60);
    const bool bFinished = State.Phase == MatchPhase::Finished;
    const bool bOvertime = State.Phase == MatchPhase::Overtime;
    const FString Period = bFinished ? TEXT("FINAL") : bOvertime ? TEXT("OVERTIME") : TEXT("REGULATION");
    Text(Period, Quiet, Center, 29.0f, 1.1f);
    Text(bOvertime ? FString(TEXT("NEXT BASKET WINS")) : Clock, FLinearColor::White,
        Center, 52.0f, bOvertime ? 1.45f : 2.1f);

    Rect(FLinearColor(0.10f, 0.13f, 0.17f, 0.95f), Left, 98.0f, ScoreWidth, 42.0f);
    if (bFinished)
    {
        const bool bHomeWon = State.Winner == Team::Home;
        Text(bHomeWon ? TEXT("HOME WINS") : TEXT("AWAY WINS"),
            bHomeWon ? HomeColor : AwayColor, Center, 109.0f, 1.45f);
    }
    else
    {
        const int32 ShotSeconds = SecondsRemaining(State.ShotClockRemainingMilliseconds);
        const FLinearColor PossessionColor = State.Possession == Team::Home ? HomeColor : AwayColor;
        Text(DescribePossession(State), PossessionColor, Center - 71.0f, 110.0f, 1.25f);
        Text(FString::Printf(TEXT("SHOT CLOCK   %02d"), ShotSeconds),
            ShotSeconds <= 4 ? WarningColor : FLinearColor::White, Center + 163.0f, 110.0f, 1.25f);
    }

    if (!Mode->GetFeedback().IsEmpty())
    {
        Rect(Panel, Center - 175.0f, 148.0f, 350.0f, 30.0f);
        Text(Mode->GetFeedback(), FLinearColor::White, Center, 155.0f, 1.22f);
    }

    Rect(Panel, Center - 337.0f, Bottom - 66.0f, 674.0f, 52.0f);
    if (bFinished)
    {
        Text(TEXT("PLAY AGAIN   R  /  Menu button"), FLinearColor::White, Center, Bottom - 54.0f, 1.5f);
        return;
    }

    Text(TEXT("WASD  Move     SHIFT  Sprint     SPACE  Shoot     E  Pass / Switch     F  Contest / Steal"),
        FLinearColor::White, Center, Bottom - 58.0f, 1.15f);
    Text(TEXT("Left stick   /   LB   /   Bottom face   /   Left face   /   Right face"),
        Quiet, Center, Bottom - 35.0f, 1.12f);
    if (Mode->IsChargingShot())
    {
        constexpr float MeterWidth = 236.0f;
        const float MeterLeft = Center - MeterWidth * 0.5f;
        const float MeterTop = Bottom - 108.0f;
        const float Start = ACrossoverPracticeMode::MakeWindowStart;
        const float End = ACrossoverPracticeMode::MakeWindowEnd;
        const float Charge = FMath::Clamp(Mode->GetChargeFraction(), 0.0f, 1.0f);
        Rect(Panel, MeterLeft - 6.0f, MeterTop - 6.0f, MeterWidth + 12.0f, 28.0f);
        Rect(Quiet * 0.33f, MeterLeft, MeterTop, MeterWidth, 16.0f);
        Rect(HomeColor, MeterLeft + MeterWidth * Start, MeterTop, MeterWidth * (End - Start), 16.0f);
        Rect(FLinearColor::White, MeterLeft + (MeterWidth - 3.0f) * Charge, MeterTop - 3.0f, 3.0f, 22.0f);
    }
}
