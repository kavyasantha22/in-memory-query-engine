# Optimization History

This document preserves completed implementations and their evidence. Current
execution design lives in [engine design](engine-design.md); routine commands
and promotion rules live in the [benchmark guide](benchmarks.md).

## Current Reference: Precomputed Values

Promoted on 4 October 2026 at the developer's request.

- Active reference: `89ca7cddb972f669c27387a18e9108c87f78280c` on `main`.
- Implementation commits: `73caa34` (cached projection and sorting indices),
  `ea33db6` (cached aggregate projection lookup and accessor experiment), and
  `552a37e` (fixed function-pointer accessor array). `89ca7cd` documents
  the earlier experiment; it is the exact measured checkout.
- Previous comparison reference: `dc9bc85776e6d5c2d5223e974324c7fc9a1df32d`.
- All seven correctness tests passed before measurement. Both revisions built
  independently with the same compiler and Release flags.
- `general-best.json` and `general-best.metadata.json` now contain this
  checkpoint's measurements and revision provenance. This is the **new local
  comparison baseline**, not a replacement for the historical `baseline`
  branch or `general-baseline.json`. No Git tag was created.

### Design And Scope

Projection and ORDER BY resolve requested column indices before processing
output rows or comparing them. Comparisons reuse those indices rather than
converting expression names and searching result metadata repeatedly.
Aggregate projection resolves its output-column index before its row loop.
Field access uses a fixed array of function pointers rather than the previous
`std::function` vector.

The measurements below compare the complete checkpoint with object semantics.
They do not isolate the contribution of each change, and they do not establish
that function pointers are faster than the original switch. The earlier
six-case accessor experiment measured `std::function` versus a switch, not
this function-pointer implementation. Group lookup is still a linear scan;
hash-based grouping is not part of this checkpoint.

### Capture Conditions

The 18-case general suite ran in three alternating paired rounds with five
repetitions per case, a 0.1-second minimum repetition time, and random
interleaving. The summary is the median of per-round median wall times.
Ranges below are minimum-maximum round medians, not confidence intervals.
Positive reduction means less time than the old reference.

The machine was Apple M4, 10 cores, 32 GiB RAM, macOS 27.0. Builds used AppleClang
21.0.0, CMake 4.4.2, and `-O3 -DNDEBUG -std=c++20 -arch arm64 -Wall -Wextra`.
The recorded power snapshot was **battery power**; background load, temperature,
and scheduling were not isolated. This capture is not directly comparable to
the earlier AC-powered absolute timings. Recalibrate this same revision under
matching power and machine conditions when necessary.

| Workload | Previous ms (range) | Precomputed ms (range) | Time Reduction | Interpretation |
| --- | ---: | ---: | ---: | --- |
| `bmAggregateAverage/1000000` | 3.499 (3.437-3.525) | 3.047 (2.978-3.052) | 12.9% | observed reduction |
| `bmAggregateSum/1000000` | 3.571 (3.522-3.578) | 3.071 (3.006-3.119) | 14.0% | observed reduction |
| `bmComposedGroupedQuery/1000000` | 34.270 (33.329-36.783) | 33.839 (33.254-36.213) | 1.3% | ranges overlap; inconclusive |
| `bmComposedHiddenOrderQuery/1000000` | 79.799 (79.255-82.819) | 44.569 (41.817-47.140) | 44.1% | observed reduction |
| `bmFilterFiftyPercent/1000000` | 83.243 (81.273-83.849) | 80.988 (78.181-87.661) | 2.7% | ranges overlap; inconclusive |
| `bmFilterOneHundredPercent/1000000` | 163.055 (161.332-188.887) | 160.060 (155.060-163.159) | 1.8% | ranges overlap; inconclusive |
| `bmFilterTenPercent/1000000` | 23.156 (22.047-24.585) | 23.229 (22.648-24.232) | -0.3% | ranges overlap; inconclusive |
| `bmGroupByCategory/1000000` | 70.189 (68.144-71.948) | 70.985 (69.031-73.444) | -1.1% | ranges overlap; inconclusive |
| `bmGroupByCategoryAndProduct/1000000` | 236.644 (233.522-249.062) | 236.137 (230.622-240.876) | 0.2% | ranges overlap; inconclusive |
| `bmGroupByProduct/1000000` | 167.527 (165.902-168.063) | 167.669 (163.017-172.731) | -0.1% | ranges overlap; inconclusive |
| `bmInsertReserved/1000000` | 5.171 (5.142-5.226) | 5.217 (5.213-5.300) | -0.9% | ranges overlap; inconclusive |
| `bmInsertUnreserved/1000000` | 8.276 (8.230-8.592) | 8.317 (8.125-8.385) | -0.5% | ranges overlap; inconclusive |
| `bmOrderByMultipleColumns/100000` | 136.119 (135.871-140.333) | 75.078 (68.601-75.927) | 44.8% | observed reduction |
| `bmOrderByVisibleColumn/100000` | 82.852 (82.254-85.629) | 44.562 (41.990-46.104) | 46.2% | observed reduction |
| `bmOrderByWithLimit/100000` | 70.911 (69.294-71.003) | 34.554 (34.359-36.667) | 51.3% | observed reduction |
| `bmProjectionAllColumns/1000000` | 353.036 (342.960-364.826) | 240.372 (235.645-250.180) | 31.9% | observed reduction |
| `bmProjectionOneColumn/1000000` | 174.133 (173.334-175.914) | 167.748 (154.411-172.098) | 3.7% | observed reduction |
| `bmProjectionThreeColumns/1000000` | 261.823 (245.923-263.079) | 205.955 (197.867-212.172) | 21.3% | observed reduction |

