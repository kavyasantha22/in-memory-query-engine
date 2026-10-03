def median:
  sort | length as $n |
  if $n % 2 == 1 then .[($n / 2 | floor)]
  else (.[($n / 2)-1] + .[$n / 2]) / 2 end;

def cases:
  [.groups | to_entries[] | .key as $group | .value[] |
    {group: $group, benchmark, rows, label, median}] | sort_by(.benchmark);

def identity: cases | map({group, benchmark, rows, label});

def valid:
  .schema_version == 1 and (cases | length > 0) and
  (cases | all(.[];
    (.median.real_time_ms | type == "number" and . > 0) and
    (.median.cpu_time_ms | type == "number" and . >= 0)));

def summarize($revision):
  . as $rounds |
  $rounds[0] | .baseline = null | .generated_at = (now | todateiso8601) |
  .revision = $revision |
  .summary_method = "median of per-round medians" |
  .groups |= with_entries(
    .key as $group |
    .value |= map(
      .benchmark as $name |
      [$rounds[] | .groups[$group][] | select(.benchmark == $name) | .median] as $values |
      .median |= with_entries(
        .key as $key |
        .value = ([$values[] | .[$key]] |
          if all(.[]; type == "number") then median else null end)) |
      .change_from_baseline_percent = null));

. as $documents |
if length != $rounds * 2 then error("Incorrect number of rounds") else . end |
if all(.[]; valid) then . else error("Invalid or empty summary measurements") end |
(.[0] | identity) as $expected |
if all(.[]; identity == $expected) and
   ($expected | map(.benchmark) | unique | length) == ($expected | length)
then . else error("Workload names, groups, sizes, or labels differ") end |
($documents[:$rounds] | summarize($reference)) as $reference_summary |
($documents[$rounds:] | summarize($candidate)) as $candidate_summary |
($reference_summary | cases) as $reference_cases |
($candidate_summary | cases) as $candidate_cases |
{
  schema_version: 1,
  generated_at: (now | todateiso8601),
  rounds: $rounds,
  dry_run: $dry_run,
  repeatability_only: ($reference == $candidate),
  reference: {revision: $reference, summary: $reference_summary},
  candidate: {revision: $candidate, summary: $candidate_summary},
  workloads: [range(0; $reference_cases | length) as $i |
    $reference_cases[$i] as $ref | $candidate_cases[$i] as $cand |
    [$documents[:$rounds][] | cases[] | select(.benchmark == $ref.benchmark) |
      .median.real_time_ms] as $ref_times |
    [$documents[$rounds:][] | cases[] | select(.benchmark == $ref.benchmark) |
      .median.real_time_ms] as $cand_times |
    {benchmark: $ref.benchmark, rows: $ref.rows, group: $ref.group,
      reference_ms: $ref.median.real_time_ms,
      candidate_ms: $cand.median.real_time_ms,
      reference_range_ms: [$ref_times | min, max],
      candidate_range_ms: [$cand_times | min, max],
      time_reduction_percent: ((1 - $cand.median.real_time_ms / $ref.median.real_time_ms) * 100),
      speedup: ($ref.median.real_time_ms / $cand.median.real_time_ms),
      interpretation: (
        if $dry_run then "smoke test only"
        elif $reference == $candidate then "repeatability only"
        elif $rounds < 2 then "single round; repeat measurements"
        elif ($ref_times | min) <= ($cand_times | max) and
             ($cand_times | min) <= ($ref_times | max) then "ranges overlap; inconclusive"
        elif $cand.median.real_time_ms < $ref.median.real_time_ms then "observed reduction"
        else "observed slowdown" end)}]
}
