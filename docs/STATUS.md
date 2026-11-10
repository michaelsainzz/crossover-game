# Project status

## Bootstrap and first prototype

Product direction accepted. Build an original, offline 3v3 half-court vertical slice for one player and five bots, with the owner approving playable product checkpoints.

| Gate | State | Evidence |
| --- | --- | --- |
| Repository and agent workflow | Implemented | One integration branch, a written handoff and ownership protocol, and a hosted portable check workflow. |
| Native rules | Passed outside Unreal | Seven C++ scenarios, including possession, defensive clears, clock expiry, overtime, and five consecutive resets. |
| Mac verifier | Passed | Five Python checks passed. Real Unreal packaging exposed a launch log assumption. The verifier now reads the readiness marker from the captured process output. |
| M0 · Unreal build and package | Passed on reference Mac | A clean checkout at `ca1aecf` built with UE 5.8.4, Xcode 26.6 and SDK 26.5. `Crossover.Smoke` passed with zero warnings and errors. UAT produced an arm64 Mac app. The packaged app logged `CROSSOVER_SMOKE_READY` and exited 0, and the local signed copy opened with Metal graphics. |
| M1 · Shot feel | Approved | The owner accepted the grayscale practice playtest and authorized the 3v3 milestone. Controller hardware verification remains open. |
| M2 · Full 3v3 | Automated match verified, owner playtest pending | On `2b8213b` and continuation `gemini/m2-continuation`, the Unreal editor build, `Crossover.Smoke`, signed Mac package, and deterministic match launch passed. The scenario verified six pawns, a pass and control transfer, scoring, defender switching, steals, clears, rebounds, shot-clock turnover, regulation, overtime, and five rematches. Build reliability was hardened with `-NoUBA` to eliminate socket hangs in sandboxed toolchains, packaging ignores were cleanly isolated, loose-ball switching was added, and AI defensive contest mechanics were implemented for fair shooting. A short diagnostic sampled 720 autonomous match frames on reference Apple Silicon M4 with Metal SM6 graphics (median 13.95 ms vs <= 16.7 ms target; 95th percentile 14.31 ms vs <= 22.0 ms target; max 16.51 ms). The short diagnostic leaves the full-regulation performance gate open. See the [full-regulation capture procedure](PERFORMANCE.md). Physical controller testing and owner feedback on spacing, readability and difficulty remain open. |
| M3 · Night Shift art and sound | Playtest candidate | Recovered the local dusk court, animated athletes, broadcast HUD, audio and effects and integrated them with the Gemini gameplay branch. See the [presentation provenance and review checklist](PRESENTATION.md). The owner reviews direction and readability in a graphical build. |
| M4 · Shareable build | Open | Requires a full real-input match and rematch in a standalone Mac build, applicable performance evidence, and a separate distribution decision. |

The coordinator kept ownership of project config and integration. Parallel workers owned the court, the match rules, and the Mac verification lane in disjoint paths. A separate agent inspected the integrated Unreal source. An independent reviewer confirmed the clean checkout, editor build, editor report, Mac package, and packaged readiness evidence. The M2 implementation was split between character/input, HUD, match rules integration, and the deterministic test. Reviewers inspected the Unreal possession path and independently confirmed the packaged scenario result. Raw evidence is kept locally. The [toolchain plan](TOOLCHAIN.md) describes how to repeat the run.