Ordering shows observed reductions of roughly 45-51%, and projection 4-32%.
Filtering, grouping, insertion, and the composed grouped query have overlapping
round ranges, so their small changes are inconclusive. These are local
observations, not formal significance tests or hardware-independent guarantees.

### Artifacts And Future Comparisons

The full local archive is
`build/benchmark-clang/benchmark-results/checkpoints/precomputed-values-89ca7cd/`.
It contains `comparison.json`, `comparison.md`, and separate `reference/`
and `candidate/` directories with summaries, metadata, raw repetitions,
logs, and exact configure/build/run commands. The promoted files were copied
from `candidate/summary.json` and `candidate/metadata.json`; no fastest-case
mixture was used. Build artifacts remain ignored; this document preserves
the checkpoint identity and numerical summary.

After committing the next optimization, compare it with this reference:

```sh
bash scripts/benchmark-checkpoints.sh --candidate HEAD
```

That command reads the revision from `general-best.metadata.json` and
re-measures both revisions under the current conditions. Measuring this same
HEAD again is a repeatability check, not another optimization. Normal general
runs compare with the promoted local timing file but never overwrite it.
Keep the original baseline and previous checkpoint archives intact.

## Object-semantics checkpoint

- Original baseline: `f15bdcbd958b9b455979dfc1c6657a1f07560fe1`.
- Measured checkpoint: `dc9bc85776e6d5c2d5223e974324c7fc9a1df32d`,
  the non-fast-forward object-semantics merge.
- Incremental commits: `996c0c4` (const-reference inputs and formatting),
  `33fd77c` (borrowed rows), `391d658` (intermediate moves),
  `6f73e2d` (benchmark setup copies).
- Status: library and benchmark builds succeed. All seven debug tests pass
  after adapting the stale direct aggregation test to wrapper selections in
  this documentation change. Engine behavior is unchanged by that test update.
- No best checkpoint has been promoted automatically. No tag was created.

### Ownership choices

The caller owns the input `Table` and `Query`. Execution borrows them rather
than copying their vectors. Filtering and grouping own vectors of
`std::reference_wrapper<const Row>`, borrowing rows from the unchanged source
table. Groups own their selections, not a reference to the filtering vector.
Source rows must remain alive and at stable addresses until execution finishes.

Completed key vectors, groups, result rows, and projection buffers move into
their owners. Returned temporary values need no explicit move; local return
statements permit copy elision. Numeric values still copy where appropriate:
a numeric-only `Row` or `ResultValue` has no expensive buffer to transfer.
The returned `ResultTable` owns its values and can outlive the source table.

The comparator borrows ordering/schema metadata and is passed via `std::cref`.
Grouping also stops searching when a matching key is found. This checkpoint
therefore includes a search reduction as well as copy reductions; its measured
benefits cannot be attributed solely to move semantics.

