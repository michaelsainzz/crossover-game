# Agent development harness

MarketMind supplies the pattern for scoped branches, up-front acceptance criteria, independent review, and task evidence. Crossover adds ownership for Unreal's binary assets, an engine-backed build gate, and playable product approvals. Start with a coordinator, one implementer, and one independent reviewer. Add a second writer only when the work can be separated cleanly.

| Role | Responsibility |
| --- | --- |
| Product owner | Approves feel, scope, visual direction, spending, and release. Receives a clip or playable build and plain-language options. |
| Coordinator and integrator | Owns milestones, tickets, interfaces, merge ordering, shared maps and config, and evidence on the integrated commit. |
| Gameplay engineer | Owns assigned C++ gameplay, movement, rules or bots and their deterministic tests. |
| Presentation engineer | Owns assigned court art, animation, HUD and sound assets and records provenance. Tunes presentation through Blueprints. |
| Build and QA reviewer | Independently reproduces acceptance checks, reviews the diff and playable evidence, and reports failed, blocked, or unverified checks. |

## How a ticket moves

1. The coordinator records the behavior and failure cases before assigning work. Identify dependencies, the starting commit, write paths, binary asset owner, allowed tools, acceptance checks, and any product gate.
2. Each writer works from that commit in its own worktree. Run editor automation and packaging one job at a time on the reference Mac. Give the court, shared Blueprints, and configuration a single writer. Place new props in separate assets and have the integrator wire them into the court. Use Git LFS for Unreal packages before the first binary commit. Coordinate ownership even if the Git host does not support locking.
3. The implementer hands off a commit, changed paths, checks and logs, and any deviations. The reviewer receives the task, diff, commit, and evidence and reruns the critical checks independently. Return an actionable failure to its owner with a repro. Escalate two unsuccessful attempts with the evidence and the smallest decision needed.
4. The coordinator integrates dependencies first and reruns affected checks at the new head. A draft PR can hold partial work with open gates listed. Ready for merge means every applicable technical gate passed and the owner accepted any required product decision. If no independent reviewer is available, record that gate as open.
5. Record the accepted build commit and gate decision. If a change breaks the playtest baseline, revert the offending change through a PR and retry from the last passing build.

## Evidence by change type

| Check | Run when | Pass evidence |
| --- | --- | --- |
| Foundation | Every PR | Hosted checks on the current head validate repo docs, portable C++ match rules, and the Mac verifier. |
| Rules | Gameplay and rule edits | C++ tests cover inside/outside arc, possession and clear requirements, score once per basket, timed expiry and overtime, and clean resets. Inject clock and random seeds where needed. |
| Integration | Changes to play, controls, HUD, maps, assets or build config | On the Mac, compile and run Unreal functional tests for match start, five bots, passing, shot/score/HUD, possession transitions and rematch. Resolve asset references and test binary files are hydrated. |
| Package and performance | M0 and M4 for package, M2 onward for performance | Mac build launches outside the editor. M4 includes a full real-input match and rematch. Record the M2 performance baseline and compare M3/M4 against the slice budget. |
| Feel and originality | M1, M2, M3, M4 as applicable | Owner decision tied to the build commit and clip or playable build. Record feedback separately from automated results. |

M0 installs the local Mac commands and records how to run each lane. Use Unreal's Automation Test Framework with a repository-owned test prefix and a fresh report folder per run. Fail on build errors, timeouts, missing reports, zero or missing expected tests, or failed tests. Keep editor logs and JSON reports tied to the SHA. Add the packaged smoke check via Unreal Automation Tool first, and consider Gauntlet after packaged scenarios multiply. Run Unreal checks locally before wiring trusted CI. Keep untrusted public PRs off the personal Mac and any self-hosted runner. [Epic automation tests](https://dev.epicgames.com/documentation/unreal-engine/run-automation-tests-in-unreal-engine) · [Unreal Automation Tool](https://dev.epicgames.com/documentation/unreal-engine/unreal-automation-tool-overview-for-unreal-engine)

## Ticket and run record

Copy this list into each issue or PR. Keep private logs outside public Git and attach a redacted summary.

- Ticket, gate and owner
- Expected behavior, failure cases and acceptance checks
- Base SHA, branch, write paths, asset owner and dependencies
- Implementer and reviewer, tool/model when visible
- Exact commands, exit status, expected and observed test counts, artifact links and tested SHA
- Findings, corrective attempts and owner interventions
- Result per gate (passed, failed, blocked, or pending), final SHA and next action
- Elapsed and human review time, cost and tokens when available

At each milestone count first-pass acceptances, rework cycles, and blocked tickets. Track build time, packaging reliability, and playtest feedback separately so agent speed and product quality can be evaluated independently.

See [Run Codex and Gemini together on your Mac](MODEL_WORKFLOW.md) for the local documentation pilot.
