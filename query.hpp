#pragma once

#include <vector>
#include <functional>
#include <optional>
#include "table.hpp"
#include "aggregation.hpp"

struct Query {
    std::vector<ColumnName> projection;
    std::optional<std::function<bool(Row)>> filter;
    std::optional<Aggregation> aggregation;
    std::optional<std::vector<ColumnName>> group_by;
};

ResultTable query_table(Table table, Query query);
ResultValue getColumnValue(Row row, ColumnName column);
