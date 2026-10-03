# Benchmark Guide

The performance suite measures the public query engine APIs with Google
Benchmark. It is split into a normal suite for regular development and an
opt-in stress suite for high-memory workloads.

The suite measures current behavior as implemented. In particular,
`queryTable`, `aggregate`, and several helper paths currently accept large
objects by value. Those copies are intentionally included in the results.

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
cmake --build --preset benchmark --target run_general_benchmarks
```

These paths assume the development checkout is named `QueryEngine`; adjust
them if your checkout has a different name. Create the reference worktree once
and reuse it for later measurements.

The development summary now compares against the reference implementation.
Running `save_general_benchmark_baseline` in the development checkout replaces
that comparison file with measurements of the development code. Use it there
only when deliberately establishing a new local reference.

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

To establish the current checkout as a new local timing reference, run:

```sh
cmake --build --preset benchmark --target save_general_benchmark_baseline
```

This runs the general suite and copies the summary to:

```text
build/benchmark-clang/benchmark-results/general-baseline.json
```

For comparisons against the `baseline` branch, capture this file in its
reference worktree and copy it here as described above.

Later general runs automatically compare matching cases with that file. The
console and JSON summary then show percentage changes. A positive time change
means slower; a negative time change means faster. For throughput, the meaning
is reversed: positive means more rows processed per second.

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
