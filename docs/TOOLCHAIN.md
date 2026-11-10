# Unreal setup and first engineering ticket

## Reference Mac

Observed Apple M4, 16 GB unified memory, macOS 26.5.1, Xcode 26.6, and macOS SDK 26.5. The Launcher installed UE 5.8.4. Xcode's optional Metal Toolchain reports Apple Metal 32023.883. The internal disk had about 11 GiB free after setup. Git LFS is still required before committing binary Unreal assets.

Epic lists 16 GB as its memory minimum and 32 GB as its recommendation. Start with simple meshes, baked or inexpensive lighting, and scalable effects. Leave Nanite, Lumen, and expensive shadows off in the baseline. Close memory-heavy apps during the first shader compile. Check the Epic installer's actual disk estimate and allow additional room for caches, intermediates, and a packaged build. If internal space is tight, use available fast external storage after confirming it can hold the installation and work files.

UE 5.8.4 compiled the Mac editor target with Xcode 26.6 (17F113) and macOS SDK 26.5. Keep this toolchain for the first playtest. If compilation fails, capture the log, compare the specific patch's requirements and known issues, and test a supported toolchain side by side. Try another supported Unreal patch if needed. Preserve the existing Xcode setup. [Epic macOS requirements](https://dev.epicgames.com/documentation/unreal-engine/macos-development-requirements-for-unreal-engine) · [UE 5.8 release notes](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-release-notes)

Use macOS as the development and first playtest target. A Windows build requires a separate supported Windows x64 build and test environment with its own packaging and GPU measurements. Keep platform-specific SDKs and plugins out of the initial project. Use native Unreal C++ and Enhanced Input, Blueprints for presentation, and simple state-driven bots. Prefer repo-owned assets and reproducible editor actions. Live editor tools can speed up authoring after the project opens, but a clean checkout and a recorded build command are the proof.

## M0 · Prove the build loop

The agent owns setup diagnosis and implementation. The owner handles any Epic sign-in or system dialogs that require personal access.

1. Record the Launcher's install size and available disk space. Install the selected Unreal patch and Git LFS. Before adding production content, confirm a clean clone can download a real Unreal package through LFS.
2. Run the verification command below from a fresh checkout with Xcode selected. It builds the C++ editor, requires a clean `Crossover.Smoke` automation result, cooks and archives a Mac Development build, and checks that the standalone game starts the native sandbox and exits cleanly. The engine's Entry map hosts the procedural test court until a dedicated court asset is ready.
3. Open the archived `Crossover.app` normally from Finder. Confirm the sandbox draws and accepts input. Record the graphical launch result separately from the headless smoke test.
4. Record the successful engine patch, Xcode build, SDK, tested commit, logs, app launch result, and disk footprint in the PR. If a step fails or a tool is missing, preserve the failure and keep M0 open.

### Run the Mac verification lane

Use Python 3.9 or newer. Finish Xcode’s first-time setup and install its Metal Toolchain component for cooking and graphics. The runner verifies the actual compiler with `xcrun --sdk macosx metal --version`. Set the path to the installed Unreal version, using the directory that contains `Engine/`.

```sh
python3 scripts/ue_mac.py --ue-root "/Users/Shared/Epic Games/UE_5.8"
```

The command also accepts the path through `UE_ROOT`. Run one instance at a time and close Unreal Editor before the build. Build, test and packaging steps each time out after 30 minutes by default. Use `--step-timeout` to increase that limit for a cold shader cache. The headless standalone launch is capped at 120 seconds.

Use a clean local checkout outside any file-provider-managed folder for packaging. On the reference Mac, the Documents file provider added Finder metadata to the generated app bundle, and Xcode refused to sign it. A checkout outside Documents passed the signing step. [Apple code-signing diagnosis](https://developer.apple.com/library/archive/qa/qa1940/_index.html)

Each run writes to a fresh `Saved/MacVerification/run-*` directory. Keep the engine, Xcode and SDK metadata, command logs, editor test report and standalone game log with the run record. The archive lives under that run's `archive/` directory. Keep raw logs private and attach redacted excerpts to public PRs. [Unreal automation report format](https://dev.epicgames.com/documentation/unreal-engine/review-test-results-in-unreal-engine) · [BuildCookRun guide](https://dev.epicgames.com/documentation/unreal-engine/build-operations-cooking-packaging-deploying-and-running-projects-in-unreal-engine)

Without Unreal, run `python3 scripts/ue_mac_test.py` to exercise the verifier's missing-tool, timeout, and report checks. To inspect an existing result, pass `--check-report /path/to/index.json`. That report-only command does not build or launch the game.

Ignore generated engine files and place `.uasset` and `.umap` files in Git LFS. Review LFS storage and bandwidth before importing larger sets. Assets that exceed the approved budget or lack redistribution rights stay outside the public repository.
