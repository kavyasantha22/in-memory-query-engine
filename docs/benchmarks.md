# Benchmark Guide

The performance suite measures the public query engine APIs with Google
Benchmark. It is split into a normal suite for regular development and an
opt-in stress suite for high-memory workloads.

The suite measures current public API behavior. Query execution borrows the
table and query, constructs row selections, and returns an owning result.
Direct aggregation uses a wrapper selection prepared outside timing.
See [optimization history](optimization-history.md) for measured checkpoints.

For an explanation of why the benchmark code is structured this way and a
guided tour of the C++ features it uses, see
[Benchmark Design and C++ Features](benchmark-design.md).

For guidance on reading timings, throughput, Big-O coefficients, RMS errors,
and baseline changes, see [Interpreting Benchmark Results](benchmark-interpretation.md).

## Baseline branch

`main` is the development branch. The separate `baseline` branch preserves the
reference implementation for performance comparisons. A branch identifies
source code; `general-baseline.json` contains machine-specific measurements.
Checking out a branch does not create or restore that ignored timing file.

To compare changes on `main`, create a reference worktree from the project root:

```sh
git worktree add ../QueryEngine-baseline baseline
cd ../QueryEngine-baseline
cmake --preset benchmark
cmake --build --preset benchmark --target save_general_benchmark_baseline
```

Return to the development checkout, build it, and copy the reference timings:

```sh
cd ../QueryEngine
cmake --preset benchmark
cmake --build --preset benchmark
mkdir -p build/benchmark-clang/benchmark-results
cp ../QueryEngine-baseline/build/benchmark-clang/benchmark-results/general-baseline.json \
   build/benchmark-clang/benchmark-results/general-baseline.json
./build/benchmark-clang/query_engine_general_benchmark \
  --general_summary=build/benchmark-clang/benchmark-results/general-vs-original.json \
  --general_baseline=build/benchmark-clang/benchmark-results/general-baseline.json \
  --benchmark_min_time=0.1s --benchmark_repetitions=5 \
  --benchmark_enable_random_interleaving --benchmark_report_aggregates_only=true
```

These paths assume the development checkout is named `QueryEngine`; adjust
them if your checkout has a different name. Create the reference worktree once
and reuse it for later measurements.

This explicit run compares against the original reference implementation.
The normal `run_general_benchmarks` target instead uses `general-best.json`.
Running `save_general_benchmark_baseline` deliberately captures the current
checkout into `general-baseline.json`, without changing the best checkpoint or
latest report. Use the baseline worktree when capturing the original reference.

Use identical workload definitions, compiler and build settings, and the same
machine under similar conditions. For small changes, repeat and alternate
reference and candidate measurements. Record both commit IDs with results you
retain for later comparison.

## Build

Configure and build the optimized benchmark targets:

```sh
cmake --preset benchmark
cmake --build --preset benchmark
```

The executables are:

```text
build/benchmark-clang/query_engine_benchmark
build/benchmark-clang/query_engine_general_benchmark
build/benchmark-clang/query_engine_stress_benchmark
```

The benchmark preset uses Apple Clang and CMake's Release configuration. On
the current toolchain, Release applies `-O3 -DNDEBUG`.

## General suite

Start with the curated general report:

```sh
cmake --build --preset benchmark --target run_general_benchmarks
```

It runs 18 representative cases at fixed input sizes and prints a grouped
table for projection, filtering, aggregation, grouping, ordering, complete
queries, and insertion. It writes the concise report to:

```text
build/benchmark-clang/benchmark-results/general-latest.json
```

`general-latest.json` is the concise report intended for regular reading. It
contains one median result per case, grouped by engine stage. Run the normal
suite when individual repetitions, aggregate statistics, and complexity fits
are needed.

The representative sizes are 1M rows for scans, aggregation, grouping,
complete queries, and insertion, and 100K rows for ordering. These sizes make
the cases large enough to expose meaningful costs without turning the general
report into a stress test.

The default comparison reference is:

```text
build/benchmark-clang/benchmark-results/general-best.json
```

