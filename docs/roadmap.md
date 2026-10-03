# In-Memory Query Engine — Features and Roadmap

Status reviewed against the `main` branch on 3 October 2026.
Implemented behavior and remaining correctness coverage are listed separately.

Library declarations in this document are excerpts from `namespace query_engine`.
Caller examples use qualified names; the public API does not
provide global compatibility aliases.

## 1. Project Overview

This project is a small in-memory analytical query engine written in C++.

The main goals are:

1. Learn modern C++ through a systems-oriented project.
2. Understand the core execution model of a database query engine.
3. Build a deliberately simple and inefficient baseline.
4. Measure the baseline before introducing optimizations.
5. Improve performance one change at a time while preserving correctness.

The engine operates on one predefined in-memory sales table. Queries are represented directly as C++ objects rather than SQL strings.

---

## 2. Current Project Scope

### Included

The project will support:

- Synthetic table generation
- Sequential table scans
- Row filtering (`WHERE`)
- Column projection (`SELECT`)
- Aggregation
  - `COUNT`
  - `SUM`
  - `AVG`
  - `MIN`
  - `MAX`
- Grouped aggregation (`GROUP BY`)
- Result sorting (`ORDER BY`)
- Result limiting (`LIMIT`)
- Correctness tests
- Performance benchmarks
- Multiple execution and storage optimizations

### Explicitly excluded from the baseline

The initial version does not need:

- SQL parsing
- Persistent storage
- Multiple source tables
- Joins
- Transactions
- Indexes
- Query planning
- Query optimization
- Concurrency
- SIMD
- Custom memory allocators
- Networking
- Client-server behavior

These may be explored later, but they are not required for the main project.

---

## 3. Dataset

The engine currently uses a fixed sales-transaction schema:

```cpp
struct Row {
    std::uint64_t transaction_id;
    std::uint64_t product_id;
    std::uint64_t category_id;
    double price;
    std::uint32_t quantity;
    std::int64_t timestamp;
};
```

The table currently uses a row-store representation:

```cpp
struct Table {
    std::vector<ColumnName> column_names;
    std::vector<Row> rows;
};
```

This means all values belonging to one transaction are stored together.

The dataset is generated synthetically so the engine can be tested and benchmarked without depending on external files.

---

## 4. Query Model

Queries are represented as C++ objects.

The current query model contains:

```cpp
using OrderExpression = std::variant<ColumnName, Aggregation>;

struct OrderByItem {
    OrderExpression expr;
    bool ascending;
};

struct Query {
    std::vector<ColumnName> projection;
    std::optional<std::function<bool(const Row&)>> filter;
    std::optional<Aggregation> aggregation;
    std::optional<std::vector<ColumnName>> group_by;
    std::optional<std::vector<OrderByItem>> order_by;
    std::optional<std::size_t> limit;
};
```

### Filter

A filter determines whether a row should be included.

Example:

```cpp
[](query_engine::Row row) {
    return row.price > 100.0;
}
```

Equivalent SQL concept:

```sql
WHERE price > 100
```

Complex expressions such as `AND`, `OR`, and `NOT` can be written directly inside the filter function.

### Projection

Projection determines which columns are returned.

Example:

```cpp
{
    query_engine::ColumnName::PRODUCT_ID,
    query_engine::ColumnName::PRICE
}
```

Equivalent SQL concept:

```sql
SELECT product_id, price
```

### Aggregation

Aggregation reduces multiple filtered rows into one value.

Examples:

```sql
SELECT SUM(price)
SELECT COUNT(product_id)
SELECT AVG(quantity)
```

Only one aggregation can be selected per query. For valid non-grouped aggregate
queries, projection is empty and the aggregate is returned as one value.
Grouped queries can project grouping keys, with the selected aggregate appended.
Unrelated projected columns are not currently rejected by query validation.

Grouping supports multiple columns and the numeric types in `ResultValue`.
Ordering supports multiple expressions with independent directions, including
hidden source columns, hidden group keys, and the selected aggregate. An
order-only aggregate that differs from the selected aggregate is not computed.

---

## 5. Result Model

Projected and aggregated results use a generic value type:

```cpp
using ResultValue = std::variant<
    std::uint64_t,
    std::uint32_t,
    double,
    std::int64_t
>;
```

