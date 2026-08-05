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
    }else{
        const auto& aggregation = std::get<Aggregation>(expression);
        return aggregationToString(aggregation);
    }
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


void applyOrderBy(ResultTable& result_table, std::vector<OrderByItem> order_by){
    RowComparator row_comparator = {
        .order_by = order_by,
        .column_names = result_table.column_names
    };
    sort(result_table.rows.begin(), result_table.rows.end(), row_comparator);
}


void applyLimit(ResultTable& result_table, size_t limit){
    while (result_table.rows.size() > limit){
        result_table.rows.pop_back();
    }
}


std::vector<Row> filterRows(
    std::vector<Row> rows,
    std::optional<std::function<bool(Row)>> filter
){
    if (!filter) return rows;

    std::vector<Row> filtered_rows;
    for (auto row: rows){
        if ((*filter)(row)){
            filtered_rows.push_back(row);
        }
    }
    
    return filtered_rows;
}


std::vector<Group> buildGroups(
    std::vector<Row> filtered_rows, 
    std::optional<std::vector<ColumnName>> group_by
){
    std::vector<Group> groups;
    if (group_by){
        for (auto row: filtered_rows){
            std::vector<ResultValue> new_key;
            std::vector<ColumnName> key_columns;
            Row new_row = row;
            for (ColumnName col: *group_by){
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
    }else {
        groups.push_back(Group{
            .key_columns = {},
            .key = {},
            .rows = filtered_rows
        });
    }
    
    return groups;
}


void applyProjection(
    ResultTable& result_table, 
    std::optional<Aggregation> aggregation, 
    std::vector<ColumnName> projection
){
    std::vector<ResultRow> temp_rows;
    for (ResultRow row: result_table.rows){
        ResultRow new_row;

        for (ColumnName col: projection){
            int idx = -1;
            for (size_t i = 0; i < result_table.column_names.size(); i++){
                std::string col_name = result_table.column_names[i];
                if (columnNameToString(col) == col_name){
                    idx = i;
                    break;
                }
            }

            if (idx != -1){
                new_row.data.push_back(row.data[idx]);
            }
        }

        if (aggregation){
            const std::string aggregation_column_name = aggregationToString(*aggregation);

            int idx = -1;
            for (size_t i = 0; i < result_table.column_names.size(); i++){
                std::string col_name = result_table.column_names[i];
                if (aggregation_column_name == col_name){
                    idx = i;
                    break;
                }
            }

            if (idx != -1){
                new_row.data.push_back(row.data[idx]);
            }
        }   
        temp_rows.push_back(new_row);
    }

    std::vector<std::string> temp_cols;
    for (ColumnName col: projection){
        temp_cols.push_back(columnNameToString(col));
    }
    if (aggregation){
        const std::string aggregation_column_name = aggregationToString(*aggregation);
        temp_cols.push_back(aggregation_column_name);
    }

    result_table.rows = temp_rows;
    result_table.column_names = temp_cols;
}


ResultTable buildResultTable(
    const std::vector<Row> rows,
    const std::vector<ColumnName> columns
){
    ResultTable result_table;
    for (ColumnName col: columns){
        result_table.column_names.push_back(columnNameToString(col));
    }

    for (Row row: rows){
        ResultRow new_row;
        for (ColumnName col: columns){
            new_row.data.push_back(getColumnValue(row, col));
        }
        result_table.rows.push_back(new_row);
    }
    return result_table;
}


ResultTable buildResultTable(const std::vector<Group> groups, const Query query){
    ResultTable result_table;
    for (ColumnName col: groups[0].key_columns){
        result_table.column_names.push_back(columnNameToString(col));
    }

    if (query.aggregation){
        const Aggregation aggregation = *query.aggregation;
        const std::string aggregation_column_name = aggregationToString(aggregation);
        
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

    return result_table;
}


ResultTable queryTable(Table table, Query query){
    // This is for filter
    std::vector<Row> filtered_rows = filterRows(table.rows, query.filter);

    if (filtered_rows.size() == 0){
        ResultTable result_table = buildResultTable(filtered_rows, table.column_names);
        applyProjection(result_table, query.aggregation, query.projection);
        return result_table;
    } 

    ResultTable result_table;
    // no group by and no aggregation
    if (!query.group_by && !query.aggregation){
        result_table = buildResultTable(filtered_rows, table.column_names);
    }else {
        std::vector<Group> groups = buildGroups(filtered_rows, query.group_by);
        result_table = buildResultTable(groups, query);
    }

    if (query.order_by) 
        applyOrderBy(result_table, *query.order_by);

    if (query.limit) 
        applyLimit(result_table, *query.limit);
        
    applyProjection(result_table, query.aggregation, query.projection);
    
    return result_table;
}





