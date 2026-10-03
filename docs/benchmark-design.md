# Benchmark Design and C++ Features

This document explains how the query engine benchmark suite is designed and
why its implementation uses particular C++ features. Comparisons with C focus
on features that standard C either does not provide or requires the programmer
to build manually.

The benchmark suite is not part of the query engine's production behavior. It
is a separate client of the public API, just like an application using the
engine would be.

## Source organization

The benchmark code is split by responsibility:

```text
benchmarks/
  benchmark_common.hpp
  query_benchmark.cpp
  aggregation_benchmark.cpp
  insert_benchmark.cpp
  general_benchmark_main.cpp
  stress_benchmark.cpp
```

The normal executable links the query, aggregation, and insertion translation
units. The general executable links those same benchmark translation units plus
its custom `main` and reporter. The stress source is linked into a separate
executable so a normal run cannot accidentally allocate the memory required by
the 10-million-row cases.

A **translation unit** is roughly one `.cpp` file after the preprocessor has
expanded all of its `#include` directives. Each benchmark source is compiled
independently and then linked into the final executable.

C has the same separate compilation model, but C++ adds features such as
namespaces, templates, overloaded functions, constructors, and destructors
that the linker and compiler must coordinate across those translation units.

## Why the helpers are in a header

The subsystem benchmarks share setup and measurement code from
`benchmark_common.hpp`. For example:

```cpp
inline void recordRows(benchmark::State& state, std::int64_t rowCount){
    state.SetItemsProcessed(state.iterations() * rowCount);
    state.SetComplexityN(rowCount);
}
```

Because this definition is included by several `.cpp` files, it is declared
`inline`. In C++, `inline` allows equivalent function definitions to appear in
multiple translation units without violating the One Definition Rule. The
linker treats them as one logical function.

`inline` does not guarantee that the optimizer will replace a function call
with the function body. The compiler makes that optimization decision
independently.

C also has an `inline` keyword, but its linkage rules differ and are easier to
misconfigure. A common C alternative is a `static inline` function in a header,
which gives each translation unit its own private definition.

## Namespaces

The production library uses `namespace query_engine`. Benchmark clients refer
to its types and functions explicitly, for example `query_engine::Query` and
`query_engine::queryTable()`. Library type-definition excerpts in this guide
are understood to be inside that namespace. Header include paths and CMake
target names are independent of C++ namespaces and retain their existing names.

Shared helpers live in a namespace:

```cpp
namespace benchmarkSupport {
    // Shared benchmark helpers
}
```

The full function name is therefore:

```cpp
benchmarkSupport::runQuery
```

Namespaces prevent unrelated libraries from defining conflicting global names.
The `::` operator selects a name inside a namespace.

Standard C does not have namespaces. C libraries normally approximate them
with name prefixes:

```c
void benchmark_support_run_query(...);
```

The unnamed namespace used in each `.cpp` file has a different purpose:

```cpp
namespace {
    void bmAggregateSum(benchmark::State& state){ /* ... */ }
}
```

Names in an unnamed namespace have internal linkage and cannot conflict with
names in another translation unit. In C, file-local functions are normally
declared with `static`.

## References instead of pointers

Benchmark functions receive the framework state by reference:

```cpp
void bmAggregateSum(benchmark::State& state)
```

`benchmark::State&` is a C++ reference. It aliases an existing object and uses
normal member access:

```cpp
state.range(0);
```

A reference is expected to refer to a valid object and cannot normally be
reseated after initialization. It also avoids copying the state object.

C has pointers instead of references. An equivalent C interface would look
more like:

```c
void bm_aggregate_sum(struct benchmark_state* state);
```

and member access would use `state->field`. A C pointer can be null, can be
reassigned, and requires explicit pointer syntax. C++ also supports pointers;
references are used when non-null aliasing better expresses the contract.

## Objects and member functions

`benchmark::State` and `benchmark::Benchmark` are objects with member
functions:

