#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT

"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic \
    -I "$root/Source/Crossover/Public" \
    "$root/Source/Crossover/Private/Rules/MatchRules.cpp" \
    "$root/Tests/MatchRulesTests.cpp" \
    -o "$build_dir/match_rules_tests"
"$build_dir/match_rules_tests"
