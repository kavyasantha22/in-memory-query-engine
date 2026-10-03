# QueryEngine

A small C++20 query engine with filtering, projection, aggregation, grouping,
ordering, limits, correctness tests, and performance benchmarks.

The `benchmark` branch is the reference implementation for performance
comparisons. Create optimization branches from it and compare them with a
fresh local baseline. The branch preserves the code used for comparison;
timing files under `build/` are machine-specific and are not committed.

## Project layout

```text
include/query_engine/  Public library headers
src/                   Query engine implementation
examples/              Example programs
tests/                 Correctness tests
benchmarks/            Google Benchmark workloads and reporting
docs/                  Design, build, benchmark, and roadmap documentation
```

## Build and test

Read the [engine design and workflow](docs/engine-design.md) for the execution
stages, design tradeoffs, advantages, and current limitations.

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

## Benchmarks

```sh
cmake --preset benchmark
cmake --build --preset benchmark --target run_general_benchmarks
```

See the [benchmark guide](docs/benchmarks.md),
[result interpretation guide](docs/benchmark-interpretation.md), and
[CMake guide](docs/cmake.md) for details.

The [branch comparison workflow](docs/benchmarks.md#benchmark-branch)
explains how to capture the reference results and compare an optimization.
