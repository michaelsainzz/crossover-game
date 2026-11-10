#include "Prototype/CrossoverMatchScenario.h"

#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Prototype/CrossoverMatchMode.h"
#include "Prototype/CrossoverPracticeCourt.h"
#include "Prototype/CrossoverPracticePlayer.h"

namespace
{
constexpr int32 PlayersPerTeam = 3;
constexpr int32 PlayerCount = 2 * PlayersPerTeam;
constexpr float Frame = 1.0f / 60.0f;

bool Require(bool bCondition, const TCHAR* Description)
{
    if (!bCondition)
    {
        UE_LOG(LogTemp, Error, TEXT("CrossoverMatchSmoke: %s"), Description);
    }
    return bCondition;
}
}

bool Crossover::MatchScenario::Run(ACrossoverMatchMode& Mode)
{
    using Rules::BallPhase;
    using Rules::MatchPhase;
    using Rules::Team;

    APlayerController* Controller = Mode.GetWorld()->GetFirstPlayerController();
    if (!Require(Controller && Mode.Court && Mode.CourtCamera, TEXT("missing controller, court, or camera")))
    {
        return false;
    }

    auto IsHome = [](int32 Index) { return Index >= 0 && Index < PlayersPerTeam; };
    auto VerifyRoster = [&]()
    {
        if (!Require(Mode.Roster.Num() == PlayerCount, TEXT("expected three home and three away pawns")))
        {
            return false;
        }
        TSet<ACrossoverPracticePlayer*> UniquePlayers;
        for (int32 Index = 0; Index < PlayerCount; ++Index)
        {
            ACrossoverPracticePlayer* Player = Mode.Roster[Index].Get();
            if (!Require(IsValid(Player) && !UniquePlayers.Contains(Player), TEXT("invalid or duplicate roster pawn")))
            {
                return false;
            }
            UniquePlayers.Add(Player);
        }
        int32 WorldPlayerCount = 0;
        for (TActorIterator<ACrossoverPracticePlayer> It(Mode.GetWorld()); It; ++It)
        {
            ++WorldPlayerCount;
            if (!Require(UniquePlayers.Contains(*It), TEXT("unexpected extra pawn in the world")))
            {
                return false;
            }
        }
        return Require(WorldPlayerCount == PlayerCount && IsHome(Mode.HumanIndex)
                && Controller->GetPawn() == Mode.Roster[Mode.HumanIndex].Get()
                && Controller->GetViewTarget() == Mode.CourtCamera.Get(),
            TEXT("wrong world roster, controlled pawn, or camera"));
    };

    if (!VerifyRoster())
    {
        return false;
    }

    const FVector Rim = Mode.Court->GetRimLocation();
    auto Place = [&](int32 Index, double OffsetX, double OffsetY)
    {
        ACrossoverPracticePlayer& Player = *Mode.Roster[Index];
        const FVector Location(Rim.X + OffsetX, Rim.Y + OffsetY, Player.GetActorLocation().Z);
        Player.SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
    };
    auto Advance = [&](int32 Frames)
    {
        for (int32 Index = 0; Index < Frames; ++Index)
        {
            Mode.TickMatch(Frame, false);
        }
    };
    auto LivePossession = [&](Team TeamInControl)
    {
        const auto& State = Mode.GetMatchState();
        return State.Ball == BallPhase::Live && State.Possession == TeamInControl;
    };
    auto SpreadPlayers = [&]()
    {
        Place(0, -700.0, 0.0);
        Place(1, -750.0, -320.0);
        Place(2, -750.0, 320.0);
        Place(3, -150.0, -360.0);
        Place(4, -150.0, 360.0);
        Place(5, -150.0, 600.0);
    };
    auto Shoot = [&](int32 HoldFrames)
    {
        ACrossoverPracticePlayer& Player = *Mode.Roster[Mode.HumanIndex];
        Mode.StartShot(Player);
        Advance(HoldFrames);
        Mode.ReleaseShot(Player);
        if (!Require(Mode.GetMatchState().Ball == BallPhase::ShotInFlight, TEXT("release did not start a shot")))
        {
            return false;
        }
        for (int32 Index = 0; Index < 120 && Mode.GetMatchState().Ball == BallPhase::ShotInFlight; ++Index)
        {
            Advance(1);
        }
        return Require(Mode.GetMatchState().Ball != BallPhase::ShotInFlight, TEXT("shot did not resolve"));
    };

    const TCHAR* StartingDifficulty = Mode.GetDifficultyName();
    Mode.bAtTitle = true;
    Mode.SetMenuMovement(true);
    Mode.PassOrSwitch(*Mode.Roster[0]);
    if (!Require(FCString::Strcmp(StartingDifficulty, Mode.GetDifficultyName()) != 0, TEXT("title did not change difficulty")))
    {
        return false;
    }
    Mode.ReleaseShot(*Mode.Roster[0]);
    if (!Require(!Mode.bAtTitle, TEXT("shoot release did not start the match")))
    {
        return false;
    }
    Mode.Rematch(*Mode.Roster[0]);
    const int32 BeforePause = Mode.GetMatchState().RegulationRemainingMilliseconds;
    Mode.Tick(2.0f);
    if (!Require(Mode.IsMatchPaused() && Mode.GetMatchState().RegulationRemainingMilliseconds == BeforePause,
            TEXT("pause did not preserve the game clock")))
    {
        return false;
    }
    Mode.Rematch(*Mode.Roster[0]);
    if (!Require(!Mode.IsMatchPaused(), TEXT("pause input did not resume")))
    {
        return false;
    }

    Advance(45);
    if (!Require(LivePossession(Team::Home) && Mode.CarrierIndex == 0 && Mode.HumanIndex == 0,
            TEXT("opening check-in did not give home possession")))
    {
        return false;
    }

    Mode.BoostMeter = 100;
    Mode.ActivateBoost(*Mode.Roster[0]);
    if (!Require(Mode.IsBoostActive() && Mode.BoostMeter == 0, TEXT("full boost did not activate")))
    {
        return false;
    }
    Advance(430);
    if (!Require(!Mode.IsBoostActive() && LivePossession(Team::Home), TEXT("boost did not expire during live play")))
    {
        return false;
    }
    Mode.ResetMatch();
    Advance(45);

    // A 0.55 release is inside the normal window and below the contested window.
    constexpr int32 EdgeReleaseFrames = 33;
    SpreadPlayers();
    if (!Shoot(EdgeReleaseFrames) || !Require(Mode.GetMatchState().HomeScore == 2,
            TEXT("uncontested edge release should score two")))
    {
        return false;
    }

    Mode.ResetMatch();
    Advance(45);
    SpreadPlayers();
    Place(3, -530.0, 0.0); // Within contest range and between the shooter and rim.
    if (!Shoot(EdgeReleaseFrames) || !Require(Mode.GetMatchState().HomeScore == 0
            && Mode.bLooseBall && LivePossession(Team::Home),
        TEXT("rim-side defensive pressure should make the same edge release miss")))
    {
        return false;
    }
    UE_LOG(LogTemp, Display, TEXT("CrossoverMatchSmoke: contested and uncontested edge releases passed"));

    Mode.ResetMatch();
    Advance(45);
    SpreadPlayers();
    Mode.PassOrSwitch(*Mode.Roster[0]);
    Advance(22);
    if (!Require(LivePossession(Team::Home) && IsHome(Mode.CarrierIndex) && Mode.CarrierIndex != 0
            && Mode.HumanIndex == Mode.CarrierIndex,
        TEXT("completed pass did not transfer possession and control to a teammate")) || !VerifyRoster())
    {
        return false;
    }

    if (!Shoot(36) || !Require(Mode.GetMatchState().HomeScore == 2
            && Mode.GetMatchState().Possession == Team::Away,
        TEXT("good outside release should score two and hand the check-in to away")))
    {
        return false;
    }

    Advance(45);
    if (!Require(LivePossession(Team::Away) && Mode.Roster.IsValidIndex(Mode.CarrierIndex)
            && !IsHome(Mode.CarrierIndex), TEXT("away check-in did not complete")))
    {
        return false;
    }

    const int32 AwayCarrier = Mode.CarrierIndex;
    const int32 PriorDefender = Mode.HumanIndex;
    SpreadPlayers();
    Place(AwayCarrier, -350.0, 0.0);
    Mode.PassOrSwitch(*Mode.Roster[PriorDefender]);
    if (!Require(IsHome(Mode.HumanIndex) && Mode.HumanIndex != PriorDefender,
        TEXT("defensive switch should select another home pawn")) || !VerifyRoster())
    {
        return false;
    }
    const int32 Stealer = Mode.HumanIndex;
    Place(Stealer, -350.0, 100.0);
    Mode.Contest(*Mode.Roster[Stealer]);
    if (!Require(LivePossession(Team::Home) && Mode.CarrierIndex == Stealer
            && Mode.GetMatchState().RequiresClear, TEXT("nearby steal should require a defensive clear")))
    {
        return false;
    }

    Mode.StartShot(*Mode.Roster[Stealer]);
    Mode.ReleaseShot(*Mode.Roster[Stealer]);
    if (!Require(!Mode.IsChargingShot() && LivePossession(Team::Home)
            && Mode.GetMatchState().RequiresClear, TEXT("uncleared possession accepted a shot")))
    {
        return false;
    }

    Place(Stealer, -750.0, 120.0);
    Advance(1);
    if (!Require(!Mode.GetMatchState().RequiresClear, TEXT("crossing the arc did not clear possession"))
        || !Shoot(6) || !Require(Mode.GetMatchState().HomeScore == 2 && Mode.bLooseBall,
            TEXT("miss should leave a loose ball and preserve the score")))
    {
        return false;
    }

    const int32 Rebounder = (Stealer + 1) % PlayersPerTeam;
    ACrossoverPracticePlayer& ReboundPlayer = *Mode.Roster[Rebounder];
    FVector ReboundPosition = Mode.LoosePosition;
    ReboundPosition.Z = ReboundPlayer.GetActorLocation().Z;
    ReboundPlayer.SetActorLocation(ReboundPosition, false, nullptr, ETeleportType::TeleportPhysics);
    Mode.PassOrSwitch(*Mode.Roster[Stealer]);
    if (!Require(Mode.HumanIndex == Rebounder && Mode.bLooseBall && LivePossession(Team::Home),
            TEXT("loose-ball switch should select the closest teammate without claiming possession"))
        || !VerifyRoster())
    {
        return false;
    }
    Advance(10);
    if (!Require(LivePossession(Team::Home) && !Mode.bLooseBall && Mode.CarrierIndex == Rebounder,
        TEXT("nearby teammate failed to recover the rebound")))
    {
        return false;
    }
    for (int32 Step = 0; Step < 125 && LivePossession(Team::Home); ++Step)
    {
        Mode.TickMatch(0.1f, false);
    }
    if (!Require(Mode.GetMatchState().Ball == BallPhase::AwaitingCheckIn
            && Mode.GetMatchState().Possession == Team::Away
            && Mode.GetMatchState().ShotClockRemainingMilliseconds == 12000,
        TEXT("shot clock violation did not hand a fresh check-in to away")))
    {
        return false;
    }
    UE_LOG(LogTemp, Display, TEXT("CrossoverMatchSmoke: six pawns, pass, shot, defensive switch, steal, clear, rebound, and turnover passed"));

    auto RunRegulation = [&]()
    {
        for (int32 Second = 0; Second < 320 && Mode.GetMatchState().Phase == MatchPhase::Regulation; ++Second)
        {
            Mode.TickMatch(1.0f, false);
        }
        return Require(Mode.GetMatchState().RegulationRemainingMilliseconds == 0,
            TEXT("regulation clock did not expire within the simulated limit"));
    };

    if (!RunRegulation() || !Require(Mode.GetMatchState().Phase == MatchPhase::Finished
            && Mode.GetMatchState().Winner == Team::Home, TEXT("regulation leader did not win")))
    {
        return false;
    }

    for (int32 Round = 0; Round < 5; ++Round)
    {
        Mode.Rematch(*Mode.Roster[Mode.HumanIndex]);
        const auto& State = Mode.GetMatchState();
        if (!Require(State.Phase == MatchPhase::Regulation && State.Possession == Team::Home
                && State.Ball == BallPhase::AwaitingCheckIn && State.Winner == Team::None
                && State.HomeScore == 0 && State.AwayScore == 0 && !State.RequiresClear
                && State.RegulationRemainingMilliseconds == 240000 && State.ShotClockRemainingMilliseconds == 12000
                && Mode.HumanIndex == 0 && !Mode.bLooseBall && !Mode.IsChargingShot(),
            TEXT("rematch carried state forward")) || !VerifyRoster())
        {
            return false;
        }

        if (Round == 4)
        {
            break;
        }

        if (!RunRegulation() || !Require(State.Phase == MatchPhase::Overtime,
            TEXT("scoreless regulation tie did not enter overtime")))
        {
            return false;
        }
        if (State.Possession == Team::Away)
        {
            if (!Require(Mode.Rules.OutOfBounds(Team::Away), TEXT("overtime turnover was rejected")))
            {
                return false;
            }
            Mode.SetUpCheckIn();
        }
        Advance(45);
        SpreadPlayers();
        Place(Mode.HumanIndex, -100.0, -45.0);
        if (!Require(LivePossession(Team::Home) && Mode.HumanIndex == Mode.CarrierIndex,
                TEXT("overtime check-in did not return the ball to the controlled player"))
            || !Shoot(1)
            || !Require(State.Phase == MatchPhase::Finished && State.Winner == Team::Home && State.HomeScore == 1,
                TEXT("overtime dunk did not score one and win the match")))
        {
            return false;
        }
    }

    UE_LOG(LogTemp, Display, TEXT("CrossoverMatchSmoke: regulation, overtime, and five rematches passed"));
    return true;
}