A result row contains a dynamic list of values:

```cpp
struct ResultRow {
    std::vector<ResultValue> data;
};
```

A query returns:

```cpp
struct ResultTable {
    std::vector<std::string> column_names;
    std::vector<ResultRow> rows;
};
```

A scalar aggregation over nonempty filtered input is represented as a
`ResultTable` containing one row with one value. Empty filtered input currently
returns no rows, including for scalar aggregation; this behavior still needs
an explicit contract and query-level tests.

---

# 6. Current Implementation Status

## Implemented

### Data model

- [x] Fixed `Row` schema
- [x] Row-store `Table`
- [x] Generic result values using `std::variant`
- [x] `ResultRow`
- [x] `ResultTable`
- [x] Column identifiers using `enum class ColumnName`

### Synthetic data

- [x] Synthetic table generation
- [x] Predictable generated values suitable for correctness tests

### Filtering

- [x] Optional row filter
- [x] Full sequential table scan
- [x] Support for arbitrary filter logic through `std::function`
- [x] Queries without a filter include all rows

### Projection

- [x] Projection of selected columns
- [x] Projection order follows query order
- [x] Conversion from fixed `Row` fields to `ResultValue`
- [x] Projection through `getColumnValue`

### Aggregation

- [x] `COUNT`
- [x] `SUM`
- [x] `AVG`
- [x] `MIN`
- [x] `MAX`
- [x] Aggregation after filtering
- [x] Aggregation integrated into `queryTable`
- [x] Scalar aggregation returned as a one-value result row for nonempty input

### Grouping, ordering, limits, and insertion

- [x] Single- and multiple-column grouping
- [x] Numeric variant grouping keys and complete rows stored per group
- [x] Grouping with and without aggregation
- [x] Ascending, descending, and multiple-expression ordering
- [x] Hidden source-column and group-key ordering
- [x] Ordering by the selected aggregate
- [x] Limits after ordering, including zero and oversized limits
- [x] Row insertion through `insertRow`

### Testing and build workflow

- [x] Query correctness tests
- [x] Aggregation correctness tests
- [x] Filtered aggregation tests
- [x] Empty-result tests for several aggregations
- [x] Makefile test targets
- [x] Separate test executables
- [x] CMake presets and seven registered correctness executables
- [x] Google Benchmark general, detailed, and stress suites
- [x] Grouped JSON summaries, medians, throughput, and local baseline comparison

---

## Current main execution flow

### Detail query (no grouping or aggregation)

```text
Borrow input table and query
    ↓
Filter input rows, or select all rows when no filter is supplied
    ↓
Store read-only row wrappers in filtered_rows
    ↓
Materialize all table columns as result values
    ↓
Apply ORDER BY and then LIMIT, when supplied
    ↓
Project selected columns in query order
    ↓
Return ResultTable
```

### Grouping or aggregation query

```text
Borrow input table and query
    ↓
Filter input rows, or select all rows when no filter is supplied
    ↓
Store read-only row wrappers in filtered_rows
    ↓
Build groups by comparing keys against a vector of existing groups
    ↓
Without GROUP BY, use one group containing all filtered rows
    ↓
Build result rows from group keys and the optional aggregate
    ↓
Apply ORDER BY and LIMIT, then project keys and append the aggregate
    ↓
Return ResultTable
```

The original baseline retains by-value inputs and copied rows. Current main
borrows source rows and moves completed output buffers; further algorithmic
optimizations remain separate experiments. See [optimization history](optimization-history.md).

Empty filtered input takes an early return before grouping, aggregation,
ordering, and limiting. Aggregation functions and intermediate builders borrow
their inputs. Grouping searches existing groups linearly rather than using
a hash map.

---

# 7. Functional Features and Remaining Coverage

## Phase 1 — Verify and complete the functional baseline

### 7.1 Single-column `GROUP BY`

Required behavior:

```sql
SELECT category_id, SUM(price)
FROM sales
GROUP BY category_id;
```

Initial implementation requirements:

- [x] Add an optional `group_by` field to `Query`
- [x] Support one or multiple grouping columns
- [x] Partition filtered rows into groups
- [x] Store complete rows inside each group
- [x] Run the existing aggregation function for every group
- [x] Return one result row per group
- [x] Include projected group keys and the aggregate value in each result row

