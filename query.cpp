#include "table.hpp"
#include "query.hpp"
#include "aggregation.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>


std::string orderExpressionToString(const OrderExpression& expression) {
    if (const auto* column = std::get_if<ColumnName>(&expression)) {
        return columnNameToString(*column);
    }

    const auto& aggregation = std::get<Aggregation>(expression);

    return aggregatationTypeToString(aggregation.type)
        + "("
        + columnNameToString(aggregation.column)
        + ")";
}

class RowComparator {
public:
    std::vector<OrderByItem> order_by;
    std::vector<std::string> column_names;

    bool operator()(const ResultRow& left, const ResultRow& right) const {
        for (OrderByItem order: order_by){
            int idxSort = -1;
            for (int i = 0; i < (int) column_names.size(); i++){
                std::string expr_str = orderExpressionToString(order.expr);
                if (column_names[i] == expr_str){
                    idxSort = i;
                    break;
                }
            }
            if (idxSort == -1) continue;

            ResultValue left_val = left.data[idxSort];
            ResultValue right_val = right.data[idxSort];
            if (
                (order.ascending && left_val < right_val) ||
                (!order.ascending && left_val > right_val)
            ) return true;

            if (
                (order.ascending && left_val > right_val) ||
                (!order.ascending && left_val < right_val)
            ) return false;
        }
        return false;
    }
};


void handle_order_by(ResultTable& result_table, std::vector<OrderByItem> order_by){
    RowComparator row_comparator = {
        .order_by = order_by,
        .column_names = result_table.column_names
    };
    sort(result_table.rows.begin(), result_table.rows.end(), row_comparator);
}


void handle_limit(ResultTable& result_table, size_t limit){
    while (result_table.rows.size() > limit){
        result_table.rows.pop_back();
    }
};


ResultTable query_table(Table table, Query query){
    // This is for filter
    std::vector<Row> filtered_rows;
    for (auto row: table.rows){
        if (!query.filter || (*query.filter)(row)){
            filtered_rows.push_back(row);
        }
    }

    if (filtered_rows.size() == 0){
        ResultTable result_table;
        for (ColumnName col: query.projection){
            result_table.column_names.push_back(columnNameToString(col));
        }
        return result_table;
    } 

    // no group by and no aggregation
    if (!query.group_by && !query.aggregation){
        ResultTable result_table;
        for (ColumnName col: query.projection){
            result_table.column_names.push_back(columnNameToString(col));
        }
        for (Row row: filtered_rows){
            ResultRow new_row;
            for (ColumnName col: query.projection){
                new_row.data.push_back(getColumnValue(row, col));
            }
            result_table.rows.push_back(new_row);
        }

        if (query.order_by){
            handle_order_by(result_table, *query.order_by);
        }

        if (query.limit){
            handle_limit(result_table, *query.limit);
        }
        return result_table;
    }

    // this is for group by
    std::vector<Group> groups;
    if (query.group_by){
        for (auto row: filtered_rows){
            std::vector<ResultValue> new_key;
            std::vector<ColumnName> key_columns;
            Row new_row = row;
            for (ColumnName col: *query.group_by){
                key_columns.push_back(col);
                new_key.push_back(getColumnValue(row, col));
            }
            bool keyExist = false;
            for (Group& g: groups){
                if (g.key == new_key){
                    keyExist = true;
                    g.rows.push_back(new_row);
                }
            }
            if (!keyExist){
                Group newGroup = {
                    .key_columns = key_columns,
                    .key = new_key,
                    .rows = {row}
                };
                groups.push_back(newGroup);
            }
        }
    }else{
        groups.push_back(Group{
            // i need to add all columnNames,
            .key_columns = {},
            // and then put everything in both .key and .rows
            .key = {},
            .rows = filtered_rows
        });
    }

    // this is for aggregation
    ResultTable result_table;
    for (ColumnName col: groups[0].key_columns){
        result_table.column_names.push_back(columnNameToString(col));
    }

    if (query.aggregation){
        const Aggregation aggregation = *query.aggregation;
        const std::string aggregation_column_name = 
            aggregatationTypeToString(aggregation.type) + 
            "(" + columnNameToString(aggregation.column) + ")";
        
        result_table.column_names.push_back(aggregation_column_name);
        for (Group g: groups){
            ResultRow curRow;
            for (ResultValue k: g.key){
                curRow.data.push_back(k);
            }
            curRow.data.push_back(aggregate(g.rows, aggregation));
            result_table.rows.push_back(curRow);
        }
    }else {
        for (Group g: groups){
            ResultRow curRow;
            for (ResultValue val: g.key){
                curRow.data.push_back(val);
            }
            result_table.rows.push_back(curRow);
        }
    }

    // this is for projection
    // i'm assuming all columns are in result_table.column_names
    for (int i = (int) result_table.column_names.size() - 1; i >= 0 ; i--){
        int included = false;
        std::string column_name = result_table.column_names[i];
        for (ColumnName col: query.projection){
            if (columnNameToString(col) == column_name){
                included = true;
            }
        }
        if (query.aggregation){
            const Aggregation aggregation = *query.aggregation;
            const std::string aggregation_column_name = 
                aggregatationTypeToString(aggregation.type) + 
                "(" + columnNameToString(aggregation.column) + ")";

            if (aggregation_column_name == column_name) included = true;
        }


        if (!included) {
            result_table.column_names.erase(result_table.column_names.begin() + i);
            for (ResultRow& row: result_table.rows){
                row.data.erase(row.data.begin() + i);
            }
        }
    }

    // handle Order By
    if (query.order_by){
        handle_order_by(result_table, *query.order_by);
    }

    if (query.limit){
        handle_limit(result_table, *query.limit);
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


std::string columnNameToString(ColumnName col){
    switch (col) {
        case ColumnName::TRANSACTION_ID:
            return "transaction_id";
        case ColumnName::PRODUCT_ID:
            return "product_id";
        case ColumnName::CATEGORY_ID:
            return "category_id";
        case ColumnName::PRICE:
            return "price";
        case ColumnName::QUANTITY:
            return "quantity";
        case ColumnName::TIMESTAMP:
            return "timestamp";
    }

    throw std::invalid_argument("Unknown column");
}


std::string aggregatationTypeToString(AggregationType type){
    switch (type) {
        case AggregationType::NONE:
            return "none";
        case AggregationType::COUNT:
            return "count";
        case AggregationType::SUM:
            return "sum";
        case AggregationType::AVG:
            return "avg";
        case AggregationType::MIN:
            return "min";
        case AggregationType::MAX:
            return "max";
    }

    throw std::invalid_argument("Unknown aggregation type");
}
