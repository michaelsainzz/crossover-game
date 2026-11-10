# Unreal setup and first engineering ticket

## Reference Mac

Observed Apple M4, 16 GB unified memory, macOS 26.5.1, Xcode 26.6, and about 44 GiB of available space. The selected Xcode includes a Metal tool path. No Unreal install was found in the standard locations checked, and Git LFS was unavailable.

Epic lists 16 GB as its memory minimum and 32 GB as its recommendation. Start with simple meshes, baked or inexpensive lighting, and scalable effects. Leave Nanite, Lumen, and expensive shadows off in the baseline. Close memory-heavy apps during the first shader compile. Check the Epic installer's actual disk estimate and allow additional room for caches, intermediates, and a packaged build. If internal space is tight, use available fast external storage after confirming it can hold the installation and work files.

Evaluate the latest stable UE 5.8 patch offered in the Launcher as the first candidate. Epic's macOS page flags Xcode 26.4 as incompatible, and the installed 26.6 has yet to be proven on this Mac. Pin the exact engine patch, Xcode build, and macOS SDK only after M0 succeeds. If compilation fails, capture the log, compare the specific patch's requirements and known issues, and test a supported toolchain side by side. Try another supported Unreal patch if needed. Preserve the existing Xcode setup. [Epic macOS requirements](https://dev.epicgames.com/documentation/unreal-engine/macos-development-requirements-for-unreal-engine) · [UE 5.8 release notes](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-release-notes)

Use macOS as the development and first playtest target. A Windows build requires a separate supported Windows x64 build and test environment with its own packaging and GPU measurements. Keep platform-specific SDKs and plugins out of the initial project. Use native Unreal C++ and Enhanced Input, Blueprints for presentation, and simple state-driven bots. Prefer repo-owned assets and reproducible editor actions. Live editor tools can speed up authoring after the project opens, but a clean checkout and a recorded build command are the proof.

## M0 · Prove the build loop

The agent owns setup diagnosis and implementation. The owner handles any Epic sign-in or system dialogs that require personal access.

1. Record launcher space requirements, free space, engine candidates, Xcode version and active SDK. Install the chosen engine and Git LFS. Verify LFS is active and a clean clone can hydrate Unreal assets from GitHub before adding production content.
2. Create a minimal C++ project, an empty test court, and one known `Crossover.Smoke` test. Build the Mac editor target from a clean checkout. Run the test and confirm the report identifies it and marks it successful.
3. Add documented, bounded-time build and automation commands. Verify an empty or failed test run fails the check. Package a Mac Development build with Unreal Automation Tool and launch the app outside the editor.
4. Record the successful engine patch, Xcode build, SDK, commands, commit, logs, app launch result and disk footprint in the PR. If a step is blocked, preserve the exact failure and keep M0 open.

Ignore generated engine files and place `.uasset` and `.umap` files in Git LFS. Review LFS storage and bandwidth before importing larger sets. Assets that exceed the approved budget or lack redistribution rights stay outside the public repository.