Current representation:

```cpp
std::vector<query_engine::Group>
```

Each `Group` contains `key_columns`, a `std::vector<ResultValue>` key, and
complete input rows. Numeric variant keys already exist. Hash-based grouping
remains a possible optimization.

Required tests:

- [x] Grouped `SUM` assertions in ordering and limit tests
- [ ] Grouped `COUNT`
- [ ] Grouped `AVG`
- [ ] Grouped `MIN`
- [ ] Grouped `MAX`
- [ ] Filtering before grouping
- [ ] Empty grouped input
- [ ] One group
- [ ] Many groups

The dedicated `group_by_test.cpp` currently prints a grouped SUM result but
contains no assertions. Multiple-key grouping without aggregation is asserted
in the hidden-ordering tests. The remaining cases above need focused tests,
even though the same aggregate dispatch implements them.

---

### 7.2 `ORDER BY`

Required behavior:

```sql
SELECT product_id, price
FROM sales
ORDER BY price;
```

Initial implementation requirements:

- [x] Add optional ordering information to `Query`
- [x] Support one or multiple ordering expressions
- [x] Support ascending order
- [x] Support descending order
- [x] Sort the materialized result using `std::sort`
- [x] Compare values stored inside `ResultValue`

Current representation:

```cpp
using OrderExpression = std::variant<ColumnName, Aggregation>;

struct OrderByItem {
    OrderExpression expr;
    bool ascending;
};
```

Required tests:

- [x] Ascending numeric sort
- [x] Descending numeric sort
- [ ] Empty result
- [ ] One-row result
- [x] Duplicate values with additional ordering expressions

---

### 7.3 `LIMIT`

Required behavior:

```sql
SELECT product_id, price
FROM sales
LIMIT 10;
```

Initial implementation requirements:

- [x] Add an optional limit to `Query`
- [x] Apply the limit after aggregation and ordering, before final projection
- [x] Return at most the requested number of rows
- [x] Handle a limit larger than the result size
- [x] Handle a limit of zero

Current representation:

```cpp
std::optional<std::size_t> limit;
```

Required tests:

- [x] Limit smaller than result
- [x] Limit equal to result size
- [x] Limit larger than result
- [x] Limit zero
- [x] Limit after sorting

---

### 7.4 General query validation

The engine should reject unsupported or contradictory query combinations.

Validation status and applicable query rules:

- [x] `GROUP BY` without aggregation is supported; it need not be rejected
- [ ] Aggregation mixed with unrelated projected columns
- [ ] Invalid column identifiers or unsupported future grouping key types
- [ ] Unavailable order-by expressions (currently silently skipped)
- [x] `aggregate()` rejects an active `AggregationType::NONE`
- [ ] Consistent query-level validation before execution, including empty input

Ordering uses expressions rather than user-supplied result indexes. There is no
central query validator. Lower-level helpers reject some invalid identifiers,
but an empty-input early return can bypass aggregation validation.

For the first version, throwing `std::invalid_argument` is sufficient.

---

### 7.5 Edge-case behavior

Define and test the behavior of:

- [ ] `AVG` over zero rows
- [x] Direct `aggregate()` returns zero for empty `MIN`, with a test
- [x] Direct `aggregate()` returns zero for empty `MAX`, with a test
- [ ] Empty projection
- [ ] Empty table
- [x] Detail query with a filter matching zero rows
- [ ] Empty scalar and grouped query contracts and assertions
- [ ] Integer-to-double conversion during aggregation
- [ ] Floating-point comparison in tests

The current implementation returns zero for empty `MIN` and `MAX`. This should be explicitly documented as a project decision or later changed to an optional/null result.

Direct empty `AVG` divides zero by zero without an explicit empty-input branch.
The scalar query API returns no rows on empty input instead of calling the
aggregate. Tests should distinguish these two execution paths.

---

# 8. Baseline Completion Milestone

The required execution paths below are implemented. The functional feature
milestone has been reached, but complete validation and edge-case coverage
remain pending as described in section 7.

The functional baseline is complete when the engine can execute:

```sql
SELECT product_id, price
FROM sales
WHERE price > 100
ORDER BY price DESC
LIMIT 10;
```

