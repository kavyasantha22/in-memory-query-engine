# CMake Build Guide

This project uses CMake to describe its libraries, executables, tests, and
Google Benchmark dependency. `CMakePresets.json` provides named configurations
so the same settings can be reused from the terminal and VS Code.

## Build workflow

CMake has three separate steps:

1. **Configure** reads `CMakeLists.txt`, selects a compiler, resolves
   dependencies, and generates a native build system.
2. **Build** invokes that generated build system to compile and link targets.
3. **Test** runs tests that were registered with CTest.

For the debug configuration:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

For the optimized benchmark configuration:

```sh
cmake --preset benchmark
cmake --build --preset benchmark
cmake --build --preset benchmark --target run_benchmarks
```

The first benchmark configuration may download Google Benchmark from GitHub.
Later builds reuse the downloaded source in the benchmark build directory.

## CMakeLists.txt

### Project requirements

```cmake
cmake_minimum_required(VERSION 3.20)
project(QueryEngine LANGUAGES CXX)
```

These commands require CMake 3.20 or newer and declare a C++ project named
`QueryEngine`.

```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

Every target uses standard C++20. The standard is required, and compiler-only
language extensions such as GNU C++ extensions are disabled.

### Query engine library

```cmake
add_library(query_engine
    src/aggregation.cpp
    src/formatter.cpp
    src/query.cpp
    src/table.cpp
    src/util.cpp
)
```

The reusable engine code is compiled once into the `query_engine` static
library. Tests and applications link against this target instead of compiling
the same implementation files independently.

```cmake
target_include_directories(query_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
```

This exposes the public `include` directory. `PUBLIC` means the include
directory is also inherited by targets that link to `query_engine`.

```cmake
if(MSVC)
    target_compile_options(query_engine PRIVATE /W4)
else()
    target_compile_options(query_engine PRIVATE -Wall -Wextra)
endif()
```

The engine is compiled with common warning flags. `/W4` is used by Microsoft
Visual C++; GCC and Clang use `-Wall -Wextra`. `PRIVATE` means these warnings
only apply while compiling the library itself.

### Example executable

```cmake
add_executable(random_query examples/random.cpp)
target_link_libraries(random_query PRIVATE query_engine)
```

This creates the `random_query` executable from `examples/random.cpp` and links
it to the query engine library.

### Correctness tests

```cmake
include(CTest)
```

CTest creates the `BUILD_TESTING` option, which defaults to `ON`, and enables
test registration.

```cmake
function(add_query_engine_test target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE query_engine)
    add_test(NAME ${target} COMMAND ${target})
endfunction()
```

This helper creates a test executable, links it to the engine, and registers
it with CTest. Each `add_query_engine_test(...)` call below the function adds
one correctness test using that same setup.

Tests are only created inside `if(BUILD_TESTING)`. They are included by the
debug preset and omitted by the benchmark preset.

### Google Benchmark

```cmake
option(QUERY_ENGINE_BUILD_BENCHMARKS "Build Google Benchmark targets" ON)
```

This project-specific option controls whether benchmark dependencies and
targets are included.

```cmake
include(FetchContent)
```

`FetchContent` lets CMake download and build a dependency as part of this
project. Google Benchmark is pinned to `v1.9.5` so builds do not silently
change when its main branch changes.

The `BENCHMARK_ENABLE_*` settings disable Google Benchmark's own tests and
documentation. The project only needs the library.

```cmake
FetchContent_MakeAvailable(google_benchmark)
```

This downloads Google Benchmark when necessary, configures it with the active
compiler, and makes its CMake targets available.

The general reporter also fetches the header-only `nlohmann/json` library,
pinned to `v3.11.3`. C++20 does not include a standard JSON API, so this
dependency provides structured serialization and parsing for summary and
baseline files. Its tests and installation rules are disabled because only the
library target is needed.

The benchmark configuration builds `query_engine_benchmark` from the query,
aggregation, and insertion benchmark sources. It also builds the separate
`query_engine_stress_benchmark` executable for opt-in 10-million-row cases.
Both targets link to the query engine and `benchmark::benchmark_main`, which
provides their `main()` function.

`query_engine_general_benchmark` compiles the same normal benchmark sources but
links `benchmark::benchmark` instead of `benchmark::benchmark_main`. Its
`general_benchmark_main.cpp` supplies a custom `main()` and reporter so it can
select representative registrations and produce grouped JSON directly in C++.

The `run_general_benchmarks`, `run_benchmarks`, and `run_stress_benchmarks`
custom targets execute these programs with consistent repetition settings and
save JSON output under `build/benchmark-clang/benchmark-results`.
`run_general_benchmarks` compares with `general-best.json` when present; it never
promotes results. `save_general_benchmark_baseline` independently records the
current checkout into `general-baseline.json`, leaving the latest and best
files untouched. Run it in the baseline worktree for the original reference.
The [benchmark guide](benchmarks.md) explains explicit checkpoint promotion.

See [Benchmark Guide](benchmarks.md) for the workload matrix and comparison
workflow.

## CMakePresets.json

A preset stores arguments that would otherwise have to be supplied to CMake
manually. Presets also appear in IDE integrations such as VS Code CMake Tools.

### Configure presets

Configure presets determine how CMake generates a build directory.

The `debug` preset uses:

- Build directory: `build/debug`
- Build type: `Debug`
- Correctness tests: enabled by default
- Google Benchmark: disabled

The `benchmark` preset uses:

- Build directory: `build/benchmark-clang`
- Build type: `Release`
- Compiler: Apple Clang at `/usr/bin/clang++`
- Correctness tests: disabled
- Google Benchmark: enabled

The benchmark preset uses Apple Clang because the current macOS SDK is not
compatible with the installed Homebrew GCC when compiling Google Benchmark's
system information code. Its separate build directory also prevents compiler
settings from being mixed with the debug build cache.

### Build presets

Build presets point to configure presets:

```sh
cmake --build --preset debug
cmake --build --preset benchmark
```

They build the already-configured directory associated with the selected
preset.

### Test preset

The `debug` test preset runs CTest in `build/debug` and enables
`outputOnFailure`, so failed tests print their program output:

```sh
ctest --preset debug
```

## Release optimization

The benchmark preset sets:

```json
"CMAKE_BUILD_TYPE": "Release"
```

`Release` does not literally mean `-O3` on every compiler. CMake maps the build
type to flags supplied by the selected compiler toolchain. In the current
Apple Clang configuration, the generated cache contains:

```text
CMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG
```

Therefore the current benchmark executable is compiled with `-O3`.
`-DNDEBUG` disables standard `assert(...)` checks.

Inspect the configured release flags with:

```sh
cmake -N -LA build/benchmark-clang | grep CMAKE_CXX_FLAGS_RELEASE
```

To see complete compiler and linker commands during a build:

```sh
cmake --build --preset benchmark --verbose
```

Do not manually add `-O3` to `target_compile_options`. Selecting the `Release`
build type keeps optimization settings toolchain-aware and avoids accidentally
optimizing debug builds.

## VS Code

Use these commands from the Command Palette:

1. `CMake: Select Configure Preset`
2. Select `Debug` or `Release with benchmarks`
3. Run `CMake: Configure`
4. Run `CMake: Build`

When a configure preset is selected, a compiler kit is unnecessary because
the preset and CMake configuration provide the required build settings.

## Generated files

Everything under `build/` is generated. Source changes belong in
`CMakeLists.txt`, `CMakePresets.json`, or the C++ files, not in generated
Makefiles or CMake cache files inside `build/`.
