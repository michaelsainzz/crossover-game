# Run Codex and Gemini together on your Mac

One Python coordinator starts the two command-line agents. Codex writes in a ticket worktree. Gemini reviews the committed diff in a separate empty folder. The coordinator runs the checks itself and records the candidate SHA, diff, tool outputs and result in a private local folder. Both desktop apps can stay open for manual work. The script does not drive either app's UI.

This first lane handles Markdown tickets. Gameplay, assets, maps, configuration and CI changes use the existing [engineering harness](HARNESS.md) with an assigned writer and independent reviewer. Keeping the pilot to Markdown lets the coordinator run existing portable checks without executing new agent-written code. The writer's workspace sandbox and the reviewer's deny rules are both required. Git worktrees separate changes and are not a permission boundary.

## 1. Install and sign in once

Open Terminal. Use the official [Codex CLI setup](https://developers.openai.com/codex/cli) and [Antigravity CLI setup](https://antigravity.google/docs/cli/install/) if either command is missing. Complete sign-in interactively with `codex` and `agy`, then exit their interfaces. Confirm the tools and list the available Gemini model slugs.

```sh
python3 --version
codex --version
agy --version
agy models
```

Codex supports [non-interactive execution](https://learn.chatgpt.com/docs/non-interactive-mode), the [Codex SDK](https://learn.chatgpt.com/docs/codex-sdk), and [App Server](https://learn.chatgpt.com/docs/app-server) for custom clients. Google supports [Antigravity CLI headless mode](https://antigravity.google/docs/cli/headless/) and a [Python Antigravity SDK](https://antigravity.google/docs/sdk/overview/). The CLI pair uses normal sign-ins and requires no SDK dependency or new API key. Google directs [consumer Gemini CLI users](https://developers.google.com/gemini-code-assist/docs/deprecations/code-assist-individuals) to Antigravity. Provider usage and quotas still apply.

## 2. Queue one documentation ticket

The coordinator applies the reviewer's deny rules for its own Antigravity CLI session using `--settings`. Your usual Antigravity sessions keep their existing settings. Use a CLI version that lists `--settings` and `--disable-slash-commands` under `agy --help`. [Google CLI permissions](https://antigravity.google/docs/permissions/?tab=cli)

In Terminal, open a Crossover Git checkout containing this workflow. Use a trusted commit that includes the code and docs you intend to build on. Running the script with a local queue is approval to send the ticket and relevant repository material to both providers. Keep secrets, private transcripts and restricted assets out of the ticket and checkout. Keep ticket text and diffs public-safe. The short review prompt is passed as a local command-line argument.

```sh
git status --short --branch
git rev-parse HEAD
```

Save the following as `crossover-queue.json` in your home folder. Replace `PASTE_FULL_SHA_HERE` with the full commit printed above. Keep the queue outside Git. Each ticket starts at its own explicit base and cannot consume another ticket's output automatically.

```json
[
  {
    "id": "controls-doc-pilot",
    "base": "PASTE_FULL_SHA_HERE",
    "paths": ["README.md"],
    "task": "After the README controls paragraph, add a short sentence explaining how to start a rematch once the result is shown.",
    "acceptance": "The new sentence names R on keyboard and Menu on controller, matching the controls table. Preserve every existing binding."
  }
]
```

```sh
python3 scripts/model_workflow.py ~/crossover-queue.json --plan
python3 scripts/model_workflow.py ~/crossover-queue.json --gemini-model YOUR_GEMINI_SLUG
```

Replace `YOUR_GEMINI_SLUG` with a Gemini entry from `agy models`. Leave Terminal open while it runs. Every ticket gets a fresh private clone and one branch named `workflow/<ticket-id>` in that clone. The source checkout, Gemini's existing branch and GitHub stay untouched. The runner checks the path allowlist, commits the draft, runs the four portable commands and asks Gemini to review. Gemini evaluates the ticket and diff. The command logs supply independent portable test evidence.

On failure the runner stops the queue and prints where to find the evidence. Open the private run folder and inspect the ticket's `*.log` and `*.err` files. Fix the ticket or environment and re-run once. If the second attempt fails, bring the evidence to the coordinator. A new run gets its own folder, so earlier candidates remain available.

## 3. Review and publish

Open the ticket's `candidate.diff`, `candidate.txt`, `gemini.log` and portable check logs. Review the exact SHA before publishing. To import an accepted candidate, run the following from your main project checkout, substituting the absolute run folder printed by the script and your ticket ID. The commands create a new local branch without switching your current checkout.

```sh
git fetch /ABSOLUTE/RUN/FOLDER/controls-doc-pilot/repo workflow/controls-doc-pilot
git branch workflow/controls-doc-pilot FETCH_HEAD
git log -1 workflow/controls-doc-pilot
```

After confirming the SHA, push that new branch and use GitHub's **Compare & pull request** button to open a **draft**. Choose the branch that contains the ticket's base. Require hosted checks on the published head and rerun affected checks if integration changes the candidate. An agent review is evidence for the coordinator. Merge, spend, asset rights, playtest and release decisions remain with the owner.

## Gameplay and Unreal gates

Use one writer per binary asset and map. Give the integrator shared configuration and input. For gameplay changes, hand the exact candidate and acceptance cases to the other model for review, then have the coordinator run the [Mac verification lane](TOOLCHAIN.md) on a trusted checkout, one engine job at a time. Attach redacted evidence for the editor build, smoke tests and package as required by the milestone. The owner approves feel, controls and readiness using a playable build. M2 requires a full regulation match performance capture under the [slice conditions](SLICE.md). Gemini's recorded 720-frame sample is partial evidence. Keep physical controller and owner playtest gates separate from automated test results.

The pilot uses a local, sequential queue. For the next lane, keep the same ticket and evidence contract, add a separately approved C++ write scope, isolate build execution, and require gameplay tests and the Mac gate before integration. A draft PR may carry open gates.
