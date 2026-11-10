# Project status

## Bootstrap and first prototype

Product direction accepted. Build an original, offline 3v3 half-court vertical slice for one player and five bots, with the owner approving playable product checkpoints.

| Gate | State | Evidence |
| --- | --- | --- |
| Repository and agent workflow | Implemented | One integration branch, a written handoff and ownership protocol, and a hosted portable check workflow. |
| Native rules | Passed outside Unreal | Seven C++ scenarios, including possession, defensive clears, clock expiry, overtime, and five consecutive resets. |
| Mac verifier | Passed outside Unreal | Five Python checks plus a simulated build, report, package and launch path. |
| M0 · Unreal build and package | Blocked | The reference Mac has Xcode 26.6 and SDK 26.5. No Unreal installation was found, and the Mac lane stopped before compilation. |
| M1 · Shot feel | In progress | Native practice court, camera, character, input, shot meter, and scripted scoring are in source. First compile and graphical playtest are pending M0. |
| M2 · Full 3v3 | In progress | Portable rule engine is available. Integration, teammates, opponents, passing and the match HUD remain open. |

The coordinator kept ownership of project config and integration. Parallel workers owned the court, the match rules, and the Mac verification lane in disjoint paths. A separate agent inspected the integrated Unreal source. The engine-backed build and playable approval gates remain open. The [toolchain plan](TOOLCHAIN.md) describes the next run.
