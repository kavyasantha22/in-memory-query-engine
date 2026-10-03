#pragma once

#include "query_engine/table.hpp"
#include <vector>

namespace query_engine {

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

ResultValue aggregate(const std::vector<Row>& rows, const Aggregation& aggr);

std::vector<ResultValue> aggregateGroups(const std::vector<Group>& groups, const Aggregation& aggr);

std::string aggregationTypeToString(const AggregationType& type);

std::string aggregationToString(const Aggregation& aggregation);

} // namespace query_engine
