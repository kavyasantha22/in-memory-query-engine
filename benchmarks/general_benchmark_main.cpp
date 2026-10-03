#include <benchmark/benchmark.h>
#include <nlohmann/json.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

struct GeneralCase {
    std::string_view group;
    std::string_view groupTitle;
    std::string_view label;
    std::string_view benchmark;
    std::int64_t rows;
};

constexpr std::array<GeneralCase, 18> generalCases{{
    {"projection", "Projection", "One column", "bmProjectionOneColumn/1000000", 1'000'000},
    {"projection", "Projection", "Three columns", "bmProjectionThreeColumns/1000000", 1'000'000},
    {"projection", "Projection", "All columns", "bmProjectionAllColumns/1000000", 1'000'000},
    {"filtering", "Filtering", "10% selected", "bmFilterTenPercent/1000000", 1'000'000},
    {"filtering", "Filtering", "50% selected", "bmFilterFiftyPercent/1000000", 1'000'000},
    {"filtering", "Filtering", "100% selected", "bmFilterOneHundredPercent/1000000", 1'000'000},
    {"aggregation", "Aggregation", "SUM", "bmAggregateSum/1000000", 1'000'000},
    {"aggregation", "Aggregation", "AVG", "bmAggregateAverage/1000000", 1'000'000},
    {"grouping", "Grouping", "10 category groups", "bmGroupByCategory/1000000", 1'000'000},
    {"grouping", "Grouping", "100 product groups", "bmGroupByProduct/1000000", 1'000'000},
    {"grouping", "Grouping", "Category and product", "bmGroupByCategoryAndProduct/1000000", 1'000'000},
    {"ordering", "Ordering", "Visible column", "bmOrderByVisibleColumn/100000", 100'000},
    {"ordering", "Ordering", "Multiple columns", "bmOrderByMultipleColumns/100000", 100'000},
    {"ordering", "Ordering", "ORDER BY with LIMIT", "bmOrderByWithLimit/100000", 100'000},
    {"complete_queries", "Complete queries", "Grouped query", "bmComposedGroupedQuery/1000000", 1'000'000},
    {"complete_queries", "Complete queries", "Hidden ORDER BY", "bmComposedHiddenOrderQuery/1000000", 1'000'000},
    {"insertion", "Insertion", "Reserved capacity", "bmInsertReserved/1000000", 1'000'000},
    {"insertion", "Insertion", "Unreserved capacity", "bmInsertUnreserved/1000000", 1'000'000},
}};

struct Measurement {
    double realTimeMs;
    double cpuTimeMs;
    std::optional<double> itemsPerSecond;
};

struct Options {
    std::filesystem::path summaryPath;
    std::optional<std::filesystem::path> baselinePath;
};

double toMilliseconds(double value, benchmark::TimeUnit unit){
    switch (unit) {
        case benchmark::kSecond:
            return value * 1'000.0;
        case benchmark::kMillisecond:
            return value;
        case benchmark::kMicrosecond:
            return value / 1'000.0;
        case benchmark::kNanosecond:
            return value / 1'000'000.0;
    }
    return value;
}

std::optional<double> percentageChange(double current, double baseline){
    if (baseline == 0) return std::nullopt;
    return ((current - baseline) / baseline) * 100.0;
}

nlohmann::ordered_json optionalNumber(const std::optional<double>& value){
    if (value.has_value()) return *value;
    return nullptr;
}

std::string benchmarkFilter(){
    std::string filter = "^(";
    for (std::size_t i = 0; i < generalCases.size(); ++i){
        if (i != 0) filter += '|';
        filter += generalCases[i].benchmark;
    }
    filter += ")$";
    return filter;
}

std::string generatedAtUtc(){
    const auto now = std::chrono::system_clock::now();
    const std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm utcTime{};
#ifdef _WIN32
    gmtime_s(&utcTime, &currentTime);
#else
    gmtime_r(&currentTime, &utcTime);
#endif
    std::ostringstream output;
    output << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");
    return output.str();
}

std::string formatRows(std::int64_t rows){
    std::string result = std::to_string(rows);
    for (std::ptrdiff_t position = static_cast<std::ptrdiff_t>(result.size()) - 3;
         position > 0;
         position -= 3){
        result.insert(static_cast<std::size_t>(position), ",");
    }
    return result;
}

std::string formatThroughput(const std::optional<double>& itemsPerSecond){
    if (!itemsPerSecond.has_value()) return "n/a";

    std::ostringstream output;
    output << std::fixed << std::setprecision(2);
    if (*itemsPerSecond >= 1'000'000){
        output << *itemsPerSecond / 1'000'000 << " M/s";
    } else if (*itemsPerSecond >= 1'000){
        output << *itemsPerSecond / 1'000 << " K/s";
    } else {
        output << *itemsPerSecond << " /s";
    }
    return output.str();
}

std::string formatDelta(const std::optional<double>& delta){
    if (!delta.has_value()) return "n/a";

    std::ostringstream output;
    output << std::showpos << std::fixed << std::setprecision(1) << *delta << '%';
    return output.str();
}

class GeneralReporter : public benchmark::BenchmarkReporter {
public:
    explicit GeneralReporter(Options options): options_(std::move(options)) {}

    bool ReportContext(const Context&) override {
        return true;
    }

    void ReportRuns(const std::vector<Run>& reports) override {
        for (const Run& run: reports){
            const bool isMedian = run.run_type == Run::RT_Aggregate
                && run.aggregate_name == "median";
            const bool isSingleRun = run.run_type == Run::RT_Iteration
                && run.repetitions == 1;
            if ((!isMedian && !isSingleRun) || run.report_big_o || run.report_rms){
                continue;
            }

            std::optional<double> itemsPerSecond;
            const auto counter = run.counters.find("items_per_second");
            if (counter != run.counters.end()){
                itemsPerSecond = counter->second.value;
            }

            measurements_[run.run_name.str()] = Measurement{
                .realTimeMs = toMilliseconds(run.GetAdjustedRealTime(), run.time_unit),
                .cpuTimeMs = toMilliseconds(run.GetAdjustedCPUTime(), run.time_unit),
                .itemsPerSecond = itemsPerSecond
            };
        }
    }

    void Finalize() override {
        try {
            const auto baseline = loadBaseline();
            const nlohmann::ordered_json summary = buildSummary(baseline);
            writeSummary(summary);
            printSummary(baseline);
        } catch (const std::exception& error){
            succeeded_ = false;
            GetErrorStream() << "general benchmark report failed: " << error.what() << '\n';
        }
    }

    bool succeeded() const {
        return succeeded_;
    }

private:
    using MeasurementMap = std::unordered_map<std::string, Measurement>;

    MeasurementMap loadBaseline() const {
        MeasurementMap baseline;
        if (!options_.baselinePath.has_value()
            || !std::filesystem::exists(*options_.baselinePath)){
            if (options_.baselinePath.has_value()){
                std::cerr << "No checkpoint comparison available: reference file not found: "
                          << options_.baselinePath->string() << '\n';
            }
            return baseline;
        }

        std::ifstream source(*options_.baselinePath);
        if (!source) throw std::runtime_error("could not open baseline file");

        const nlohmann::json document = nlohmann::json::parse(source);
        for (const auto& [groupName, results]: document.at("groups").items()){
            (void)groupName;
            for (const auto& result: results){
                const auto& median = result.at("median");
                std::optional<double> throughput;
                if (!median.at("items_per_second").is_null()){
                    throughput = median.at("items_per_second").get<double>();
                }
                baseline.emplace(
                    result.at("benchmark").get<std::string>(),
                    Measurement{
                        .realTimeMs = median.at("real_time_ms").get<double>(),
                        .cpuTimeMs = median.at("cpu_time_ms").get<double>(),
                        .itemsPerSecond = throughput
                    }
                );
            }
        }
        return baseline;
    }

    nlohmann::ordered_json buildSummary(const MeasurementMap& baseline) const {
        nlohmann::ordered_json groups = nlohmann::ordered_json::object();
        for (const GeneralCase& testCase: generalCases){
            const std::string benchmarkName(testCase.benchmark);
            const auto current = measurements_.find(benchmarkName);
            if (current == measurements_.end()){
                throw std::runtime_error("missing median result for " + benchmarkName);
            }

            const Measurement& measurement = current->second;
            nlohmann::ordered_json changes = nullptr;
            const auto baselineMeasurement = baseline.find(benchmarkName);
            if (baselineMeasurement != baseline.end()){
                changes = {
                    {"real_time", optionalNumber(percentageChange(
                        measurement.realTimeMs,
                        baselineMeasurement->second.realTimeMs
                    ))},
                    {"cpu_time", optionalNumber(percentageChange(
                        measurement.cpuTimeMs,
                        baselineMeasurement->second.cpuTimeMs
                    ))},
                    {"items_per_second", optionalNumber(throughputChange(
                        measurement.itemsPerSecond,
                        baselineMeasurement->second.itemsPerSecond
                    ))}
                };
            }

            const std::string groupName(testCase.group);
            if (!groups.contains(groupName)){
                groups[groupName] = nlohmann::ordered_json::array();
            }
            groups[groupName].push_back({
                {"label", std::string(testCase.label)},
                {"benchmark", benchmarkName},
                {"rows", testCase.rows},
                {"median", {
                    {"real_time_ms", measurement.realTimeMs},
                    {"cpu_time_ms", measurement.cpuTimeMs},
                    {"items_per_second", optionalNumber(measurement.itemsPerSecond)}
                }},
                {"change_from_baseline_percent", changes}
            });
        }

        const bool baselineUsed = !baseline.empty();
        return {
            {"schema_version", 1},
            {"generated_at", generatedAtUtc()},
            {"baseline", baselineUsed
                ? nlohmann::ordered_json(options_.baselinePath->string())
                : nlohmann::ordered_json(nullptr)},
            {"change_convention", "positive time is slower; positive throughput is faster"},
            {"groups", groups}
        };
    }

    static std::optional<double> throughputChange(
        const std::optional<double>& current,
        const std::optional<double>& baseline
    ){
        if (!current.has_value() || !baseline.has_value()) return std::nullopt;
        return percentageChange(*current, *baseline);
    }

    void writeSummary(const nlohmann::ordered_json& summary) const {
        std::filesystem::create_directories(options_.summaryPath.parent_path());
        std::ofstream destination(options_.summaryPath);
        if (!destination) throw std::runtime_error("could not open summary output file");
        destination << std::setw(2) << summary << '\n';
    }

    void printSummary(const MeasurementMap& baseline) const {
        GetOutputStream() << "\nGeneral benchmark summary (median wall time)\n"
                          << "Positive time change means slower; negative means faster.\n\n";

        std::string_view previousGroup;
        for (const GeneralCase& testCase: generalCases){
            if (testCase.group != previousGroup){
                GetOutputStream() << testCase.groupTitle << '\n'
                                  << "  " << std::left << std::setw(24) << "Case"
                                  << std::right << std::setw(11) << "Rows"
                                  << std::setw(12) << "Time"
                                  << std::setw(15) << "Throughput"
                                  << std::setw(13) << "vs baseline" << '\n';
                previousGroup = testCase.group;
            }

            const std::string benchmarkName(testCase.benchmark);
            const Measurement& measurement = measurements_.at(benchmarkName);
            std::optional<double> delta;
            const auto baselineMeasurement = baseline.find(benchmarkName);
            if (baselineMeasurement != baseline.end()){
                delta = percentageChange(
                    measurement.realTimeMs,
                    baselineMeasurement->second.realTimeMs
                );
            }

            std::ostringstream time;
            time << std::fixed << std::setprecision(2) << measurement.realTimeMs << " ms";
            GetOutputStream() << "  " << std::left << std::setw(24) << testCase.label
                              << std::right << std::setw(11) << formatRows(testCase.rows)
                              << std::setw(12) << time.str()
                              << std::setw(15) << formatThroughput(measurement.itemsPerSecond)
                              << std::setw(13) << formatDelta(delta) << '\n';

            const bool isLastInGroup = &testCase == &generalCases.back()
                || (&testCase + 1)->group != testCase.group;
            if (isLastInGroup) GetOutputStream() << '\n';
        }

        GetOutputStream() << "Summary: " << options_.summaryPath.string() << '\n';
    }

    Options options_;
    MeasurementMap measurements_;
    bool succeeded_ = true;
};

std::optional<std::string> optionValue(std::string_view argument, std::string_view name){
    const std::string prefix = "--" + std::string(name) + '=';
    if (!argument.starts_with(prefix)) return std::nullopt;
    return std::string(argument.substr(prefix.size()));
}

std::optional<Options> extractOptions(int& argc, char** argv){
    Options options;
    int writeIndex = 1;
    for (int readIndex = 1; readIndex < argc; ++readIndex){
        const std::string_view argument(argv[readIndex]);
        if (const auto value = optionValue(argument, "general_summary")){
            options.summaryPath = *value;
        } else if (const auto value = optionValue(argument, "general_baseline")){
            options.baselinePath = std::filesystem::path(*value);
        } else {
            argv[writeIndex++] = argv[readIndex];
        }
    }
    argc = writeIndex;

    if (options.summaryPath.empty()){
        std::cerr << "missing required --general_summary=<path> argument\n";
        return std::nullopt;
    }
    return options;
}

} // namespace

int main(int argc, char** argv){
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    const std::optional<Options> options = extractOptions(argc, argv);
    if (!options.has_value()) return 1;

    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    benchmark::SetBenchmarkFilter(benchmarkFilter());

    GeneralReporter reporter(*options);
    benchmark::RunSpecifiedBenchmarks(&reporter);
    benchmark::Shutdown();
    return reporter.succeeded() ? 0 : 1;
}
