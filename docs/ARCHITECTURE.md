# Technical architecture

## Current project

| Area | Responsibility |
| --- | --- |
| Unreal targets | `Crossover.uproject` selects the UE 5.8 candidate and one runtime module. The editor target compiles the project and exposes `Crossover.Smoke`. |
| Practice world | The engine's Entry map starts `ACrossoverMatchMode`. It reuses the practice mode's native court, broadcast camera and ball. The procedural court uses Unreal primitives. No imported source assets are needed for this checkpoint. |
| Player and shot feel | `ACrossoverPracticePlayer` owns movement and Enhanced Input. Match mode resolves shot timing and court range and drives scripted passes and shot arcs. The practice release window remains the default. |
| Match rules | `Crossover::Rules::MatchRules` owns possession, clear requirements, score, clocks and win conditions in portable C++. All inputs are explicit, including elapsed time and shot outcomes. Shot IDs guard against repeated or stale resolutions. |
| Verification | Hosted checks cover the repository, match rules, and report verifier. The Mac lane builds with Unreal, requires an editor smoke result, and runs the packaged game to its readiness marker. The first graphical run remains a separate gate. |

## Match integration

- Match mode owns one rules instance and a six-player roster. It translates check-ins, clears, turnovers, rebounds and resolved shots into rule calls. It carries fractional milliseconds across frames.
- The human and bots use the same character movement component. Enhanced Input moves the human. Bot targets cover spacing, guarding, clearing, simple passes and loose balls. Possession changes can switch the controlled player while keeping the court camera fixed.
- The HUD reads the match state and local feedback from the game mode. A future replicated mode would publish the state through Unreal's game state. The first build runs offline.
- The packaged smoke scenario exercises the real match mode and possession switches with fixed positions and simulated time. Graphical play separately covers input, rendering and autonomous bot movement.
- After the feel checkpoint, use approved assets and Blueprints for visual tuning, effects, animation and sound. Record provenance and give each binary asset a single writer.

The match starts with a 12-second shot clock. Both clocks pause during check-in. A shot released before the buzzer can finish, and a made basket hands the ball to the other team. A defensive recovery must clear beyond the arc before the next shot.