The formatter borrows read-only objects and moves completed string-row buffers.
Direct aggregation benchmarks prepare row wrappers outside timing; complete
query benchmarks include selection construction inside the timed public call.
Moving benchmark setup vectors/callbacks and borrowing the runner's query are
outside timing and are not reported query speedups.

### Measurement method

Measured on 3 October 2026 on a MacBook Pro Mac16,1, Apple M4 (10 cores,
32 GB), macOS 27.0 (26A428), AC power, low-power mode off. Apple Clang
21.0.0 (`clang-2100.1.1.101`), CMake 4.4, C++20 Release with
`-O3 -DNDEBUG`; Google Benchmark v1.9.5 and nlohmann/json v3.11.3.

Both exact revisions were built independently in detached worktrees, using
the same locally cached pinned dependency sources. The curated case list,
reporter, and preset settings are identical across revisions. Each run used
five repetitions, random repetition interleaving, and `0.1s` minimum time.
Three rounds ran sequentially: baseline/checkpoint, checkpoint/baseline,
baseline/checkpoint. No builds or tests ran concurrently with measurements.

The table reports the median of three per-round median wall times; ranges are
the minimum and maximum per-round medians, not confidence intervals. Raw
reports retain every repetition and CPU timing. Time reduction is
`(baseline - checkpoint) / baseline * 100`; speedup is `baseline / checkpoint`.
Negative reduction means slower.

| Workload | Baseline ms (range) | Checkpoint ms (range) | Time Reduction | Speedup |
| --- | ---: | ---: | ---: | ---: |
| `bmProjectionOneColumn/1000000` | 126.033 (125.620-126.962) | 88.988 (88.885-92.744) | 29.4% | 1.42x |
| `bmProjectionThreeColumns/1000000` | 168.786 (166.760-170.154) | 130.456 (127.431-143.461) | 22.7% | 1.29x |
| `bmProjectionAllColumns/1000000` | 222.827 (217.942-222.927) | 181.933 (178.932-188.097) | 18.4% | 1.22x |
| `bmFilterTenPercent/1000000` | 15.628 (15.583-15.714) | 11.726 (11.493-11.886) | 25.0% | 1.33x |
| `bmFilterFiftyPercent/1000000` | 63.206 (62.716-64.214) | 42.983 (42.827-43.300) | 32.0% | 1.47x |
| `bmFilterOneHundredPercent/1000000` | 125.293 (121.973-125.423) | 83.071 (82.861-84.301) | 33.7% | 1.51x |
| `bmAggregateSum/1000000` | 4.243 (4.227-4.331) | 1.771 (1.768-1.772) | 58.3% | 2.40x |
| `bmAggregateAverage/1000000` | 5.528 (5.504-5.623) | 1.772 (1.770-1.804) | 67.9% | 3.12x |
| `bmGroupByCategory/1000000` | 46.025 (45.335-47.100) | 37.634 (37.485-38.075) | 18.2% | 1.22x |
| `bmGroupByProduct/1000000` | 146.629 (145.876-148.818) | 85.876 (85.217-86.266) | 41.4% | 1.71x |
| `bmGroupByCategoryAndProduct/1000000` | 189.471 (188.322-189.805) | 122.807 (122.113-122.824) | 35.2% | 1.54x |
| `bmOrderByVisibleColumn/100000` | 43.292 (42.936-44.539) | 41.402 (40.752-62.104) | 4.4% | 1.05x |
| `bmOrderByMultipleColumns/100000` | 64.660 (64.506-68.217) | 67.844 (65.281-96.096) | -4.9% | 0.95x |
| `bmOrderByWithLimit/100000` | 37.370 (35.606-37.810) | 36.282 (36.120-47.070) | 2.9% | 1.03x |
| `bmComposedGroupedQuery/1000000` | 24.708 (24.575-25.291) | 18.488 (18.384-18.776) | 25.2% | 1.34x |
| `bmComposedHiddenOrderQuery/1000000` | 42.666 (41.234-43.050) | 42.005 (39.889-55.240) | 1.6% | 1.02x |
| `bmInsertReserved/1000000` | 2.623 (2.617-2.635) | 2.628 (2.627-2.841) | -0.2% | 1.00x |
| `bmInsertUnreserved/1000000` | 4.042 (4.008-4.080) | 4.094 (4.044-4.130) | -1.3% | 0.99x |

