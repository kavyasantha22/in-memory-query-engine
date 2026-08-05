#include "table.hpp"
#include "query.hpp"
#include "formatter.hpp"
#include <optional>

int main(){
    Table table = generateTable(0);
    printSqlTable("Initial Table", table);
    Query query = {
        .projection = {ColumnName::CATEGORY_ID},
        .filter = std::nullopt,
        .aggregation = Aggregation{
            .type=AggregationType::AVG,
            .column=ColumnName::PRICE
        },
        .group_by = std::vector<ColumnName>{ColumnName::CATEGORY_ID},
    };
    ResultTable rTable = queryTable(table, query);
    printSqlTable("Result", rTable);
    return 0;
}
