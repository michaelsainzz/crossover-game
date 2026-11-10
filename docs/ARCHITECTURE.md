# Technical architecture

## Current project

| Area | Responsibility |
| --- | --- |
| Unreal targets | `Crossover.uproject` selects the UE 5.8 candidate and one runtime module. The editor target compiles the project and exposes `Crossover.Smoke`. |
| Practice world | The engine's Entry map starts `ACrossoverPracticeMode`, which creates a native court, camera and practice ball. The procedural court uses Unreal primitives. No imported source assets are needed for this checkpoint. |
| Player and shot feel | `ACrossoverPracticePlayer` owns movement and Enhanced Input. Practice mode resolves shot timing and court range, drives the scripted ball arc, and supplies data for the practice HUD. These are tuning defaults for the first playtest. |
| Match rules | `Crossover::Rules::MatchRules` owns possession, clear requirements, score, clocks and win conditions in portable C++. All inputs are explicit, including elapsed time and shot outcomes. Shot IDs guard against repeated or stale resolutions. |
| Verification | Hosted checks cover the repository, match rules, and report verifier. The Mac lane builds with Unreal, requires an editor smoke result, and runs the packaged game to its readiness marker. The first graphical run remains a separate gate. |

## Connect the 3v3 match

1. Finish the one-hoop feel checkpoint with movement, sprint, release timing, contextual dunk and a readable broadcast camera. Keep the practice court as a repeatable tuning and testing environment.
2. Add a match game mode that owns one rules instance. Translate ball recovery, arc crossing, out-of-bounds, shot release, and shot resolution into rule calls. Carry fractional frame time across ticks so the millisecond clock remains accurate. Once a shot is accepted, resolve its ID exactly once.
3. Publish a read-only match snapshot through Unreal's game state. Build the score, possession and clock HUD from that snapshot. The UI and court presentation consume state and events.
4. Let human input and bots issue the same movement and play actions. Start bot behavior with state-driven spacing, marking, shot selection and loose-ball recovery. Use a seed for repeatable scripted checks. Add tactical complexity after one full match plays cleanly.
5. Keep rules and control logic in C++. Once the binary asset workflow is verified, use Blueprints and data assets for tuning, visual effects, animations, UI and sound. Record provenance before adding assets. Use a single owner for each binary map or Blueprint.

Current rules defaults use a 12-second shot clock. Both clocks pause during check-in. A shot released before the buzzer can finish, and a made basket hands the ball to the other team. A defensive recovery must clear beyond the arc before the next shot. These are playtest defaults.

The practice scoreboard is session-local. The full-match mode, rule integration, passing, bots, and rematch UI land at the 3v3 checkpoint.