If it does not exist, the reporter explicitly says no checkpoint comparison is
available and produces measurements with null comparison fields. A missing
reference is not evidence of an improvement.

After reviewing results and correctness, explicitly promote an archived,
revision-identified report (substitute the checkpoint you reviewed):

```sh
cp build/benchmark-clang/benchmark-results/checkpoints/dc9bc85/summary.json \
   build/benchmark-clang/benchmark-results/general-best.json
cp build/benchmark-clang/benchmark-results/checkpoints/dc9bc85/metadata.json \
   build/benchmark-clang/benchmark-results/general-best.metadata.json
```

The copy commands above are promotion, not part of a normal run. Keep the
original `general-baseline.json` unchanged. The metadata companion identifies
the measured revision and environment; the benchmark reporter reads only the
timing JSON. Do not promote a mix of fastest cases from different revisions.

Later general runs automatically compare matching cases with the best file. The
console and JSON summary then show percentage changes. A positive time change
means slower; a negative time change means faster. For throughput, the meaning
is reversed: positive means more rows processed per second.

Archive `general-latest.json` before another run overwrites it. Re-measure the
best revision under comparable conditions when compiler, machine, workload, or
system state changes. "Best" is your reviewed checkpoint, not an automated
winner across workloads. Raw per-repetition JSON can also be requested using
`--benchmark_out=<path> --benchmark_out_format=json`; omit
`--benchmark_report_aggregates_only=true` when collecting it.

## Normal suite

Run the complete normal suite:

```sh
cmake --build --preset benchmark --target run_benchmarks
```

This target uses five repetitions, random repetition interleaving, a
0.1-second minimum measurement time, and aggregate-only console display. The
complete JSON result, including individual repetitions, is written to:

```text
build/benchmark-clang/benchmark-results/latest.json
```

The normal suite includes:

- One-, three-, and six-column projections over 10K, 100K, and 1M rows.
- Filters with 0%, 1%, 10%, 50%, and 100% selectivity.
- Zero-row and ten-row limits.
- Visible, hidden, single-column, and multi-column ordering.
- ORDER BY followed by LIMIT.
- Global SUM and grouped SUM queries with 10, 100, and multi-column keys.
- High-cardinality transaction grouping at 1K and 10K rows.
- Composed filtering, grouping, AVG, ordering, and limiting.
- Direct COUNT, SUM, AVG, MIN, and MAX aggregation.
- Reserved and unreserved batch insertion.

## Stress suite

Run the opt-in stress suite:

```sh
cmake --build --preset benchmark --target run_stress_benchmarks
```

It runs three repetitions and writes:

```text
build/benchmark-clang/benchmark-results/stress-latest.json
```

The stress suite uses 10 million rows for projection, 10%-selectivity
filtering, direct SUM, and reserved and unreserved insertion. Sorting and
high-cardinality grouping are intentionally excluded at this size because
their current memory and runtime costs are not bounded enough for a reliable
local workflow.

## Standardized paired measurements

Use the orchestration script to generate evidence for Markdown documentation:

```sh
bash scripts/benchmark-checkpoints.sh --reference baseline --candidate HEAD
```

For the next improvement, compare against the nominated checkpoint instead:

```sh
bash scripts/benchmark-checkpoints.sh --candidate HEAD
```

The second command requires `general-best.metadata.json` with its `revision`.
An explicit `--reference <commit-or-tag>` overrides that choice. Both revisions
are resolved to commits before execution; working-tree edits are not measured.
Commit candidate implementation changes before measuring them. Equal revisions
are permitted for calibration, but the report labels them repeatability checks,
not improvements.

The script requires Bash, Git, CMake, and jq. Bash orchestrates processes and jq
parses structured JSON; timed engine work still runs in the existing C++
executables. Each revision uses its own detached worktree and Release build.
Compiler versions and engine flags must match. Cached dependency sources are
reused when committed CMake declarations match; otherwise CMake may need network
access. Builds finish before sequential measurements start.

Defaults are three alternating paired rounds, ten repetitions, and a 1-second
minimum per repetition. This can take 20 minutes or more. Stay on AC power and
avoid concurrent heavy tasks. These settings reduce noise, not eliminate it.
Change them explicitly when needed:

