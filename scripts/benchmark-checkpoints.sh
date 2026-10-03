#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'HELP'
Usage: bash scripts/benchmark-checkpoints.sh [options]
  --reference REF     Git revision to compare against (default: nominated best)
  --candidate REF     Git revision to measure (default: HEAD)
  --rounds N          Alternating paired rounds (default: 3)
  --repetitions N     Repetitions per case, at least 2 (default: 10)
  --min-time TIME     Minimum repetition time, e.g. 1s (default: 1s)
  --jobs N            Parallel build jobs, not measurement jobs (default: 4)
  --output DIR        New archive directory; must not already exist
  --dry-run           Execute bodies once; generated timings are smoke tests only
  --help              Show this help

Requires Bash, Git, CMake, jq, and the compiler in the benchmark preset.
Only committed revisions are measured. References are never promoted or replaced.
HELP
}

fail() { printf 'Error: %s\n' "$*" >&2; exit 1; }
need_value() { [ "$#" -ge 2 ] && [ -n "$2" ] || fail "Missing value for $1"; }
script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo=$(git -C "$script_dir" rev-parse --show-toplevel)
reference=
candidate=HEAD
rounds=3
repetitions=10
min_time=1s
jobs=4
output=
dry_run=false
while [ "$#" -gt 0 ]; do
    case "$1" in
        --reference) need_value "$@"; reference=$2; shift 2 ;;
        --candidate) need_value "$@"; candidate=$2; shift 2 ;;
        --rounds) need_value "$@"; rounds=$2; shift 2 ;;
        --repetitions) need_value "$@"; repetitions=$2; shift 2 ;;
        --min-time) need_value "$@"; min_time=$2; shift 2 ;;
        --jobs) need_value "$@"; jobs=$2; shift 2 ;;
        --output) need_value "$@"; output=$2; shift 2 ;;
        --dry-run) dry_run=true; shift ;;
        --help|-h) usage; exit 0 ;;
        *) fail "Unknown option: $1" ;;
    esac
done
for tool in git cmake jq; do
    command -v "$tool" >/dev/null || fail "Required command not found: $tool"
done
for number in "$rounds" "$repetitions" "$jobs"; do
    [[ "$number" =~ ^[1-9][0-9]*$ ]] || fail "Counts must be positive integers"
done
[ "$repetitions" -ge 2 ] || fail "At least two repetitions are needed for medians"
[[ "$min_time" =~ ^[0-9]+([.][0-9]+)?s$ ]] || fail "--min-time must be seconds, e.g. 1s"
jq -en --arg time "$min_time" '$time | rtrimstr("s") | tonumber > 0' >/dev/null \
    || fail "--min-time must be greater than zero"
if [ -z "$reference" ]; then
    best="$repo/build/benchmark-clang/benchmark-results/general-best.metadata.json"
    [ -f "$best" ] || fail "No nominated checkpoint metadata; supply --reference REF"
    reference=$(jq -er '.revision | select(type == "string" and length > 0)' "$best")
fi
reference_commit=$(git -C "$repo" rev-parse --verify --end-of-options "${reference}^{commit}")
candidate_commit=$(git -C "$repo" rev-parse --verify --end-of-options "${candidate}^{commit}")
if [ -z "$output" ]; then
    output="$repo/build/benchmark-clang/benchmark-results/comparisons/$(date -u +%Y%m%dT%H%M%SZ)-$$"
