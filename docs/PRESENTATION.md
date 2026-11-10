# Night Shift presentation

Night Shift gives the offline 3v3 slice a rooftop court at dusk, six articulated players, a broadcast scoreboard, shot and dunk effects, and synthesized sound. The Nov 9, 2026 source work is preserved in the integration commit. The direction is ready for owner review once the packaged playtest and clip are available.

## Asset provenance

| In-game destination | Creator and source | Proof in the repository |
| --- | --- | --- |
| Dusk District court, hoop, skyline, lighting and signage | Codex-authored geometry, paint layout and palette from Unreal Engine primitives | [Court source](../Source/Crossover/Private/Prototype/CrossoverPracticeCourt.cpp) |
| Six fictional athletes, jerseys, poses and shot animations | Codex-authored poses and team palettes from Unreal Engine primitives | [Athlete source](../Source/Crossover/Private/Prototype/CrossoverAthleteVisual.cpp) |
| Title screen, scoreboard, ball trail and broadcast camera motion | Codex-authored canvas and gameplay presentation using Unreal Engine shapes, materials and fonts | [HUD source](../Source/Crossover/Private/Prototype/CrossoverMatchHUD.cpp) and [effects source](../Source/Crossover/Private/Prototype/CrossoverMatchPresentation.cpp) |
| Title, pause heading and score numerals | Barlow Condensed Bold 1.4 by The Barlow Project Authors. Unmodified font obtained Nov 10, 2026 from the [Google Fonts repository](https://github.com/google/fonts/tree/main/ofl/barlowcondensed) | [Font](../Content/ThirdParty/Barlow/BarlowCondensed-Bold.ttf) and [SIL Open Font License 1.1](../Content/ThirdParty/Barlow/OFL.txt) |
| Backing beat and court cues | Codex-authored procedural synthesis using Unreal's audio playback APIs | [Audio source](../Source/Crossover/Private/Prototype/CrossoverArcadeAudio.cpp) |

The project tracks the procedural source and references the engine's bundled content from the Unreal installation. Engine content is supplied through Unreal under its terms. Barlow is the only separately imported asset. Its license permits source redistribution and bundling with the game under the conditions in the accompanying OFL notice. The Mac package stages the unmodified font and its notice together. The font matches upstream Git blob `9e480f467d703009e3abfce290b2ec2b7d24e188`. Public distribution and title or logo clearance remain owner decisions under the [slice plan](SLICE.md).

## Review the playtest

Check the court, uniforms, ball and possession indicator at gameplay distance. Watch a pass, a shot and a dunk, then listen for the backing beat, score cue and turnover cue. Record the build commit, clip, and the owner's decision on readability, visual direction and audio separately from the automated [test gates](HARNESS.md).