```sh
bash scripts/benchmark-checkpoints.sh --reference dc9bc85 --candidate HEAD \
  --rounds 3 --repetitions 10 --min-time 1s --jobs 4
```

`--jobs` controls builds only. Results go into a new timestamped directory under
`build/benchmark-clang/benchmark-results/comparisons/`; `--output <new-directory>`
overrides it. Existing directories are rejected to prevent accidental overwrite.

The archive contains:
- `comparison.md`: the documentation-ready table and interpretation caveats.
- `comparison.json`: full-precision medians, ranges, percentage reductions, and speedups.
- `reference/` and `candidate/`: three round summaries, raw repetitions, logs,
  exact command files, compiler/flags files, metadata, and `summary.json`.

The table uses the median of round medians and their minimum-maximum range.
Positive time reduction means faster (unlike the reporter's positive time-change
convention). Ranges that overlap are labeled inconclusive, conservatively; this
is not a formal significance test. Workload identities must match, and benchmark
errors abort the run. Copy the report's relevant table and provenance into
[optimization history](optimization-history.md), retaining its raw archive.
Benchmark definitions should still be reviewed for semantic equivalence across
revisions; matching names alone cannot guarantee equivalent workloads.

Successful runs remove only the worktrees they created. Failed/interrupted runs
retain worktrees and logs for investigation. Baseline/best files and Git tags
are never changed. To promote reviewed results, use the earlier explicit copy
commands with `candidate/summary.json` and `candidate/metadata.json` instead of
the historical checkpoint paths.

For a fast setup check (not performance evidence):

```sh
bash scripts/benchmark-checkpoints.sh --reference baseline --candidate HEAD \
  --rounds 1 --repetitions 2 --dry-run
```

Report calculation/validation tests can be run separately, without builds or
measurements:

```sh
bash scripts/test-benchmark-report.sh
```

## Focused runs

List all registered normal benchmarks:

```sh
./build/benchmark-clang/query_engine_benchmark --benchmark_list_tests
```

Run one benchmark family:

```sh
./build/benchmark-clang/query_engine_benchmark \
  --benchmark_filter='bmProjectionOneColumn'
```

Execute every benchmark body once without calibration:

```sh
./build/benchmark-clang/query_engine_benchmark --benchmark_dry_run
```

Every workload reports milliseconds and processed rows per second. Scaling
families also report fitted complexity and RMS error.

## Local baselines

Benchmark results depend on the machine, power mode, temperature, and current
system load. Keep baseline files local and compare runs made on the same Mac
under similar conditions.

Create a baseline:

```sh
cmake --build --preset benchmark --target run_benchmarks
cp build/benchmark-clang/benchmark-results/latest.json \
   build/benchmark-clang/benchmark-results/baseline.json
```

After making a code change, create the candidate result:

```sh
cmake --build --preset benchmark --target run_benchmarks
cp build/benchmark-clang/benchmark-results/latest.json \
   build/benchmark-clang/benchmark-results/candidate.json
```

Google Benchmark's comparison utility requires its Python dependencies. Install
them in a virtual environment rather than into the system Python:

```sh
python3 -m venv .venv
.venv/bin/pip install -r \
  build/benchmark-clang/_deps/google_benchmark-src/tools/requirements.txt
```

Compare the two JSON files:

```sh
.venv/bin/python \
  build/benchmark-clang/_deps/google_benchmark-src/tools/compare.py \
  benchmarks \
  build/benchmark-clang/benchmark-results/baseline.json \
  build/benchmark-clang/benchmark-results/candidate.json \
  --display_aggregates_only
```

Treat small differences as noise unless they repeat across multiple runs. A
change should only be called a regression when the affected benchmark is
stable, the difference is material, and the comparison was made under similar
system conditions.

## Apple Silicon metadata

Google Benchmark may report that it cannot read `hw.cpufrequency` or set thread
affinity on Apple Silicon. These warnings affect CPU metadata, not the measured
wall and CPU times. The reported `24 MHz` value should not be used when
interpreting results.
