def rounded: (. * 1000 | round) / 1000 | tostring;
def timing($value; $range):
  "\($value | rounded) (\($range[0] | rounded)-\($range[1] | rounded))";

"# Checkpoint Comparison\n",
"- Reference: `\(.reference.revision)`",
"- Candidate: `\(.candidate.revision)`",
"- Settings: \(.rounds) alternating rounds, \($repetitions) repetitions, \($min_time) minimum time, random interleaving.",
"- Statistic: median of per-round median wall times; ranges are minimum-maximum round medians, not confidence intervals.",
"- Environment and exact commands: see role metadata, compiler/flags files, and per-round command files. Raw JSON and logs accompany this report.\n",
(if .dry_run then "**Smoke test only: do not use these timings as performance evidence.**\n"
 elif .repeatability_only then "**Same revision: differences measure repeatability, not a code improvement.**\n"
 else "Measurements share a non-isolated machine. Observed reductions/slowdowns are not formal significance tests.\n" end),
"| Workload | Reference ms (range) | Candidate ms (range) | Time Reduction | Speedup | Interpretation |",
"| --- | ---: | ---: | ---: | ---: | --- |",
(.workloads[] |
  "| `\(.benchmark)` | \(timing(.reference_ms; .reference_range_ms)) | \(timing(.candidate_ms; .candidate_range_ms)) | \(.time_reduction_percent | rounded)% | \(.speedup | rounded)x | \(.interpretation) |"),
"\nPositive time reduction means less time; negative means slower. Speedup = reference / candidate. Overlapping ranges are flagged conservatively, not treated as statistical proof.",
"\nResults are not promoted automatically. Compare the owning revisions and conditions before selecting a best-checkpoint reference."
