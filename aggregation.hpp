#pragma once

#include "table.hpp"
#include <vector>

enum class AggregationType {
    NONE,
    COUNT,
    SUM,
    AVG,
    MIN,
    MAX
};

struct Aggregation {
    AggregationType type;
    ColumnName column;
};

ResultValue aggregate(std::vector<Row> rows, Aggregation aggr);

std::vector<ResultValue> aggregate_groups(std::vector<Group> groups, Aggregation aggr);