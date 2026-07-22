#include "table.hpp"
#include "query.hpp"
#include "aggregation.hpp"
#include <stdexcept>
#include <vector>

ResultTable query_table(Table table, Query query){
    std::vector<Row> filtered_rows;

    for (auto row: table.rows){
        if (!query.filter || (*query.filter)(row)){
            filtered_rows.push_back(row);
        }
    }

    if (query.aggregation){
        ResultTable result_table;
        ResultRow result_row;
        result_row.data.push_back(handle_aggregation(filtered_rows, *query.aggregation));
        result_table.rows.push_back(result_row);
        return result_table;
    }

    ResultTable result_table;
    for (auto row: filtered_rows){
        ResultRow result_row;
        for (ColumnName column: query.projection){
            result_row.data.push_back(getColumnValue(row, column));
        }
        result_table.rows.push_back(result_row);
    }
    return result_table;
}




ResultValue getColumnValue(Row row, ColumnName column){
    switch (column) {
        case ColumnName::TRANSACTION_ID:
            return row.transaction_id;

        case ColumnName::PRODUCT_ID:
            return row.product_id;

        case ColumnName::CATEGORY_ID:
            return row.category_id;

        case ColumnName::PRICE:
            return row.price;

        case ColumnName::QUANTITY:
            return row.quantity;

        case ColumnName::TIMESTAMP:
            return row.timestamp;
    }

    throw std::invalid_argument("Unknown column");
}