```cpp
state.SetItemsProcessed(...);
benchmark->Arg(10'000)->Unit(benchmark::kMillisecond);
```

The second expression demonstrates method chaining. `Arg` returns a pointer to
the same benchmark registration object, allowing another member function to be
called immediately.

C has structures but no built-in member functions or access control. A C API
would usually pass a structure pointer to standalone functions:

```c
benchmark_set_items_processed(&state, count);
benchmark_add_argument(registration, 10000);
```

## Templates and standard containers

The engine and benchmark code use template-based standard-library types:

```cpp
std::vector<query_engine::ColumnName>
std::vector<query_engine::Row>
std::optional<query_engine::Aggregation>
std::function<bool(query_engine::Row)>
```

A template describes a family of types or functions. `std::vector<Row>` and
`std::vector<ColumnName>` are separate concrete types generated from the same
`std::vector<T>` template.

`std::vector<T>` owns a dynamically sized contiguous array. It tracks its
pointer, size, and capacity, grows automatically, copies or moves its elements,
and releases its allocation in its destructor.

Standard C has no template system or standard dynamic-array container. A C
implementation would need a custom structure and functions, often repeated or
generated for each element type:

```c
struct row_vector {
    struct row* data;
    size_t size;
    size_t capacity;
};
```

The C programmer must call `malloc`, `realloc`, and `free` correctly and define
the ownership rules.

## RAII and deterministic destruction

RAII means **Resource Acquisition Is Initialization**. A C++ object owns a
resource and releases it automatically when the object leaves scope.

The insertion helper deliberately uses a nested scope:

```cpp
state.PauseTiming();
{
    query_engine::Table table = query_engine::generateTable(0);
    state.ResumeTiming();

    // Timed insertions

    state.PauseTiming();
} // table and its vectors are destroyed while timing is paused
state.ResumeTiming();
```

When execution reaches the closing brace, `table` is destroyed. Its
`std::vector` members release their memory automatically. The scope is placed
while timing is paused so cleanup is not accidentally counted as insertion
work.

C does not have destructors or automatic resource-owning containers. Cleanup
would need an explicit call on every exit path:

```c
table_destroy(&table);
```

This is one reason C programs often use `goto cleanup` to centralize resource
release after an error.

## Automatic type deduction and range-based loops

The timed loop is written as:

```cpp
for (auto _ : state){
    query_engine::ResultTable result = query_engine::queryTable(table, query);
    benchmark::DoNotOptimize(result);
}
```

`auto` asks the compiler to deduce the variable's type from its initializer.
The range-based `for` loop asks the `state` object for an iterator range and
runs once per benchmark iteration selected by Google Benchmark.

The `_` variable is not special C++ syntax. It is an ordinary variable name
chosen because the value is intentionally unused.

Traditional C has neither `auto` type deduction nor range-based loops. A C
benchmark framework would expose an iteration count and use an indexed loop:

```c
for (size_t i = 0; i < state.iterations; ++i) {
    /* measured operation */
}
```

Modern C uses `auto` as a storage-class keyword, not C++-style type deduction.

## Lambdas and callable objects

Filter selectivity is expressed with lambdas:

```cpp
[](query_engine::Row row){ return row.transaction_id % 100 == 0; }
```

A lambda creates an unnamed function object. The empty capture list `[]` means
it does not capture local variables. A capturing lambda could retain values
from its surrounding scope.

The query stores filters in:

```cpp
std::function<bool(query_engine::Row)>
```

`std::function` uses **type erasure**: it can hold different callable types
behind one uniform function-call interface. It can store a regular function,
a lambda, or a class implementing `operator()`.

Standard C has function pointers but no lambdas or closures. Passing state to a
callback normally requires a separate `void*` context pointer:

```c
typedef bool (*row_filter)(struct row row, void* context);
```

