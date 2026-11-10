#include "Rules/MatchRules.h"

#include <algorithm>

namespace Crossover::Rules
{
namespace
{
bool IsPlayingTeam(Team team)
{
    return team == Team::Home || team == Team::Away;
}

Team Opponent(Team team)
{
    return team == Team::Home ? Team::Away : Team::Home;
}
}

MatchRules::MatchRules(MatchConfig config) : Config(config)
{
    const MatchConfig defaults;
    if (Config.TargetScore <= 0)
        Config.TargetScore = defaults.TargetScore;
    if (Config.RegulationMilliseconds <= 0)
        Config.RegulationMilliseconds = defaults.RegulationMilliseconds;
    if (Config.ShotClockMilliseconds <= 0)
        Config.ShotClockMilliseconds = defaults.ShotClockMilliseconds;
    Reset();
}

void MatchRules::Reset(Team startingTeam)
{
    State = {};
    State.RegulationRemainingMilliseconds = Config.RegulationMilliseconds;
    Restart(IsPlayingTeam(startingTeam) ? startingTeam : Team::Home);
}

void MatchRules::Restart(Team team)
{
    State.Ball = BallPhase::AwaitingCheckIn;
    State.Possession = team;
    State.RequiresClear = false;
    State.ShotClockRemainingMilliseconds = Config.ShotClockMilliseconds;
    ActiveShot = InvalidShot;
}

void MatchRules::Finish(Team team)
{
    State.Phase = MatchPhase::Finished;
    State.Ball = BallPhase::DeadBall;
    State.Possession = Team::None;
    State.Winner = team;
    State.RequiresClear = false;
}

bool MatchRules::CheckIn(Team team)
{
    if (State.Ball != BallPhase::AwaitingCheckIn || team != State.Possession)
        return false;

    State.Ball = BallPhase::Live;
    return true;
}

bool MatchRules::Recover(Team team)
{
    if (State.Ball != BallPhase::Live || !IsPlayingTeam(team) || team == State.Possession)
        return false;

    State.Possession = team;
    State.RequiresClear = true;
    State.ShotClockRemainingMilliseconds = Config.ShotClockMilliseconds;
    return true;
}

bool MatchRules::ClearArc(Team team)
{
    if (State.Ball != BallPhase::Live || team != State.Possession || !State.RequiresClear)
        return false;

    State.RequiresClear = false;
    return true;
}

bool MatchRules::OutOfBounds(Team lastTouch)
{
    if ((State.Ball != BallPhase::Live && State.Ball != BallPhase::ShotInFlight)
        || !IsPlayingTeam(lastTouch))
    {
        return false;
    }

    if (State.Ball == BallPhase::ShotInFlight)
    {
        ResolveShot(ActiveShot, false);
        if (State.Ball != BallPhase::Live)
            return true;
    }
    Restart(Opponent(lastTouch));
    return true;
}

ShotId MatchRules::BeginShot(Team team, ShotRange range)
{
    if (State.Ball != BallPhase::Live || team != State.Possession || State.RequiresClear)
        return InvalidShot;

    State.Ball = BallPhase::ShotInFlight;
    ActiveRange = range;
    ActiveShot = ++LastIssuedShot;
    return ActiveShot;
}

bool MatchRules::ResolveShot(ShotId shot, bool made)
{
    if (State.Ball != BallPhase::ShotInFlight || shot != ActiveShot)
        return false;

    ActiveShot = InvalidShot;
    State.Ball = BallPhase::Live;
    if (!made)
    {
        ApplyClockExpirations();
        return true;
    }

    auto& score = State.Possession == Team::Home ? State.HomeScore : State.AwayScore;
    score += ActiveRange == ShotRange::OutsideArc ? 2 : 1;
    if (score >= Config.TargetScore || State.Phase == MatchPhase::Overtime)
    {
        Finish(State.Possession);
        return true;
    }

    // A made shot takes precedence over an expired shot clock.
    Restart(Opponent(State.Possession));
    ApplyClockExpirations();
    return true;
}

void MatchRules::AdvanceTime(int elapsedMilliseconds)
{
    if (elapsedMilliseconds <= 0
        || (State.Ball != BallPhase::Live && State.Ball != BallPhase::ShotInFlight))
    {
        return;
    }

    const bool shotInFlight = State.Ball == BallPhase::ShotInFlight;
    if (!shotInFlight)
        elapsedMilliseconds = std::min(elapsedMilliseconds, State.ShotClockRemainingMilliseconds);

    // A live tie crosses directly into overtime. In other cases the regulation
    // deadline either ends play or waits for the active shot to resolve.
    if (State.Phase == MatchPhase::Regulation
        && (shotInFlight || State.HomeScore != State.AwayScore))
    {
        elapsedMilliseconds = std::min(elapsedMilliseconds, State.RegulationRemainingMilliseconds);
    }

    State.RegulationRemainingMilliseconds =
        std::max(0, State.RegulationRemainingMilliseconds - elapsedMilliseconds);
    State.ShotClockRemainingMilliseconds =
        std::max(0, State.ShotClockRemainingMilliseconds - elapsedMilliseconds);

    if (!shotInFlight)
        ApplyClockExpirations();
}

void MatchRules::ApplyClockExpirations()
{
    if (State.Phase == MatchPhase::Regulation && State.RegulationRemainingMilliseconds == 0)
    {
        if (State.HomeScore != State.AwayScore)
        {
            Finish(State.HomeScore > State.AwayScore ? Team::Home : Team::Away);
            return;
        }
        State.Phase = MatchPhase::Overtime;
    }

    if (State.ShotClockRemainingMilliseconds == 0)
        Restart(Opponent(State.Possession));
}
}
