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

    std::vector<Group> groups;
    if (query.group_by){
        for (auto row: filtered_rows){
            std::vector<ResultValue> newKey;
            Row newRow = row;
            for (ColumnName col: *query.group_by){
                newKey.push_back(getColumnValue(row, col));
            }
            bool keyExist = false;
            for (Group& g: groups){
                if (g.key == newKey){
                    keyExist = true;
                    g.rows.push_back(newRow);
                }
            }
            if (!keyExist){
                Group newGroup = {
                    .key = newKey,
                    .rows = {row}
                };
                groups.push_back(newGroup);
            }
        }
    }else{
        groups.push_back(Group{
            .key = {},
            .rows = filtered_rows
        });
    }

    ResultTable result_table;
    if (query.aggregation){
        for (Group g: groups){
            ResultRow curRow;
            for (ResultValue k: g.key){
                curRow.data.push_back(k);
            }
            curRow.data.push_back(aggregate(g.rows, *query.aggregation));
            result_table.rows.push_back(curRow);
        }
        return result_table;
    }else{
        for (auto row: filtered_rows){
            ResultRow result_row;
            for (ColumnName column: query.projection){
                result_row.data.push_back(getColumnValue(row, column));
            }
            result_table.rows.push_back(result_row);
        }
        return result_table;
    }
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