`std::function` is more flexible but can add indirect-call and, for some
callables, allocation overhead. That overhead is part of the query's current
filtering behavior and is intentionally measured.

## Strong enums

Columns and aggregation types use scoped enumerations:

```cpp
query_engine::ColumnName::PRICE
query_engine::AggregationType::SUM
```

An `enum class` keeps enumerator names inside the enum's scope and does not
implicitly convert to an integer. This prevents accidental mixing of unrelated
enumerations.

C enums are unscoped integer-like constants. Prefixes are commonly used to
avoid collisions:

```c
enum column_name {
    COLUMN_NAME_PRICE
};
```

## Optional values

A query clause that may be absent uses `std::optional<T>`:

```cpp
std::optional<query_engine::Aggregation> aggregation;
std::optional<std::size_t> limit;
```

`std::nullopt` represents absence. An optional stores either no value or one
fully constructed `T` value without requiring a sentinel number.

C has no standard optional-value type. A typical C representation uses a flag:

```c
struct optional_limit {
    bool has_value;
    size_t value;
};
```

## Variants and tagged unions

The engine's result values use:

```cpp
using ResultValue = std::variant<
    std::uint64_t,
    std::uint32_t,
    double,
    std::int64_t
>;
```

`std::variant` is a type-safe tagged union. It records which alternative is
active, constructs and destroys that value correctly, and rejects access using
the wrong type.

C provides unions but not automatic tagging:

```c
struct result_value {
    enum result_type type;
    union {
        uint64_t u64;
        uint32_t u32;
        double f64;
        int64_t i64;
    } data;
};
```

In C, every operation must manually keep the enum tag synchronized with the
union member.

## Designated initialization

Queries use named fields:

```cpp
query_engine::Query query{
    .projection = {query_engine::ColumnName::PRICE},
    .filter = std::nullopt,
    .aggregation = std::nullopt,
    .group_by = std::nullopt,
    .order_by = std::nullopt,
    .limit = std::nullopt
};
```

This is C++20 designated initialization. It improves readability for aggregate
types with several fields and makes benchmark intent visible at the call site.

Unlike many features in this document, designated initialization is not unique
to C++. C has supported designated initializers since C99. C++20's version is
more restrictive: designators must follow declaration order and cannot use all
of C's nested or array-designator forms.

## `constexpr` and digit separators

The stress size is a compile-time constant:

```cpp
constexpr std::int64_t stressRowCount = 10'000'000;
```

`constexpr` states that a value can be evaluated at compile time. C++ also
supports `constexpr` functions and objects with increasingly rich compile-time
behavior.

The apostrophes are digit separators. They do not change the value; they make
large numbers easier to read. Digit separators entered C++ in C++14. C23 also
supports them, but older C standards do not.

Traditional C commonly uses macros or enum constants for compile-time values.
C23 added a `constexpr` facility, but it is newer and not equivalent to the
full C++ constant-expression system.

## Explicit conversions

Conversions that might lose information are written explicitly:

```cpp
static_cast<std::uint64_t>(rowCount)
static_cast<std::size_t>(state.range(0))
```

`static_cast<T>` documents the intended category of conversion and is checked
more narrowly than a C-style cast.

C uses cast syntax such as `(uint64_t)row_count`. C++ supports that syntax too,
but named casts make intent easier to search and prevent some unsafe
conversions.

## Pass-by-value is part of the measurement

The shared query runner receives `Query` by value:

```cpp
inline void runQuery(
    benchmark::State& state,
    query_engine::Query query,
    std::size_t expectedRows
)
```

More importantly, the production API currently receives both `Table` and
`Query` by value:

```cpp
ResultTable queryTable(Table table, Query query);
```

Copying a `Table` deep-copies its `std::vector<Row>`. The benchmark deliberately
does not change this to references because it is measuring the public API as a
caller experiences it today. If the production signature later changes to
`const Table&`, the benchmark should reveal that improvement.

