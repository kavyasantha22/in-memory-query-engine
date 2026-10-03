# Interpreting Benchmark Results

This guide explains what the query engine's benchmark output means and how to
use it to choose an optimization target. A benchmark measures a controlled
workload. It does not, by itself, prove why the workload is slow.

## Start with the general report
  cmake --build --preset benchmark --target run_general_benchmarks
Run:

```sh
cmake --build --preset benchmark --target run_general_benchmarks
```

The grouped summary is the first report to read. It contains representative
projection, filtering, aggregation, grouping, ordering, complete-query, and
insertion cases. Use the full suite only after the general report identifies
an area that needs investigation.

The report displays columns similar to:

```text
Case                           Rows        Time     Throughput  vs baseline
Three columns             1,000,000   142.30 ms       7.03 M/s        +8.4%
```

- `Rows` is the logical input size, also called `N` in complexity analysis.
- `Time` is the median elapsed wall-clock time for one benchmark iteration.
- `Throughput` is the number of input rows processed per second.
- `vs baseline` compares median wall time with `general-baseline.json`.

A positive time delta means the candidate is slower. A negative time delta
means it is faster. Do not treat a small delta as a regression until repeated
runs show the same direction under similar machine conditions.

## Wall time and CPU time

Google Benchmark normally reports two timing columns:

```text
Benchmark                 Time          CPU
bmAggregateSum       4.82 ms      4.79 ms
```

`Time` is elapsed wall-clock time observed outside the process. It includes
time during which the operating system pauses the benchmark.

`CPU` is processor time consumed by the benchmark process. For a single-threaded,
CPU-bound operation, wall and CPU time should usually be close. A large gap can
indicate scheduling interruptions, waiting, blocking, or other system activity.

The general report emphasizes median wall time because it reflects how long a
caller waits. Its JSON also retains median CPU time.

## Iterations and calibration

The `Iterations` column is not the table row count. It is how many times Google
Benchmark repeated the timed body during one measurement:

```cpp
for (auto _ : state) {
    query_engine::ResultTable result = query_engine::queryTable(table, query);
}
```

Fast operations need many iterations to produce a stable timing interval.
Slow operations need fewer. Google Benchmark calibrates this automatically.

The input row count comes from:

```cpp
state.range(0)
```

and appears after the benchmark name:

```text
bmProjectionOneColumn/1000000
```

This means one benchmark iteration processes a table containing one million
rows. It does not mean the benchmark loop runs one million times.

## Throughput

The suite calls:

```cpp
state.SetItemsProcessed(state.iterations() * rowCount);
```

Google Benchmark uses this to calculate `items_per_second`:

```text
items_per_second = total rows processed / measured time
```

Throughput is useful when comparing the same logical operation before and
after a change:

```text
Before: 7.0 million rows/second
After:  8.4 million rows/second
```

That is a meaningful improvement. Comparing throughput between unrelated
operations, such as insertion and sorting, is usually not meaningful because
each "item" requires different work.

## Repetitions and aggregate statistics

The benchmark targets run each case several times. Google Benchmark derives
statistics such as:

- `mean`: arithmetic average of the repetitions.
- `median`: middle result after sorting the repetitions.
- `stddev`: absolute spread around the mean.
- `cv`: coefficient of variation, which is standard deviation divided by the
  mean and reported as a percentage.

The general report uses the median because one unusually interrupted run has
less influence on it than on the mean.

A high coefficient of variation means the benchmark is noisy. Before changing
code in response, reduce background load, repeat the run, and check whether the
same result persists.

## Big-O notation

Big-O describes how runtime grows as input size `N` grows. It does not describe
the exact duration of one run.

Common models in this suite are:

```text
O(1)        constant work
O(log N)    logarithmic growth
O(N)        linear scan
O(N log N)  comparison sorting
O(N^2)      nested or repeated linear searches
```

For a linear benchmark, Google Benchmark fits the measured data to:

```text
T(N) = c * N
```

For sorting it fits:

```text
T(N) = c * N * log(N)
```

The benchmark supplies `N` using:

```cpp
state.SetComplexityN(rowCount);
```

and selects the expected model during registration:

```cpp
->Complexity(benchmark::oN)
```

The model is fitted after measurements have been collected. Selecting `oN`
does not change the workload or force the measured result to be linear.

