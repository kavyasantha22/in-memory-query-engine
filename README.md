# QueryEngine

A small C++20 query engine with filtering, projection, aggregation, grouping,
ordering, limits, correctness tests, and performance benchmarks.

The `main` branch is the development branch. The separate `baseline` branch
preserves the reference implementation for performance comparisons. Benchmark
results under `build/` are local measurements and are not committed.

Read the [engine design and workflow](docs/engine-design.md) for execution
stages, design tradeoffs, advantages, and current limitations.

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

The [branch comparison workflow](docs/benchmarks.md#baseline-branch) explains
how to compare development changes with the reference implementation.
