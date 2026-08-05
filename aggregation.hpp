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

std::vector<ResultValue> aggregateGroups(std::vector<Group> groups, Aggregation aggr);

std::string aggregationTypeToString(AggregationType type);

std::string aggregationToString(Aggregation aggregation);