## The BigO row

Output such as:

```text
bmLimitTen_BigO    84.97 N    84.96 N
```

reports the fitted coefficient `c`. Google Benchmark records this coefficient
in nanoseconds in the JSON output for this suite. The fitted wall-time model is
approximately:

```text
T(N) = 84.97 nanoseconds * N
```

It predicts approximately:

```text
N = 10,000       0.85 ms
N = 100,000      8.50 ms
N = 1,000,000   84.97 ms
```

The first coefficient is based on wall time and the second on CPU time. The
`N` printed beside them identifies the linear model; it is not a time unit.

The coefficient is useful for comparing the same benchmark across changes.
Do not compare coefficients from different complexity models as though they
were equivalent units.

## RMS fitting error

The corresponding row may be:

```text
bmLimitTen_RMS    0 %    0 %
```

RMS means root-mean-square error. Google Benchmark computes the differences
between measured times and the times predicted by the selected complexity
model, combines those differences, and normalizes the result relative to the
mean measured time.

Intuitively:

```text
low RMS   measured growth follows the selected model closely
high RMS  measured growth does not follow the selected model closely
```

`0%` is rounded output and does not necessarily mean a mathematically perfect
fit. RMS should be treated as a diagnostic rather than a pass/fail score.

A high RMS can mean:

- The selected model is wrong.
- The tested sizes are too small to show asymptotic behavior.
- Cache boundaries or allocations change between sizes.
- The machine was under inconsistent load.
- Too few input sizes were measured.

Do not change an algorithm merely to reduce RMS. First inspect the raw timing
rows and decide whether the growth pattern itself is a problem.

## Why LIMIT can still be linear

`LIMIT 10` controls the number of output rows. It does not automatically limit
how many input rows must be examined.

If projection, filtering, or sorting processes all `N` input rows before the
limit is applied, the complete query can still be `O(N)` or `O(N log N)`.
Therefore a linear `bmLimitTen` result is not contradictory: only its output
size is bounded by ten.

## Baseline comparisons

Create a local baseline with:

```sh
cmake --build --preset benchmark --target save_general_benchmark_baseline
```

After changing the engine, run:

```sh
cmake --build --preset benchmark --target run_general_benchmarks
```

The summary calculates:

```text
time change = (candidate time - baseline time) / baseline time * 100
```

For time:

```text
+10% means slower
-10% means faster
```

For throughput:

```text
+10% means more rows per second
-10% means fewer rows per second
```

Only compare runs from the same machine and build configuration. Temperature,
power mode, background applications, and operating-system scheduling can all
change local benchmark results.

## Choosing what to improve

Use this sequence:

1. Start with a complete-query benchmark that resembles intended usage.
2. Compare it with a stable baseline and identify a repeatable regression or
   an unacceptably expensive workload.
3. Use the related operator benchmarks to narrow the problem to projection,
   filtering, aggregation, grouping, ordering, or allocation.
4. Use a profiler to identify the functions and instructions consuming time.
5. Make one focused change and rerun both the operator and complete-query cases.

Benchmark duration cannot explain the cause by itself. For example, a slow
grouping benchmark might be dominated by vector copies, key comparison, group
searching, or allocation. A profiler is what distinguishes those causes.

Avoid selecting work solely because it is the slowest row in the report.
Sorting is naturally more expensive than counting. Optimization priority
depends on common workloads, scaling behavior, regressions, and user-visible
impact.

## Reading the JSON files

The reporting workflow produces:

```text
general-latest.json   concise grouped medians and baseline changes
general-baseline.json saved local general-suite reference
latest.json           complete normal-suite results and repetitions
stress-latest.json    extreme-size benchmark results
```

Use `general-latest.json` for orientation and baseline changes. Use
`latest.json` when inspecting individual repetitions, mean, median, standard
deviation, coefficient of variation, scaling across sizes, Big-O, and RMS.

## Apple Silicon warnings

Google Benchmark may be unable to read `hw.cpufrequency` or set thread
affinity on Apple Silicon. Those warnings affect system metadata and frequency
estimation. They do not invalidate the measured wall and CPU times.

The displayed estimated CPU frequency should not be used to explain a timing
change. Compare measured durations and throughput under similar conditions.