### Interpretation and limits

Projection is 18-29% faster, filtering 25-34%, direct SUM/AVG 58-68%, and
grouping 18-41% in these local runs. The grouped composed query is about 25%
faster. These are combined-checkpoint observations, not isolated measurements
of individual commits or universal C++ feature benefits.

Ordering ranges are wide for the checkpoint. Multi-column sorting's median is
about 5% slower, with overlapping ranges; other ordering changes are small or
inconsistent. These results do not establish a reliable ordering improvement
or a statistically confirmed regression. Insertion is effectively unchanged
at this resolution (roughly 0-1% median slowdown). Re-run focused workloads
before choosing the next optimization or promoting a reference.

The machine was not isolated from other applications; scheduler placement,
background load, and temperature were not controlled. Frequency/affinity
metadata warnings occurred on Apple Silicon and were retained in logs.
Random interleaving is within each executable, not between revisions.
Three rounds are evidence, not a formal statistical study.

### Local artifacts and reproduction

For new comparisons, prefer the [standardized paired-measurement script](benchmarks.md#standardized-paired-measurements).
It defaults to longer repetitions, records provenance automatically, and emits
Markdown. The historical measurements below used the original 0.1s/five-repeat
settings; running the new defaults does not retroactively change that evidence.

Artifacts stay under ignored `build/benchmark-clang/benchmark-results/checkpoints/`:
each revision directory contains `round-1.json` through `round-3.json`,
corresponding `round-N-raw.json` and logs, `summary.json` containing the median
of round medians, and `metadata.json` recording the
revision, environment, flags, commands, and execution order. They are local;
the numerical summary and provenance above are retained in Git.

To recreate isolated revisions:

```sh
git worktree add --detach /tmp/query-engine-checkpoint-baseline f15bdcb
git worktree add --detach /tmp/query-engine-checkpoint-object-semantics dc9bc85
```

If those worktrees already exist, reuse them. In each, configure with
`cmake --preset benchmark` and build
`cmake --build --preset benchmark --target query_engine_general_benchmark`.
For offline configuration, reuse the pinned dependency directories using
`FETCHCONTENT_SOURCE_DIR_GOOGLE_BENCHMARK` and
`FETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON`, as recorded in local metadata.
Inspect generated `flags.make` and compiler versions before comparing.

Run this command in each worktree, replacing the absolute archive directory
and round number, and alternate revision order as described above:

```sh
./build/benchmark-clang/query_engine_general_benchmark \
  --general_summary=/absolute/archive/REVISION/round-N.json \
  --benchmark_out=/absolute/archive/REVISION/round-N-raw.json \
  --benchmark_out_format=json \
  --benchmark_min_time=0.1s --benchmark_repetitions=5 \
  --benchmark_enable_random_interleaving
```

Create the archive directories first. Do not pass aggregate-only output flags
when collecting raw repetitions. Check that all 18 case names match and none
reports an error. Retain the environment, commit IDs, and exact commands.

## Static accessor-table experiment

Reviewed on 4 October 2026. Experiment revision: `ea33db6`; preceding
column-index checkpoint: `73caa34`.

### Approach

`getColumnValue()` replaced its switch with a function-local static
`std::vector<std::function<ResultValue(const Row&)>>`. Each entry was a
non-capturing lambda reading one numeric field. The column enum selected the
entry on every call, and enum order had to match accessor order.

The table was initialized once, but this did not resolve an accessor once per
query: every value access still selected an entry and dispatched through
`std::function`. It exchanged a switch for type-erased callable dispatch.
The static vector also required initial allocation; that is not a repeated
per-row allocation and should not be confused with the recurring dispatch cost.

The same commit separately moved aggregate projection's column-name lookup
outside the result-row loop. That change does not require the accessor table.
Its expected impact is small in the general suite: grouped cases project only
10 or 100 rows, the composed grouped case projects five rows after LIMIT, and
direct aggregation cases do not call projection.

### Observation and decision

The developer observed slower execution with the accessor-table implementation.
An isolated comparison on 4 October 2026 supports that observation, especially
for direct SUM and AVG. Both variants used `ea33db6` query code, including the
aggregate projection lookup optimization. The switch variant changed only
`src/table.cpp`, restoring its contents from `73caa34`. Comparing those two
commits directly would instead measure both changes together.

### Isolated measurements

Both detached worktrees used the `benchmark` Release preset, AppleClang
21.0.0.21000101 (`-O3 -DNDEBUG`), Google Benchmark v1.9.5, and macOS 27.0
(26A428). Six workloads each processed 1,000,000 input rows. There were three
paired rounds, ten repetitions per workload, a one-second minimum per
repetition, and random interleaving within each run. Variant order alternated:
switch/accessor, accessor/switch, switch/accessor. All builds finished before
measurement; all six runs completed with 60 iteration records and no benchmark
errors each.

Times below are median wall times across the three per-round medians. Ranges
are the minimum and maximum of those round medians, not confidence intervals.
Positive change means the accessor table took longer than the switch.

| Workload | Switch ms (range) | Accessor ms (range) | Accessor time increase |
| --- | ---: | ---: | ---: |
| One-column projection | 82.987 (82.229-85.116) | 85.525 (85.437-86.968) | +3.1% |
| All-column projection | 120.740 (120.199-121.153) | 124.390 (123.287-125.443) | +3.0% |
| SUM | 1.780 (1.775-1.798) | 2.093 (2.049-2.102) | +17.6% |
| AVG | 1.792 (1.780-1.800) | 2.059 (2.048-2.086) | +14.9% |
| 10 category groups | 38.405 (37.631-38.504) | 39.642 (39.091-40.291) | +3.2% |
| 100 product groups | 87.192 (86.953-88.443) | 89.305 (87.386-89.938) | +2.4% |

Local artifacts are in
`build/benchmark-clang/benchmark-results/accessor-experiment-20261004/`:
`{switch,accessor}-round-{1,2,3}.json`, matching console logs,
`switch-only.patch`, `metadata.json`, and `summary.json`. Metadata records the
exact revision, patch checksum, commands, and aggregation method. These build
artifacts are not committed; this table preserves the measured summary.

The machine was not isolated or CPU-pinned. The SUM/AVG round ranges are well
separated; the smaller projection/grouping differences warrant more caution,
particularly product grouping whose ranges overlap. No statistical confidence
or general hardware-independent slowdown is claimed. BigO/RMS records from a
single tested row count are not evidence of scaling behavior.

Callable dispatch is a plausible explanation for the slowdown, not a profiled
finding. A compiler can optimize a small switch efficiently; replacing it with
indirection is not inherently faster. Function pointers or resolving accessors
outside the row loop are different experiments and have not been validated by
this result. This observation does not reject every accessor-based design.

The measured switch variant retains `std::invalid_argument` for unknown columns
and the independent aggregate projection lookup optimization. It was built in
a separate worktree; the developer's current implementation was not changed.
The `std::function` experiment remains available in Git and is not promoted as
a best checkpoint. A later static function-pointer array is a separate design
and is not measured by these results.

Keep this as a rejected implementation choice with measured evidence for these
workloads. Future accessor experiments should isolate the change and repeat
paired measurements before recording numerical claims or promotion.

## Preserve and promote checkpoints

Use commits for incremental changes, tags for completed implementations, and
experiment branches for competing designs. After reviewing verification and
measurements, this explicit tag would preserve the measured engine revision:

```sh
git tag -a perf/object-semantics dc9bc85 -m "Borrowed-row and move-semantics checkpoint"
```

The measured revision's direct aggregation test was stale; the test-only
adaptation in this documentation update verifies the same engine code.
Alternatively tag the subsequent documentation/test commit after it is made,
and retain `dc9bc85` as the exact measurement revision.

Keep the original baseline fixed. Choose one reviewed checkpoint as the
best reference, not a synthetic mixture of per-case winners. Follow the
[explicit promotion commands](benchmarks.md#general-suite) to copy its report
and metadata to `general-best.json` and its companion. Normal runs compare
against that file but never replace it. Re-measure it when conditions change.

For the next checkpoint, record its revision, design changes and tradeoffs,
correctness status, environment, and measurements against both the original
baseline and nominated best checkpoint. Small differences remain unconfirmed
until repeatable; there is no automatic promotion threshold.
