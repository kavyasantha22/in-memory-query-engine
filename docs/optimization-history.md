# Optimization History

This document preserves completed implementations and their evidence. Current
execution design lives in [engine design](engine-design.md); routine commands
and promotion rules live in the [benchmark guide](benchmarks.md).

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
