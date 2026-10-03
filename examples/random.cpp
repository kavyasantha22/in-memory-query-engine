#include "query_engine/table.hpp"
#include "query_engine/query.hpp"
#include "query_engine/formatter.hpp"
#include <optional>

int main(){
    query_engine::Table table = query_engine::generateTable(0);
    query_engine::printSqlTable("Initial Table", table);
    query_engine::Query query = {
        .projection = {query_engine::ColumnName::CATEGORY_ID},
        .filter = std::nullopt,
        .aggregation = query_engine::Aggregation{
            .type=query_engine::AggregationType::AVG,
            .column=query_engine::ColumnName::PRICE
        },
        .group_by = std::vector<query_engine::ColumnName>{query_engine::ColumnName::CATEGORY_ID},
    };
    query_engine::ResultTable rTable = query_engine::queryTable(table, query);
    query_engine::printSqlTable("Result", rTable);
    return 0;
}