and:

```sql
SELECT category_id, SUM(price)
FROM sales
WHERE quantity >= 2
GROUP BY category_id
ORDER BY SUM(price) DESC
LIMIT 5;
```

At this stage, correctness matters more than performance.

The baseline may:

- Copy entire tables
- Copy each row repeatedly
- Store intermediate result vectors
- Rescan rows for every aggregation
- Store full rows inside groups
- Use `std::function`
- Use `std::variant`
- Use `std::unordered_map`
- Use `std::sort`

Do not optimize these behaviors until the complete baseline has correctness tests.

---

# 9. Benchmarking Roadmap

The harness is implemented with 82 detailed cases, 18 representative general
cases, and five stress cases. See [Benchmark Guide](benchmarks.md) for running
them and [Interpreting Benchmark Results](benchmark-interpretation.md) for
reading the output. The SQL examples below are workload ideas; the exact
registered cases and input sizes are defined in `benchmarks/`.

## Dataset sizes

Suggested sizes:

- 1,000 rows
- 100,000 rows
- 1,000,000 rows
- 10,000,000 rows, if practical

## Benchmark queries

### Query A — Full scan projection

```sql
SELECT product_id, price
FROM sales;
```

### Query B — Selective filter

```sql
SELECT product_id, price
FROM sales
WHERE category_id = 3;
```

### Query C — Low-selectivity filter

```sql
SELECT product_id, price
FROM sales
WHERE price > 10;
```

### Query D — Scalar aggregation

```sql
SELECT SUM(price)
FROM sales;
```

### Query E — Filtered aggregation

```sql
SELECT AVG(price)
FROM sales
WHERE quantity >= 3;
```

### Query F — Grouped aggregation

```sql
SELECT category_id, SUM(price)
FROM sales
GROUP BY category_id;
```

### Query G — Sorting and limiting

```sql
SELECT product_id, price
FROM sales
ORDER BY price DESC
LIMIT 100;
```

## Metrics

Initially measure:

- [x] Execution duration
- [x] Rows processed per second
- [x] Query and insertion result row-count checks
- [ ] Full value, schema, and ordering correctness checks within benchmarks

Correctness executables already validate several expected values separately.
Direct aggregation benchmarks currently preserve the result against compiler
elimination but do not verify the value.

Later measure:

- [ ] Memory usage
- [ ] Number of allocations
- [ ] CPU cache misses
- [ ] Branch mispredictions
- [ ] Instructions executed

Use multiple repetitions and report a stable statistic such as median execution time.

---

# 10. Optimization Roadmap

Each optimization should be implemented and benchmarked separately.

Every optimized implementation must satisfy:

> Same query + same dataset = same result.

## Optimization 1 — Remove unnecessary parameter copies

Completed in the object-semantics checkpoint. The original baseline copied:

- `Table`
- `Query`
- `Row`
- Aggregation input vectors

Possible changes:

```cpp
const query_engine::Table&
const query_engine::Query&
const query_engine::Row&
const std::vector<std::reference_wrapper<const query_engine::Row>>&
```

Purpose:

- Reduce large object copies
- Establish the cost of pass-by-value in the baseline

---

## Optimization 2 — Move result rows

Completed in query execution: finished groups, result rows, and projection
buffers move into their owners. Numeric values and borrowed wrappers still copy
where appropriate; not every copy needs elimination.

Replace copies such as:

```cpp
result_table.rows.push_back(result_row);
```

with moves or direct construction.

Possible approaches:

```cpp
result_table.rows.push_back(std::move(result_row));
```

or:

```cpp
result_table.rows.emplace_back(...);
```

Purpose:

- Reduce temporary vector copies

---

## Optimization 3 — Reserve vector capacity

Partially present: generation pre-sizes its row vector, and reserved insertion
benchmarks explicitly call `reserve()`. Reserving query intermediates remains
pending.

Reserve storage for:

- Filtered rows
- Result rows
- Projected values inside each result row

Purpose:

- Reduce repeated dynamic allocations and reallocations

---

## Optimization 4 — Fuse filtering and projection

Baseline:

```text
filter all rows
→ store filtered rows
→ project filtered rows
```

Optimized:

```text
scan row
→ test filter
→ immediately project matching row
```

