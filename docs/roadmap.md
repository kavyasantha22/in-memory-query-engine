# In-Memory Query Engine — Features and Roadmap

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
struct Query {
    std::vector<ColumnName> projection;
    std::optional<std::function<bool(Row)>> filter;
    std::optional<Aggregation> aggregation;
};
```

### Filter

A filter determines whether a row should be included.

Example:

```cpp
[](Row row) {
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
    ColumnName::PRODUCT_ID,
    ColumnName::PRICE
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

For the current non-grouped aggregation implementation, projection is ignored when an aggregation is present.

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
    std::vector<ResultRow> rows;
};
```

A scalar aggregation is currently represented as a `ResultTable` containing one row with one value.

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
- [x] Aggregation integrated into `query_table`
- [x] Aggregation returned as a one-value result row

### Testing and build workflow

- [x] Query correctness tests
- [x] Aggregation correctness tests
- [x] Filtered aggregation tests
- [x] Empty-result tests for several aggregations
- [x] Makefile test targets
- [x] Separate test executables

---

## Current baseline execution flow

### Projection query

```text
Copy input table
    ↓
Scan all rows
    ↓
Copy matching rows into filtered_rows
    ↓
Scan filtered_rows
    ↓
Extract projected values
    ↓
Return ResultTable
```

### Aggregation query

```text
Copy input table
    ↓
Scan all rows
    ↓
Copy matching rows into filtered_rows
    ↓
Pass filtered_rows into aggregation
    ↓
Repeatedly copy and scan rows
    ↓
Return one-row ResultTable
```

This is deliberately inefficient and is useful as the baseline for future optimization experiments.

---

# 7. Features Yet to Implement

## Phase 1 — Finish functional query features

### 7.1 Single-column `GROUP BY`

Required behavior:

```sql
SELECT category_id, SUM(price)
FROM sales
GROUP BY category_id;
```

Initial implementation requirements:

- [ ] Add an optional `group_by` field to `Query`
- [ ] Support one grouping column
- [ ] Partition filtered rows into groups
- [ ] Store complete rows inside each group
- [ ] Run the existing aggregation function for every group
- [ ] Return one result row per group
- [ ] Include the group key and aggregate value in each result row

Recommended initial representation:

```cpp
std::unordered_map<std::uint64_t, std::vector<Row>>
```

Initially, grouping may be restricted to `std::uint64_t` columns such as:

- `transaction_id`
- `product_id`
- `category_id`

Later, grouping can support every `ResultValue` type.

Required tests:

- [ ] Grouped `SUM`
- [ ] Grouped `COUNT`
- [ ] Grouped `AVG`
- [ ] Grouped `MIN`
- [ ] Grouped `MAX`
- [ ] Filtering before grouping
- [ ] Empty grouped input
- [ ] One group
- [ ] Many groups

---

### 7.2 `ORDER BY`

Required behavior:

```sql
SELECT product_id, price
FROM sales
ORDER BY price;
```

Initial implementation requirements:

- [ ] Add optional ordering information to `Query`
- [ ] Support one result column
- [ ] Support ascending order
- [ ] Support descending order
- [ ] Sort the materialized result using `std::sort`
- [ ] Compare values stored inside `ResultValue`

Possible representation:

```cpp
enum class SortDirection {
    ASCENDING,
    DESCENDING
};

struct OrderBy {
    std::size_t result_column_index;
    SortDirection direction;
};
```

Required tests:

- [ ] Ascending numeric sort
- [ ] Descending numeric sort
- [ ] Empty result
- [ ] One-row result
- [ ] Duplicate values

---

### 7.3 `LIMIT`

Required behavior:

```sql
SELECT product_id, price
FROM sales
LIMIT 10;
```

Initial implementation requirements:

- [ ] Add an optional limit to `Query`
- [ ] Apply the limit after projection or aggregation
- [ ] Return at most the requested number of rows
- [ ] Handle a limit larger than the result size
- [ ] Handle a limit of zero

Possible representation:

```cpp
std::optional<std::size_t> limit;
```

Required tests:

- [ ] Limit smaller than result
- [ ] Limit equal to result size
- [ ] Limit larger than result
- [ ] Limit zero
- [ ] Limit after sorting

---

### 7.4 General query validation

The engine should reject unsupported or contradictory query combinations.

Examples:

- [ ] `GROUP BY` without aggregation, if not supported
- [ ] Aggregation mixed with unrelated projected columns
- [ ] Unsupported grouping key type
- [ ] Invalid order-by result index
- [ ] `AggregationType::NONE` passed as an active aggregation

For the first version, throwing `std::invalid_argument` is sufficient.

---

### 7.5 Edge-case behavior

Define and test the behavior of:

- [ ] `AVG` over zero rows
- [ ] `MIN` over zero rows
- [ ] `MAX` over zero rows
- [ ] Empty projection
- [ ] Empty table
- [ ] Filter matching zero rows
- [ ] Integer-to-double conversion during aggregation
- [ ] Floating-point comparison in tests

The current implementation returns zero for empty `MIN` and `MAX`. This should be explicitly documented as a project decision or later changed to an optional/null result.

---

# 8. Baseline Completion Milestone

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

Before optimizing, create stable benchmark workloads.

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

- [ ] Execution duration
- [ ] Rows processed per second
- [ ] Result row count
- [ ] Correctness against expected results

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

Current behavior copies:

- `Table`
- `Query`
- `Row`
- Aggregation input vectors

Possible changes:

```cpp
const Table&
const Query&
const Row&
const std::vector<Row>&
```

Purpose:

- Reduce large object copies
- Establish the cost of pass-by-value in the baseline

---

## Optimization 2 — Move result rows

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
std::vector<Row>
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
std::unordered_map<GroupKey, std::vector<Row>>
```

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
- [ ] Single-column `GROUP BY`
- [ ] `ORDER BY`
- [ ] `LIMIT`
- [ ] Query validation
- [ ] Complete edge-case tests

Deliverable:

> A correct but deliberately inefficient query engine.

---

## Stage 2 — Benchmark harness

- [ ] Generate repeatable datasets
- [ ] Define fixed benchmark queries
- [ ] Add timing utilities
- [ ] Run warm-up iterations
- [ ] Run repeated measurements
- [ ] Report median execution time
- [ ] Verify outputs during benchmarks
- [ ] Save baseline results

Deliverable:

> A reproducible baseline performance report.

---

## Stage 3 — Basic C++ optimizations

- [ ] Remove unnecessary copies
- [ ] Use references
- [ ] Use move semantics
- [ ] Reserve vectors
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

These are optional and should only be attempted after the main goals are complete:

- [ ] Multiple aggregation expressions
- [ ] Multiple `GROUP BY` columns
- [ ] Generic group-key variants
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

Implement single-column grouped aggregation.

The first supported grouped query should be equivalent to:

```sql
SELECT category_id, SUM(price)
FROM sales
GROUP BY category_id;
```

Suggested order:

1. Add `std::optional<ColumnName> group_by` to `Query`.
2. Filter the input rows using the existing logic.
3. Partition filtered rows by `category_id`.
4. Store complete `Row` objects in each group.
5. Run the existing aggregation function on each group.
6. Produce result rows containing:

```text
[group key, aggregate value]
```

7. Add grouped aggregation correctness tests.
8. Only then proceed to `ORDER BY`.

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