C++ distinguishes copying from moving. Standard containers define both copy
and move operations, and returned local objects can also benefit from copy
elision. In C, structure assignment copies fields, while dynamically allocated
ownership normally needs custom copy and move-like conventions.

## Keeping the optimizer honest

An optimizer may remove work whose result is never observed. Benchmarks must
prevent that without adding unrelated I/O.

Query and aggregation results use:

```cpp
benchmark::DoNotOptimize(result);
```

Insertion additionally uses:

```cpp
benchmark::DoNotOptimize(table.rows.data());
benchmark::ClobberMemory();
```

`DoNotOptimize` tells the compiler that a value must be treated as observable.
`ClobberMemory` creates a compiler barrier for memory, preventing assumptions
that would incorrectly remove or reorder the writes being measured.

Printing the result would also make it observable, but terminal I/O is many
orders of magnitude slower and would measure formatting and output instead of
the query operation.

## Timed and untimed work

Query tables are generated before the timed loop:

```cpp
const query_engine::Table table = query_engine::generateTable(rowCount);

for (auto _ : state){
    query_engine::ResultTable result = query_engine::queryTable(table, query);
}
```

This measures query execution but not test-data generation.

Insertion needs a fresh empty table for every iteration. It explicitly pauses
timing for setup and cleanup, then resumes timing only around calls to
`insertRow`.

Separating setup is essential. Otherwise a benchmark named "insert" could
mostly measure table generation, allocation cleanup, or output formatting.

## Reserved and unreserved insertion

`std::vector` stores a capacity separately from its size. When insertion would
exceed capacity, the vector allocates a larger block and moves or copies its
elements.

The reserved benchmark calls:

```cpp
table.rows.reserve(rowCount);
```

before timing. This measures insertion when capacity planning is correct.

The unreserved benchmark allows normal geometric vector growth. Comparing both
shows the cost of allocation and relocation. C dynamic arrays require the same
capacity strategy, but the programmer implements it manually with `realloc` or
allocate-copy-free logic.

## Predictable data and selectivity

Every benchmark uses `generateTable` rather than random input. The generated
columns have stable cardinalities:

- Category has 10 distinct values.
- Product has 100 distinct values.
- Transaction ID is unique.

These properties produce controlled grouping workloads. Filters use arithmetic
predicates that deterministically select 0%, 1%, 10%, 50%, or 100% of rows.

The timestamp base is taken from the current system clock, so the complete
dataset is not identical across runs. Current benchmark queries do not filter,
group, or sort by timestamp. Fully repeatable timestamps remain a roadmap item
before adding workloads whose behavior depends on their absolute values.

Determinism matters because input changes can affect branch prediction, group
sizes, comparison counts, and memory use. A baseline should differ because the
code changed, not because a new random dataset happened to be easier.

## Complexity reporting

Each scalable benchmark records its input size:

```cpp
state.SetComplexityN(rowCount);
```

The registration declares the expected model:

```cpp
->Complexity(benchmark::oN)
->Complexity(benchmark::oNLogN)
->Complexity(benchmark::oNSquared)
```

Google Benchmark fits measured values to that model and reports a coefficient
and RMS error. The intended models are:

- Projection, filtering, aggregation, and insertion: `O(n)`.
- Comparison sorting: `O(n log n)`.
- Current unique-key grouping: `O(n^2)` because each new key scans existing
  groups.

Complexity describes growth as input becomes large. It does not promise exact
10x timing for a 10x input because cache size, allocation behavior, memory
bandwidth, and thermal conditions change the constant factors.

## Static benchmark registration

This statement registers a function with Google Benchmark:

```cpp
BENCHMARK(bmAggregateSum)
    ->Apply(benchmarkSupport::addStandardRowCounts)
    ->Unit(benchmark::kMillisecond)
    ->Complexity(benchmark::oN);
```

`BENCHMARK` is a preprocessor macro. It creates static registration code that
runs before `main`. Because all benchmark `.cpp` files are linked into one
executable, their registrations are collected into one global registry.

