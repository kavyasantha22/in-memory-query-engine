#!/usr/bin/env bash
set -euo pipefail
here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
fixture() {
    jq -n --argjson time "$2" '{schema_version: 1, baseline: null,
      groups: {scan: [{benchmark: "scan/10", label: "Scan", rows: 10,
        median: {real_time_ms: $time, cpu_time_ms: $time, items_per_second: 100},
        change_from_baseline_percent: null}]}}' > "$1"
}
inputs=()
for role in reference candidate; do
    for round in 1 2 3; do
        if [ "$role" = reference ]; then time=$((round * 10)); else time=$round; fi
        file="$tmp/$role-$round.json"
        fixture "$file" "$time"
        inputs+=("$file")
    done
done
report() {
    jq -s --argjson rounds 3 --argjson dry_run "$1" \
        --arg reference ref --arg candidate "$2" -f "$here/benchmark-report.jq" \
        "${inputs[@]}" > "$tmp/report.json"
}
report false cand
jq -e '.workloads[0] | .reference_ms == 20 and .candidate_ms == 2 and
  .reference_range_ms == [10,30] and .candidate_range_ms == [1,3] and
  (.time_reduction_percent - 90 | fabs < 0.0001) and .speedup == 10 and
  .interpretation == "observed reduction"' "$tmp/report.json" >/dev/null
jq -r --arg min_time 1s --argjson repetitions 10 -f "$here/benchmark-markdown.jq" \
    "$tmp/report.json" > "$tmp/report.md"
grep -q '90%' "$tmp/report.md"
report false ref
jq -e '.repeatability_only and .workloads[0].interpretation == "repeatability only"' \
    "$tmp/report.json" >/dev/null
report true cand
jq -e '.dry_run and .workloads[0].interpretation == "smoke test only"' \
    "$tmp/report.json" >/dev/null
fixture "$tmp/candidate-1.json" 15
fixture "$tmp/candidate-2.json" 20
fixture "$tmp/candidate-3.json" 25
report false cand
jq -e '.workloads[0].interpretation == "ranges overlap; inconclusive"' \
    "$tmp/report.json" >/dev/null

jq -s --argjson rounds 2 --argjson dry_run false --arg reference ref --arg candidate cand \
    -f "$here/benchmark-report.jq" "$tmp/reference-1.json" "$tmp/reference-2.json" \
    "$tmp/candidate-1.json" "$tmp/candidate-2.json" > "$tmp/even.json"
jq -e '.workloads[0].reference_ms == 15 and .workloads[0].candidate_ms == 17.5' \
    "$tmp/even.json" >/dev/null

fixture "$tmp/candidate-1.json" 0
if report false cand 2> "$tmp/error.log"; then
    printf 'Expected zero-time validation to fail\n' >&2; exit 1
fi
jq '.groups.scan[0].benchmark = "different/10"' "$tmp/reference-1.json" > "$tmp/candidate-1.json"
if report false cand 2> "$tmp/error.log"; then
    printf 'Expected mismatched workloads to fail\n' >&2; exit 1
fi
printf 'Benchmark report tests passed\n'
