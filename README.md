# Crossover

Crossover is the working name for an original 3v3 arcade streetball game in Unreal Engine. Think quick passes, readable team play, exaggerated dunks, and stylized 3D with a retro broadcast feel.

The first planned match is one short half-court game for one player and five bots. You approve what the game should feel like at playable checkpoints. The agents handle implementation, testing, integration, and fixes.

## Current build

Night Shift opens on the Dusk District rooftop court. Choose Rookie, Street, or Legend at the title screen. Street is the default. You play for the teal Home team with two bot teammates against three orange Away bots. A gold marker shows the player you control. Win by reaching 11 or leading after four minutes. A tie goes to the next basket.

| Action | Keyboard | Controller |
| --- | --- | --- |
| Start from the title screen | Press and release Space | Press and release bottom face button |
| Cycle difficulty on the title screen | E | Left face button |
| Move and sprint | WASD and Left Shift | Left stick and LB |
| Shoot or dunk near the rim | Hold and release Space | Hold and release bottom face button |
| Pass on offense or switch on defense or a loose ball | E | Left face button |
| Contest or steal | F | Right face button |
| Activate a full Overtake meter during live play | Q | RB |
| Pause or resume during a match, or rematch after the result | R | Menu |

Release the shot in the green window. Aim a pass with movement or press pass while stopped. Control follows the receiver. After a defensive recovery, carry the ball outside the arc before shooting. Home baskets and player steals fill the Overtake meter. The boost temporarily increases movement speed and widens the release window on open shots.

- [Current status and next step](docs/STATUS.md)
- [First playable build and milestones](docs/SLICE.md)
- [Technical architecture](docs/ARCHITECTURE.md)
- [Night Shift visuals and audio provenance](docs/PRESENTATION.md)
- [Agent roles, handoffs, and test gates](docs/HARNESS.md)
- [Mac setup and verification](docs/TOOLCHAIN.md)
- [Instructions for agents](AGENTS.md)

The portable checks require Python 3.9+, Bash, and a C++17 compiler.

```sh
python3 scripts/check_foundation.py
bash scripts/test_rules.sh
python3 scripts/ue_mac_test.py
python3 scripts/ue_perf_test.py
```
