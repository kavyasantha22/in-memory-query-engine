#pragma once

#include <vector>
#include <functional>
#include <optional>
#include "table.hpp"
#include "aggregation.hpp"
#include <variant>

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
