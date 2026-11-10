#include "Rules/MatchRules.h"

#include <cstdlib>
#include <iostream>

using namespace Crossover::Rules;

namespace
{
void Expect(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void MakeBasket(MatchRules& rules, Team team, ShotRange range)
{
    const auto shot = rules.BeginShot(team, range);
    Expect(shot != InvalidShot, "valid shot is accepted");
    Expect(rules.ResolveShot(shot, true), "active shot scores");
}

void CheckInAndScoring()
{
    MatchRules rules;
    Expect(rules.GetState().RegulationRemainingMilliseconds == 240000, "regulation starts at four minutes");
    Expect(!rules.CheckIn(Team::Away), "wrong team cannot check in");
    Expect(rules.BeginShot(Team::Home, ShotRange::InsideArc) == InvalidShot, "restart is dead ball");
    rules.AdvanceTime(1000);
    Expect(rules.GetState().RegulationRemainingMilliseconds == 240000, "check-in pauses regulation");
    Expect(rules.CheckIn(Team::Home), "opening team can check in");

    const auto shot = rules.BeginShot(Team::Home, ShotRange::OutsideArc);
    Expect(shot != InvalidShot, "outside shot accepted");
    Expect(rules.BeginShot(Team::Home, ShotRange::InsideArc) == InvalidShot, "one active shot at a time");
    Expect(!rules.ResolveShot(shot + 1, true), "unmatched shot cannot score");
    Expect(rules.ResolveShot(shot, true), "outside shot resolves");
    Expect(!rules.ResolveShot(shot, true), "duplicate scoring callback rejected");
    Expect(rules.GetState().HomeScore == 2, "outside shot scores two points");
    Expect(rules.GetState().Possession == Team::Away, "basket changes possession");
    Expect(rules.GetState().Ball == BallPhase::AwaitingCheckIn, "basket requires check-in");

    Expect(rules.CheckIn(Team::Away), "opponent can check in after scoring");
    MakeBasket(rules, Team::Away, ShotRange::InsideArc);
    Expect(rules.GetState().AwayScore == 1, "inside shot scores one point");
}

void ClearArcAndBounds()
{
    MatchRules rules;
    Expect(rules.CheckIn(Team::Home), "home checks in");
    rules.AdvanceTime(2000);
    Expect(rules.Recover(Team::Away), "defense takes possession");
    Expect(rules.GetState().RequiresClear, "defense must clear the arc");
    Expect(rules.GetState().ShotClockRemainingMilliseconds == 12000, "recovery resets shot clock");
    Expect(!rules.Recover(Team::Away), "same-team recovery leaves state alone");
    Expect(rules.BeginShot(Team::Away, ShotRange::OutsideArc) == InvalidShot, "uncleared shot is rejected");
    Expect(!rules.ClearArc(Team::Home), "defense cannot clear for offense");
    Expect(rules.ClearArc(Team::Away), "offense clears arc");
    const auto shot = rules.BeginShot(Team::Away, ShotRange::OutsideArc);
    Expect(shot != InvalidShot, "clear unlocks a shot");
    Expect(rules.OutOfBounds(Team::Home), "deflection out of bounds restarts play");
    Expect(!rules.ResolveShot(shot, true), "out-of-bounds cancels the shot");
    Expect(rules.GetState().Possession == Team::Away, "last touch determines check-in team");
    Expect(!rules.GetState().RequiresClear, "check-in starts past the arc");
}

void ShotClockAndClockPartition()
{
    MatchRules rules({11, 240000, 12000});
    Expect(rules.CheckIn(Team::Home), "home checks in before clock starts");
    rules.AdvanceTime(11999);
    Expect(rules.GetState().Ball == BallPhase::Live, "ball live one millisecond before shot expiry");
    rules.AdvanceTime(100);
    Expect(rules.GetState().Ball == BallPhase::AwaitingCheckIn, "shot violation requires check-in");
    Expect(rules.GetState().Possession == Team::Away, "shot violation turns ball over");
    Expect(rules.GetState().RegulationRemainingMilliseconds == 228000, "dead ball time after violation is ignored");
    Expect(rules.GetState().ShotClockRemainingMilliseconds == 12000, "shot violation resets clock");
}

void PendingShotAtDeadline()
{
    MatchRules rules({11, 100, 25});
    Expect(rules.CheckIn(Team::Home), "home checks in for deadline shot");
    rules.AdvanceTime(24);
    const auto first = rules.BeginShot(Team::Home, ShotRange::InsideArc);
    rules.AdvanceTime(1);
    Expect(rules.GetState().Ball == BallPhase::ShotInFlight, "release before shot clock expiry stays live");
    Expect(rules.ResolveShot(first, false), "miss after expiry resolves");
    Expect(rules.GetState().Possession == Team::Away, "expired missed shot turns over");
    Expect(rules.GetState().Ball == BallPhase::AwaitingCheckIn, "expired miss requires check-in");

    Expect(rules.CheckIn(Team::Away), "away checks in after missed deadline shot");
    const auto second = rules.BeginShot(Team::Away, ShotRange::OutsideArc);
    rules.AdvanceTime(500);
    Expect(rules.GetState().RegulationRemainingMilliseconds == 0, "regulation stops at zero");
    Expect(rules.GetState().Phase == MatchPhase::Regulation, "regulation awaits pending shot");
    Expect(rules.ResolveShot(second, true), "shot released before both deadlines scores");
    Expect(rules.GetState().AwayScore == 2, "pending outside shot scores two");
    Expect(rules.GetState().Winner == Team::Away, "basket at deadline breaks tie");
}

void ClockFinishAndOvertime()
{
    MatchRules rules({11, 100, 200});
    Expect(rules.CheckIn(Team::Home), "home starts timed match");
    MakeBasket(rules, Team::Home, ShotRange::InsideArc);
    Expect(rules.CheckIn(Team::Away), "away checks in trailing");
    rules.AdvanceTime(100);
    Expect(rules.GetState().Phase == MatchPhase::Finished, "regulation ends with leader");
    Expect(rules.GetState().Winner == Team::Home, "leader wins at regulation expiry");
    Expect(!rules.CheckIn(Team::Away), "finished match cannot restart by check-in");

    rules.Reset();
    Expect(rules.CheckIn(Team::Home), "rematch starts after check-in");
    rules.AdvanceTime(101);
    Expect(rules.GetState().Phase == MatchPhase::Overtime, "regulation tie enters overtime");
    Expect(rules.GetState().ShotClockRemainingMilliseconds == 99, "overtime keeps the live shot clock");
    MakeBasket(rules, Team::Home, ShotRange::InsideArc);
    Expect(rules.GetState().Winner == Team::Home, "next basket wins overtime");
}

void TargetScore()
{
    MatchRules rules;
    for (int round = 0; round < 5; ++round)
    {
        Expect(rules.CheckIn(Team::Home), "home check-in for score chase");
        MakeBasket(rules, Team::Home, ShotRange::OutsideArc);
        Expect(rules.CheckIn(Team::Away), "away check-in for score chase");
        Expect(rules.OutOfBounds(Team::Away), "away turnover");
    }
    Expect(rules.CheckIn(Team::Home), "home check-in at ten");
    MakeBasket(rules, Team::Home, ShotRange::InsideArc);
    Expect(rules.GetState().HomeScore == 11, "game reaches eleven");
    Expect(rules.GetState().Winner == Team::Home, "first to target wins");
    Expect(rules.GetState().Ball == BallPhase::DeadBall, "match win kills play");
}

void RematchRejectsStaleShots()
{
    MatchRules rules;
    for (int rematch = 0; rematch < 5; ++rematch)
    {
        Expect(rules.CheckIn(Team::Home), "home checks in before rematch");
        MakeBasket(rules, Team::Home, ShotRange::InsideArc);
        Expect(rules.CheckIn(Team::Away), "away checks in before rematch");
        const auto staleShot = rules.BeginShot(Team::Away, ShotRange::InsideArc);
        rules.Reset(Team::Away);
        const auto& reset = rules.GetState();
        Expect(reset.Phase == MatchPhase::Regulation && reset.Ball == BallPhase::AwaitingCheckIn,
               "rematch restores initial phases");
        Expect(reset.Winner == Team::None && !reset.RequiresClear, "rematch clears winner and clearance");
        Expect(reset.HomeScore == 0 && reset.AwayScore == 0, "rematch resets scores");
        Expect(reset.RegulationRemainingMilliseconds == 240000 && reset.ShotClockRemainingMilliseconds == 12000,
               "rematch resets clocks");
        Expect(rules.CheckIn(Team::Away), "selected rematch opening team can check in");
        const auto newShot = rules.BeginShot(Team::Away, ShotRange::InsideArc);
        Expect(newShot != staleShot && newShot != InvalidShot, "rematch issues a fresh shot ID");
        Expect(!rules.ResolveShot(staleShot, true), "old callbacks cannot affect the new match");
        Expect(rules.ResolveShot(newShot, true), "fresh shot scores in rematch");
    }
}
}

int main()
{
    CheckInAndScoring();
    ClearArcAndBounds();
    ShotClockAndClockPartition();
    PendingShotAtDeadline();
    ClockFinishAndOvertime();
    TargetScore();
    RematchRejectsStaleShots();
    std::cout << "7 match rules scenarios passed.\n";
}
