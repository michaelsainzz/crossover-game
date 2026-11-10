# Project status

## Bootstrap and first prototype

Product direction accepted. Build an original, offline 3v3 half-court vertical slice for one player and five bots, with the owner approving playable product checkpoints.

| Gate | State | Evidence |
| --- | --- | --- |
| Repository and agent workflow | Implemented | One integration branch, a written handoff and ownership protocol, and a hosted portable check workflow. |
| Native rules | Passed outside Unreal | Seven C++ scenarios, including possession, defensive clears, clock expiry, overtime, and five consecutive resets. |
| Mac verifier | Passed | Five Python checks passed. Real Unreal packaging exposed a launch log assumption. The verifier now reads the readiness marker from the captured process output. |
| M0 · Unreal build and package | Passed on reference Mac | A clean checkout at `ca1aecf` built with UE 5.8.4, Xcode 26.6 and SDK 26.5. `Crossover.Smoke` passed with zero warnings and errors. UAT produced an arm64 Mac app. The packaged app logged `CROSSOVER_SMOKE_READY` and exited 0, and the local signed copy opened with Metal graphics. |
| M1 · Shot feel | Awaiting owner playtest | The packaged practice court rendered and the player moved, scored, and received missed-release feedback. This is a grayscale blockout. Controller input and product approval of movement, camera, and shot timing remain open. |
| M2 · Full 3v3 | In progress | Portable rule engine is available. Integration, teammates, opponents, passing and the match HUD remain open. |

The coordinator kept ownership of project config and integration. Parallel workers owned the court, the match rules, and the Mac verification lane in disjoint paths. A separate agent inspected the integrated Unreal source. An independent reviewer confirmed the clean checkout, editor build, editor report, Mac package, and packaged readiness evidence. Raw evidence is kept locally. The [toolchain plan](TOOLCHAIN.md) describes how to repeat the run.
