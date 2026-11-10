# Crossover

Crossover is the working name for an original 3v3 arcade streetball game in Unreal Engine. Think quick passes, readable team play, exaggerated dunks, and stylized 3D with a retro broadcast feel.

The first planned match is one short half-court game for one player and five bots. You approve what the game should feel like at playable checkpoints. The agents handle implementation, testing, integration, and fixes.

## Current build

The first practice prototype builds and runs on the reference Apple silicon Mac. It is ready for a shot-feel playtest. It generates a half-court and a hoop at runtime with a fixed broadcast camera. After a successful build, open `Crossover.uproject` and press Play. Move with WASD or the left stick, sprint with Left Shift or LB, and hold and release Space or the controller's bottom face button to shoot. Release in the green window to make the shot, or get close to the hoop for a contextual dunk.

- [Current status and next step](docs/STATUS.md)
- [First playable build and milestones](docs/SLICE.md)
- [Technical architecture](docs/ARCHITECTURE.md)
- [Agent roles, handoffs, and test gates](docs/HARNESS.md)
- [Mac setup and verification](docs/TOOLCHAIN.md)
- [Instructions for agents](AGENTS.md)

The portable checks require Python 3.9+, Bash, and a C++17 compiler.

```sh
python3 scripts/check_foundation.py
bash scripts/test_rules.sh
python3 scripts/ue_mac_test.py
```
