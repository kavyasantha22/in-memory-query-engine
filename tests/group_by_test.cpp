#include "query_engine/aggregation.hpp"
#include "query_engine/formatter.hpp"
#include "query_engine/query.hpp"
#include "query_engine/table.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

int main(){
    query_engine::Table table = query_engine::generateTable(20);
    query_engine::printSqlTable("generated table", table);

    query_engine::Query q = {
        .projection = {
            query_engine::ColumnName::TRANSACTION_ID,
            query_engine::ColumnName::PRODUCT_ID,
            query_engine::ColumnName::CATEGORY_ID,
            query_engine::ColumnName::PRICE,
            query_engine::ColumnName::QUANTITY,
            query_engine::ColumnName::TIMESTAMP
        },
        .filter = std::nullopt,
        .aggregation = query_engine::Aggregation{query_engine::AggregationType::SUM, query_engine::ColumnName::PRICE},
        .group_by = std::vector<query_engine::ColumnName>{
            query_engine::ColumnName::CATEGORY_ID
        },
        .order_by = std::nullopt,
        .limit = std::nullopt
    };

    query_engine::ResultTable result = query_engine::queryTable(table, q);
    query_engine::printSqlTable("group by result", result);
}
