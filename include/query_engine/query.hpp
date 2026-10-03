#pragma once

#include <vector>
#include <functional>
#include <optional>
#include "query_engine/table.hpp"
#include "query_engine/aggregation.hpp"
#include <variant>

namespace query_engine {

using OrderExpression = std::variant<ColumnName, Aggregation>;

struct OrderByItem{
    OrderExpression expr;
    bool ascending;
};

struct Query {
    std::vector<ColumnName> projection;
    std::optional<std::function<bool(Row)>> filter;
    std::optional<Aggregation> aggregation;
    std::optional<std::vector<ColumnName>> group_by;
    std::optional<std::vector<OrderByItem>> order_by;
    std::optional<size_t> limit;

    // std::optional<int> limit;
};

ResultTable queryTable(Table table, Query query);

void insertRow(Table& table, Row row);

} // namespace query_engine
