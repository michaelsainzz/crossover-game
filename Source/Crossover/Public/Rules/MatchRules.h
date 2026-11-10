#pragma once

#include <cstdint>

namespace Crossover::Rules
{
enum class Team { None, Home, Away };
enum class MatchPhase { Regulation, Overtime, Finished };
enum class BallPhase { AwaitingCheckIn, Live, ShotInFlight, DeadBall };
enum class ShotRange { InsideArc, OutsideArc };

using ShotId = std::uint64_t;
inline constexpr ShotId InvalidShot = 0;

struct MatchConfig
{
    int TargetScore = 11;
    int RegulationMilliseconds = 240000;
    int ShotClockMilliseconds = 12000;
};

struct MatchState
{
    MatchPhase Phase = MatchPhase::Regulation;
    BallPhase Ball = BallPhase::AwaitingCheckIn;
    Team Possession = Team::Home;
    Team Winner = Team::None;
    bool RequiresClear = false;
    std::int64_t HomeScore = 0;
    std::int64_t AwayScore = 0;
    int RegulationRemainingMilliseconds = 0;
    int ShotClockRemainingMilliseconds = 0;
};

class MatchRules
{
public:
    explicit MatchRules(MatchConfig config = {});
    const MatchState& GetState() const { return State; }

    void Reset(Team startingTeam = Team::Home);
    bool CheckIn(Team team);
    bool Recover(Team team);
    bool ClearArc(Team team);
    bool OutOfBounds(Team lastTouch);

    // The caller determines range at release and resolves every accepted shot.
    // Stale or duplicate resolutions are ignored, including after a rematch.
    ShotId BeginShot(Team team, ShotRange range);
    bool ResolveShot(ShotId shot, bool made);

    // Advance the simulation clock in whole milliseconds. Check-ins pause play.
    // A shot released before expiry may resolve after either clock reaches zero.
    void AdvanceTime(int elapsedMilliseconds);

private:
    void Restart(Team team);
    void Finish(Team team);
    void ApplyClockExpirations();

    MatchConfig Config;
    MatchState State;
    ShotId LastIssuedShot = InvalidShot;
    ShotId ActiveShot = InvalidShot;
    ShotRange ActiveRange = ShotRange::InsideArc;
};
}