Purpose:

- Remove the intermediate filtered table
- Avoid scanning matching rows twice
- Reduce memory traffic

---

## Optimization 5 — Fuse filtering and scalar aggregation

Baseline:

```text
filter rows
→ materialize filtered rows
→ aggregate them
```

Optimized:

```text
scan row
→ test filter
→ update aggregate state immediately
```

Purpose:

- Avoid materializing filtered rows
- Reduce copying
- Reduce memory use

---

## Optimization 6 — Single-pass aggregation

The baseline may calculate `AVG` by separately calculating `SUM` and `COUNT`.

Optimized approach:

```text
one scan
→ update sum and count
→ calculate average
```

Similarly, avoid repeated conversions and repeated calls to `getColumnValue`.

Purpose:

- Reduce the number of scans
- Reduce dispatch and variant-access overhead

---

## Optimization 7 — Reduce `std::function` overhead

Possible alternatives:

- Templated filter execution
- Function objects
- Expression objects
- A small custom predicate representation

Purpose:

- Avoid type-erasure and indirect-call overhead

This should only be attempted after measuring whether filtering dispatch is significant.

---

## Optimization 8 — Improve result-value representation

The current result uses:

```cpp
std::variant<std::uint64_t, std::uint32_t, double, std::int64_t>
```

Possible experiments:

- Convert all numeric output to `double`
- Use typed result columns
- Use separate result buffers per type
- Use a tagged union or custom compact value type

Purpose:

- Reduce variant visitation and storage overhead
- Compare simplicity against type safety

---

## Optimization 9 — Column-store layout

Current row store:

```cpp
std::vector<query_engine::Row>
```

Possible column store:

```cpp
struct ColumnTable {
    std::vector<std::uint64_t> transaction_ids;
    std::vector<std::uint64_t> product_ids;
    std::vector<std::uint64_t> category_ids;
    std::vector<double> prices;
    std::vector<std::uint32_t> quantities;
    std::vector<std::int64_t> timestamps;
};
```

Purpose:

- Read only the columns required by a query
- Improve cache efficiency for analytical workloads
- Enable easier vectorization later

Benchmark row store and column store using identical datasets and queries.

---

## Optimization 10 — More efficient grouping

Baseline:

```cpp
std::vector<query_engine::Group>  // each group contains a key and complete input rows
```

Each input key is compared against existing groups by linear search. With many
distinct keys this can require quadratic work. A hash map would itself be a
change from this baseline.

Possible improvements:

- Store aggregate state instead of full rows
- Reserve hash-map capacity
- Use dense arrays when group IDs are small
- Use sorted grouping
- Use custom hash tables
- Use typed grouping keys

Purpose:

- Avoid storing and rescanning complete groups
- Reduce hash-map and allocation overhead

---

## Optimization 11 — Top-K instead of full sort

For:

```sql
ORDER BY price DESC
LIMIT 10
```

the baseline can sort all results.

Possible optimization:

- Maintain a heap of only the best `K` rows
- Use `std::partial_sort`
- Use `std::nth_element`

Purpose:

- Avoid sorting the entire result when the limit is small

---

## Optimization 12 — Parallel execution

Split the table into chunks.

Each worker:

- Scans one chunk
- Applies filters
- Produces partial projections or aggregates

Then merge partial results.

Possible targets:

- Parallel filtering
- Parallel scalar aggregation
- Parallel grouped aggregation

Purpose:

- Use multiple CPU cores

Only add concurrency after the single-threaded engine is correct and benchmarked.

---

## Optimization 13 — SIMD and branch reduction

Possible experiments:

- SIMD comparisons over columnar data
- Branchless filtering
- Batched predicate evaluation
- Bitmaps for matching rows

Purpose:

- Process multiple values per CPU instruction
- Reduce branch misprediction cost

This is an advanced phase and should come after storage-layout optimization.

---

# 11. General Development Roadmap

## Stage 1 — Functional baseline

- [x] Fixed row schema
- [x] Table representation
- [x] Synthetic data
- [x] Filtering
- [x] Projection
- [x] Scalar aggregation
- [x] Single- and multiple-column `GROUP BY`
- [x] `ORDER BY`, including multiple expressions and hidden columns
- [x] `LIMIT`
- [ ] Query validation
- [ ] Complete edge-case tests