The executable links `benchmark::benchmark_main`, which supplies `main()` and
runs everything in that registry. Defining `BENCHMARK_MAIN()` in these source
files would create a duplicate `main` definition.

Both C and C++ have preprocessor macros, so the macro itself is not a C++-only
feature. The objects, namespaces, overloaded registration API, and static
construction used behind it are C++ mechanisms.

## Why normal and stress suites are separate

The normal suite uses sizes chosen for repeated local runs. Sorting is capped
at 100K rows, and quadratic grouping is capped at 10K rows.

The stress executable contains only operations known to stay bounded at 10
million rows. This is a safety boundary, not merely organization. Running every
query shape at the largest size could exhaust memory or take an impractical
amount of time before useful results are produced.

## Repetitions, interleaving, and JSON

The normal CMake run target uses five repetitions and random interleaving.
Interleaving avoids running every repetition of one benchmark in a single time
window, reducing bias from gradual temperature or system-load changes.

Console output shows aggregate statistics, while JSON retains individual
repetitions for later comparison. Baselines are kept under `build/` rather than
committed because absolute timings are specific to the machine and its current
environment.

The general-report workflow adds a C++ presentation layer without changing the
measurements. `general_benchmark_main.cpp` owns the curated case list and builds
the Google Benchmark filter from that list. Keeping one list avoids maintaining
a CMake filter separately from report groups and labels.

The standard executable links `benchmark::benchmark_main`, which supplies a
generic `main()`. The general executable instead links `benchmark::benchmark`
and supplies its own `main()`. It initializes Google Benchmark, installs the
curated filter, and calls `RunSpecifiedBenchmarks()` with a custom reporter.

The reporter is a class derived from Google Benchmark's interface:

```cpp
class GeneralReporter : public benchmark::BenchmarkReporter {
public:
    bool ReportContext(const Context&) override;
    void ReportRuns(const std::vector<Run>& reports) override;
    void Finalize() override;
};
```

Inheritance and virtual functions are C++ features with no direct equivalent
in C. A C design would normally use a struct containing state plus function
pointers. Here, Google Benchmark calls the overridden methods through a base
`BenchmarkReporter*`, which is runtime polymorphism.

`ReportRuns()` receives completed measurements. The reporter keeps only the
median for each selected benchmark in an `std::unordered_map`. `Finalize()`
runs after all benchmark families finish; it loads an optional baseline,
calculates changes, writes grouped JSON, and prints the concise table. Because
all reporting happens after timed loops complete, formatting, file I/O, and
JSON work are not included in benchmark durations.

The implementation uses several C++ library facilities that do not exist as
equivalent standard C abstractions:

- `std::filesystem::path` for output and baseline paths.
- `std::optional` for missing baselines, throughput, and undefined deltas.
- `std::unordered_map` for measurements indexed by benchmark name.
- `std::string_view` for non-owning references to fixed metadata.
- `std::chrono` for the report timestamp.

C++20 does not provide a standard JSON parser or serializer. The reporter uses
the header-only `nlohmann/json` library rather than constructing or parsing
JSON with string manipulation. CMake fetches a pinned version so the dependency
does not change unexpectedly. This adds a dependency, but keeps baseline
parsing structured and makes the report schema straightforward to extend.

For an explanation of the resulting statistics, see
[Interpreting Benchmark Results](benchmark-interpretation.md).

## What the suite does not measure

The suite intentionally does not:

- Print or format result tables inside timed regions.
- Include generated test-data setup in query timings.
- Change production signatures to make benchmark results look faster.
- Treat Apple Silicon's unavailable fixed-frequency metadata as a timing
  failure.
- Enforce one machine's timing as a portable CI threshold.

These boundaries keep each benchmark interpretable: its name describes the
operation being measured, setup is controlled, and changes in results can be
traced back to current public query engine behavior.
