#pragma once

#include "table.hpp"
#include <vector>

enum class ColumnName;

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

ResultValue handle_aggregation(std::vector<Row> rows, Aggregation aggr);