Deliverable:

> A correct but deliberately inefficient query engine.

---

## Stage 2 — Benchmark harness

- [x] Generate predictable numeric datasets
- [ ] Make generated timestamps repeatable
- [x] Define fixed benchmark queries
- [x] Integrate Google Benchmark timing and calibration
- [ ] Configure dedicated warm-up iterations (calibration is already present)
- [x] Run repeated measurements
- [x] Report median execution time
- [x] Verify query and insertion row counts during benchmarks
- [ ] Verify full output values, schemas, and ordering during benchmarks
- [x] Save baseline results and compare grouped JSON summaries

Deliverable:

> A reproducible baseline performance report.

---

## Stage 3 — Basic C++ optimizations

- [x] Remove unnecessary input and completed-buffer copies
- [x] Replace expensive production parameter copies with references
- [x] Use move semantics for query intermediates and result rows
- [ ] Reserve query intermediate vectors (generation and insertion already do)
- [ ] Reduce temporary objects
- [ ] Reduce repeated scans

Deliverable:

> A faster row-store engine with the same architecture.

---

## Stage 4 — Execution optimizations

- [ ] Fuse filter and projection
- [ ] Fuse filter and aggregation
- [ ] Use single-pass aggregate states
- [ ] Improve grouped aggregation
- [ ] Optimize sorting with a limit

Deliverable:

> An engine that avoids unnecessary intermediate results.

---

## Stage 5 — Storage-layout comparison

- [ ] Implement column-store storage
- [ ] Run identical queries on row and column stores
- [ ] Compare projection-heavy queries
- [ ] Compare full-row queries
- [ ] Profile cache behavior

Deliverable:

> A measured comparison of row-oriented and column-oriented execution.

---

## Stage 6 — Advanced performance work

- [ ] Parallel scans
- [ ] Parallel aggregation
- [ ] SIMD filtering
- [ ] Branchless execution experiments
- [ ] Specialized query paths
- [ ] Memory allocation experiments
- [ ] Hardware-counter profiling

Deliverable:

> A set of independently measured advanced optimizations.

---

## Stage 7 — Optional extensions

Most of these remain optional future extensions. Two originally optional
grouping capabilities are already implemented:

- [ ] Multiple aggregation expressions
- [x] Multiple `GROUP BY` columns
- [x] Numeric group-key variants using `ResultValue`
- [ ] Computed expressions such as `price * quantity`
- [ ] Aliases for result columns
- [ ] Null values
- [ ] String columns
- [ ] CSV loading
- [ ] SQL parser
- [ ] Query-plan/operator representation
- [ ] Multiple tables
- [ ] Joins
- [ ] Indexes

---

# 12. Recommended Immediate Next Step

Grouping, ordering, limits, and the benchmark harness are already implemented.
The `main` branch is the development branch. The separate `baseline` branch
preserves the reference implementation for performance comparisons. Capture
fresh reference measurements locally; timing files stay under the ignored
`build/` directory.

Recommended order:

1. Define the supported-query contract, including empty aggregation and invalid
   projection or ordering combinations. Decide which validation belongs in this
   deliberately small project.
2. Add assertions to the group-by test and cover grouped aggregate types,
   filtering before grouping, empty input, and multiple grouping-key types.
3. Cover the chosen empty-input and empty-projection contracts at query level.
4. Keep the reference revision and benchmark workloads fixed for comparisons.
5. Preserve the object-semantics checkpoint, then isolate the next optimization
   on an experiment branch when trying a competing approach.
6. Run correctness tests and compare fresh reference and candidate measurements
   on the same machine using the [branch comparison workflow](benchmarks.md#baseline-branch).

Further engine optimizations should remain separate experiments. Preserve the
chosen result semantics and report repeatable measurements for each change.

---

# 13. Project Principles

Throughout the project:

1. Correctness comes before performance.
2. Keep the baseline easy to understand.
3. Avoid premature abstractions.
4. Measure before claiming improvement.
5. Change one performance dimension at a time.
6. Preserve identical query behavior after every optimization.
7. Keep correctness tests separate from performance benchmarks.
8. Document why each optimization should help.
9. Record its measured impact.
10. Record any trade-offs introduced.