fi
[ ! -e "$output" ] || fail "Output already exists: $output (will not overwrite it)"
case "$output" in /*) ;; *) output="$PWD/$output" ;; esac
mkdir -p "$output"
output=$(cd "$output" && pwd)
printf 'Reference: %s\nCandidate: %s\nArchive: %s\n' "$reference_commit" "$candidate_commit" "$output"
if [ "$reference_commit" = "$candidate_commit" ]; then
    printf 'Same revision: this is a repeatability check, not an optimization.\n'
fi
if [ -n "$(git -C "$repo" status --porcelain --untracked-files=no)" ]; then
    printf 'Note: working-tree edits are not measured; only the resolved commits are used.\n'
fi
printf 'Full measurements can take 20 minutes or more. Do not run other builds or benchmarks.\n'

worktrees=()
cleanup() {
    status=$?
    trap - EXIT
    if [ "$status" -eq 0 ]; then
        for tree in "${worktrees[@]}"; do
            git -C "$repo" worktree remove "$tree" || \
                printf 'Worktree retained; remove manually if appropriate: %s\n' "$tree" >&2
        done
    else
        printf 'Run failed. Logs and worktrees retained in %s\n' "$output" >&2
    fi
    exit "$status"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

for role in reference candidate; do
    if [ "$role" = reference ]; then commit=$reference_commit; else commit=$candidate_commit; fi
    tree="$output/worktrees/$role"
    mkdir -p "$output/$role"
    git -C "$repo" worktree add --detach "$tree" "$commit"
    worktrees+=("$tree")
    configure=(cmake --preset benchmark)
    # Reuse pinned sources only when both revisions declare the same dependencies.
    if git -C "$repo" diff --quiet "$commit" HEAD -- CMakeLists.txt; then
        for dependency in google_benchmark nlohmann_json; do
            source_dir="$repo/build/benchmark-clang/_deps/${dependency}-src"
            if [ -d "$source_dir" ]; then
                upper=$(printf '%s' "$dependency" | tr '[:lower:]' '[:upper:]')
                configure+=("-DFETCHCONTENT_SOURCE_DIR_${upper}=$source_dir")
            fi
        done
    fi
    printf 'Building %s; logs in %s/%s\n' "$role" "$output" "$role"
    printf '%q ' "${configure[@]}" > "$output/$role/configure.command"
    printf '\n' >> "$output/$role/configure.command"
    (cd "$tree" && "${configure[@]}") > "$output/$role/configure.log" 2>&1
    build=(cmake --build --preset benchmark --target query_engine_general_benchmark -j "$jobs")
    printf '%q ' "${build[@]}" > "$output/$role/build.command"
    printf '\n' >> "$output/$role/build.command"
    (cd "$tree" && "${build[@]}") \
        > "$output/$role/build.log" 2>&1
    flags="$tree/build/benchmark-clang/CMakeFiles/query_engine.dir/flags.make"
    [ -f "$flags" ] || fail "Expected Unix Makefiles flags in $flags"
    cp "$flags" "$output/$role/flags.make"
    compiler=$(sed -n 's/^CMAKE_CXX_COMPILER:FILEPATH=//p; s/^CMAKE_CXX_COMPILER:STRING=//p' \
        "$tree/build/benchmark-clang/CMakeCache.txt" | head -n 1)
    [ -n "$compiler" ] || fail "Compiler not found in CMake cache"
    "$compiler" --version > "$output/$role/compiler.txt"
done
cmp -s "$output/reference/compiler.txt" "$output/candidate/compiler.txt" \
    || fail "Compiler versions differ; comparison is not controlled"
reference_flags=$(sed -n 's/^CXX_FLAGS = //p' "$output/reference/flags.make")
candidate_flags=$(sed -n 's/^CXX_FLAGS = //p' "$output/candidate/flags.make")
[ "$reference_flags" = "$candidate_flags" ] || fail "Engine compile flags differ"

machine=$(uname -sm)
cpu=unknown
memory=unknown
power=unknown
if command -v sysctl >/dev/null; then
    cpu=$(sysctl -n machdep.cpu.brand_string 2>/dev/null || printf unknown)
    memory=$(sysctl -n hw.memsize 2>/dev/null || printf unknown)
fi
if command -v sw_vers >/dev/null; then machine="$machine $(sw_vers -productVersion)"; fi
if command -v pmset >/dev/null; then power=$(pmset -g batt); fi
for role in reference candidate; do
    if [ "$role" = reference ]; then commit=$reference_commit; else commit=$candidate_commit; fi
    jq -n --arg revision "$commit" --arg role "$role" --arg machine "$machine" \
        --arg cpu "$cpu" --arg memory "$memory" --arg power "$power" \
        --arg flags "$reference_flags" --arg compiler "$(cat "$output/$role/compiler.txt")" \
        --arg cmake "$(cmake --version | head -n 1)" --arg min_time "$min_time" \
        --argjson rounds "$rounds" --argjson repetitions "$repetitions" \
        --argjson dry_run "$dry_run" \
        '{revision: $revision, role: $role, machine: $machine, cpu: $cpu,
          memory_bytes: $memory, power: $power, compiler: $compiler, cmake: $cmake,
          build_preset: "benchmark", build_type: "Release", flags: $flags,
          min_time: $min_time, rounds: $rounds, repetitions: $repetitions,
          random_interleaving: true, dry_run: $dry_run,
          summary_method: "median of per-round medians; range of per-round medians",
          limitations: "Background load, temperature, and CPU scheduling are not isolated"}' \
        > "$output/$role/metadata.json"
done

for ((round=1; round<=rounds; round++)); do
    if ((round % 2)); then order=(reference candidate); else order=(candidate reference); fi
    for role in "${order[@]}"; do
        command=("$output/worktrees/$role/build/benchmark-clang/query_engine_general_benchmark"
            "--general_summary=$output/$role/round-$round.json"
            "--benchmark_out=$output/$role/round-$round-raw.json"
            --benchmark_out_format=json "--benchmark_min_time=$min_time"
            "--benchmark_repetitions=$repetitions" --benchmark_enable_random_interleaving)
        if [ "$dry_run" = true ]; then command+=(--benchmark_dry_run); fi
        printf '%q ' "${command[@]}" > "$output/$role/round-$round.command"
        printf '\n' >> "$output/$role/round-$round.command"
        printf 'Round %s/%s: %s\n' "$round" "$rounds" "$role"
        "${command[@]}" > "$output/$role/round-$round.log" 2>&1
        jq -e '(.benchmarks | length > 0) and
            (all(.benchmarks[]; .error_occurred != true))' \
            "$output/$role/round-$round-raw.json" >/dev/null \
            || fail "Benchmark error in $role round $round; inspect its log"
    done
done

inputs=()
for role in reference candidate; do
    for ((round=1; round<=rounds; round++)); do inputs+=("$output/$role/round-$round.json"); done
done
jq -s --argjson rounds "$rounds" --argjson dry_run "$dry_run" \
    --arg reference "$reference_commit" --arg candidate "$candidate_commit" \
    -f "$script_dir/benchmark-report.jq" "${inputs[@]}" > "$output/comparison.json"
for role in reference candidate; do
    jq ".$role.summary" "$output/comparison.json" > "$output/$role/summary.json"
done
jq -r --arg min_time "$min_time" --argjson repetitions "$repetitions" \
    -f "$script_dir/benchmark-markdown.jq" "$output/comparison.json" > "$output/comparison.md"
printf 'Completed. Markdown: %s/comparison.md\nRaw data and provenance: %s\n' "$output" "$output"
