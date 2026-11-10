#include "Prototype/CrossoverMatchHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
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
const FLinearColor Gold(1.0f, 0.74f, 0.28f);
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

void ACrossoverMatchHUD::BeginPlay()
{
    Super::BeginPlay();

    const FString FontPath = FPaths::ProjectContentDir() / TEXT("ThirdParty/Barlow/BarlowCondensed-Bold.ttf");
    if (IFileManager::Get().FileSize(*FontPath) <= 0)
    {
        return;
    }

    DisplayFont = NewObject<UFont>(this);
    DisplayFont->FontCacheType = EFontCacheType::Runtime;
    DisplayFont->LegacyFontName = TEXT("Bold");
    DisplayFont->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.Emplace(
        DisplayFont->LegacyFontName, FontPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
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
    const auto Text = [&](const FString& Label, FLinearColor Color, float X, float Y, float Size,
        bool bUseDisplayFont = false)
    {
        UFont* LabelFont = bUseDisplayFont && DisplayFont ? DisplayFont.Get() : Font;
        float Width = 0.0f;
        float Height = 0.0f;
        GetTextSize(Label, Width, Height, LabelFont, Size * Scale);
        DrawText(Label, Color, X * Scale - Width * 0.5f, Y * Scale, LabelFont, Size * Scale);
    };

    if (Mode->IsAtTitle() || Mode->IsMatchPaused())
    {
        const bool bTitle = Mode->IsAtTitle();
        const float Top = 82.0f;
        Rect(FLinearColor(0.01f, 0.018f, 0.04f, 0.93f), Center - 297.0f, Top, 594.0f, 235.0f);
        Rect(HomeColor, Center - 297.0f, Top, 594.0f, 3.0f);
        Rect(Gold, Center - 297.0f, Top + 232.0f, 594.0f, 3.0f);
        Text(bTitle ? TEXT("CROSSOVER") : TEXT("TIME OUT"), FLinearColor::White, Center, Top + 27.0f, 3.7f, true);
        Text(TEXT("NIGHT SHIFT  /  ROOFTOP STREETBALL"), HomeColor, Center, Top + 78.0f, 1.65f);
        if (bTitle)
        {
            Text(FString::Printf(TEXT("3 ON 3   /   FIRST TO 11   /   %s"), Mode->GetDifficultyName()),
                Gold, Center, Top + 113.0f, 1.45f);
            Text(TEXT("SPACE / BOTTOM FACE   START"), FLinearColor::White, Center, Top + 154.0f, 1.65f);
            Text(TEXT("E / LEFT FACE   CHANGE DIFFICULTY"), Quiet, Center, Top + 190.0f, 1.35f);
        }
        else
        {
            Text(TEXT("R / MENU   RESUME"), FLinearColor::White, Center, Top + 154.0f, 1.65f);
        }
        return;
    }

    constexpr float ScoreWidth = 536.0f;
    const float Left = Center - ScoreWidth * 0.5f;
    Rect(Panel, Left, 18.0f, ScoreWidth, 122.0f);
    Rect(HomeColor, Left, 18.0f, 4.0f, 80.0f);
    Rect(AwayColor, Left + ScoreWidth - 4.0f, 18.0f, 4.0f, 80.0f);
    Text(TEXT("HOME"), HomeColor, Center - 175.0f, 29.0f, 1.3f);
    Text(TEXT("AWAY"), AwayColor, Center + 175.0f, 29.0f, 1.3f);
    Text(FString::Printf(TEXT("%lld"), static_cast<long long>(State.HomeScore)),
        FLinearColor::White, Center - 175.0f, 52.0f, 2.5f, true);
    Text(FString::Printf(TEXT("%lld"), static_cast<long long>(State.AwayScore)),
        FLinearColor::White, Center + 175.0f, 52.0f, 2.5f, true);

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

    if (Mode->GetScoreFlash() > 0.0f)
    {
        const FLinearColor Pop = Mode->IsHomeScoreFlash() ? HomeColor : AwayColor;
        const FString Caption = FString::Printf(TEXT("%s   +%d"),
            Mode->WasLastScoreDunk() ? TEXT("SLAM") : TEXT("BUCKET"), Mode->GetLastScorePoints());
        Rect(Panel, Center - 145.0f, 148.0f, 290.0f, 39.0f);
        Text(Caption, Pop, Center, 154.0f, 2.0f);
    }
    else if (!Mode->GetFeedback().IsEmpty())
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

    const float Boost = FMath::Clamp(Mode->GetBoostFill(), 0.0f, 1.0f);
    const bool bBoostReady = !Mode->IsBoostActive() && Boost >= 1.0f;
    Rect(Panel, Center - 337.0f, Bottom - 111.0f, 158.0f, 38.0f);
    Text(Mode->IsBoostActive() ? TEXT("OVERTAKE ACTIVE") : bBoostReady ? TEXT("Q / RB   BOOST READY") : TEXT("OVERTAKE   Q / RB"),
        bBoostReady || Mode->IsBoostActive() ? Gold : Quiet, Center - 258.0f, Bottom - 106.0f, 1.05f);
    Rect(FLinearColor(0.15f, 0.16f, 0.20f), Center - 328.0f, Bottom - 85.0f, 140.0f, 5.0f);
    if (Boost > 0.0f)
    {
        Rect(Gold, Center - 328.0f, Bottom - 85.0f, 140.0f * Boost, 5.0f);
    }

    Text(TEXT("WASD Move    SHIFT Sprint    SPACE Shoot    E Pass / Switch    F Contest / Steal    R Pause"),
        FLinearColor::White, Center, Bottom - 58.0f, 1.15f);
    Text(TEXT("Left stick   /   LB   /   Bottom face   /   Left face   /   Right face"),
        Quiet, Center, Bottom - 35.0f, 1.12f);
    if (Mode->IsChargingShot())
    {
        constexpr float MeterWidth = 236.0f;
        const float MeterLeft = Center - MeterWidth * 0.5f;
        const float MeterTop = Bottom - 108.0f;
        const float Inset = Mode->GetChargeWindowInset();
        const float Start = ACrossoverPracticeMode::MakeWindowStart + Inset;
        const float End = ACrossoverPracticeMode::MakeWindowEnd - Inset;
        const float Charge = FMath::Clamp(Mode->GetChargeFraction(), 0.0f, 1.0f);
        Rect(Panel, MeterLeft - 6.0f, MeterTop - 6.0f, MeterWidth + 12.0f, 28.0f);
        Rect(Quiet * 0.33f, MeterLeft, MeterTop, MeterWidth, 16.0f);
        Rect(HomeColor, MeterLeft + MeterWidth * Start, MeterTop, MeterWidth * (End - Start), 16.0f);
        Rect(FLinearColor::White, MeterLeft + (MeterWidth - 3.0f) * Charge, MeterTop - 3.0f, 3.0f, 22.0f);
    }
}
