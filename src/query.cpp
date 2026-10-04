#include "query_engine/table.hpp"
#include "query_engine/query.hpp"
#include "query_engine/aggregation.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>

namespace query_engine {


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
    const std::vector<OrderByItem>& order_by;
    const std::vector<int>& order_by_column_idx;

    bool operator()(const ResultRow& left, const ResultRow& right) const {
        for (size_t i = 0; i < order_by.size(); i++){
            const OrderByItem& order = order_by[i];
            int idxSort = order_by_column_idx[i];

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


std::vector<int> convertToColumnIdx(
    const std::vector<std::string>& table_columns, 
    const std::vector<std::string>& query_columns
){
    std::vector<int> column_idx;

    for (const auto& query_col: query_columns){
        int col_idx = -1;
        for (size_t i = 0; i < table_columns.size(); i++){
            if (table_columns[i] == query_col){
                col_idx = i;
                break;
            }
        }
        column_idx.push_back(col_idx);
    }
    return column_idx;
}


std::vector<int> convertToColumnIdx(
    const std::vector<std::string>& table_columns, 
    const std::vector<ColumnName>& query_columns
){
    std::vector<int> column_idx;

    for (const auto& query_col: query_columns){
        std::string string_query_col = columnNameToString(query_col);

        int col_idx = -1;
        for (size_t i = 0; i < table_columns.size(); i++){
            if (table_columns[i] == string_query_col){
                col_idx = i;
                break;
            }
        }
        column_idx.push_back(col_idx);
    }
    return column_idx;
}


void applyOrderBy(ResultTable& result_table, const std::vector<OrderByItem>& order_by){
    std::vector<std::string> order_by_column_names;
    for (const OrderByItem& order: order_by){
        order_by_column_names.push_back(orderExpressionToString(order.expr));
    }

    std::vector<int> order_by_column_idx = convertToColumnIdx(
        result_table.column_names, 
        order_by_column_names
    );

    RowComparator row_comparator = {
        .order_by = order_by,
        .order_by_column_idx = order_by_column_idx
    };

    sort(
        result_table.rows.begin(), 
        result_table.rows.end(), 
        std::cref(row_comparator)
    );
}


void applyLimit(ResultTable& result_table, size_t limit){
    while (result_table.rows.size() > limit){
        result_table.rows.pop_back();
    }
}


std::vector<std::reference_wrapper<const Row>> filterRows(
    const std::vector<Row>& rows,
    const std::optional<std::function<bool(const Row&)>>& filter
){
    std::vector<std::reference_wrapper<const Row>> filtered_rows;
    for (const Row& row: rows){
        if (!filter || (*filter)(row)){
            filtered_rows.push_back(row);
        }
    }
    
    return filtered_rows;
}


std::vector<Group> buildGroups(
    const std::vector<std::reference_wrapper<const Row>>& filtered_rows, 
    const std::optional<std::vector<ColumnName>>& group_by
){

    std::vector<Group> groups;
    if (group_by){
        for (const auto& row_ref: filtered_rows){
            const Row& row = row_ref.get();

            std::vector<ResultValue> new_key;
            std::vector<ColumnName> key_columns;
            for (const ColumnName& col: *group_by){
                key_columns.push_back(col);
                new_key.push_back(getColumnValue(row, col));
            }

            bool keyExist = false;
            for (Group& g: groups){
                if (g.key == new_key){
                    keyExist = true;
                    g.rows.push_back(row_ref);
                    break;
                }
            }

            if (!keyExist){
                Group newGroup = {
                    .key_columns = std::move(key_columns),
                    .key = std::move(new_key),
                    .rows = {row_ref}
                };
                groups.push_back(std::move(newGroup));
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
    const std::optional<Aggregation>& aggregation, 
    const std::vector<ColumnName>& projection
){
    std::vector<ResultRow> temp_rows;
    std::vector<int> projection_column_idx = convertToColumnIdx(
        result_table.column_names, 
        projection
    );

    int aggregation_column_idx = -1;
    if (aggregation){
        const std::string aggregation_column_name = aggregationToString(*aggregation);
        for (size_t i = 0; i < result_table.column_names.size(); i++){
            const std::string& col_name = result_table.column_names[i];
            if (aggregation_column_name == col_name){
                aggregation_column_idx = i;
                break;
            }
        }
    }   
    
    for (const ResultRow& row: result_table.rows){
        ResultRow new_row;

        for (int idx: projection_column_idx){
            if (idx != -1){
                new_row.data.push_back(row.data[idx]);
            }
        }

        if (aggregation_column_idx != -1){
            new_row.data.push_back(row.data[aggregation_column_idx]);
        }   
        temp_rows.push_back(std::move(new_row));
    }

    std::vector<std::string> temp_cols;
    for (const ColumnName& col: projection){
        temp_cols.push_back(columnNameToString(col));
    }
    if (aggregation){
        temp_cols.push_back(aggregationToString(*aggregation));
    }

    result_table.rows = std::move(temp_rows);
    result_table.column_names = std::move(temp_cols);
}


ResultTable buildResultTable(
    const std::vector<std::reference_wrapper<const Row>>& rows,
    const std::vector<ColumnName>& columns
){
    ResultTable result_table;
    for (const ColumnName& col: columns){
        result_table.column_names.push_back(columnNameToString(col));
    }

    for (const auto& row_ref: rows){
        const Row& row = row_ref.get();

        ResultRow new_row;
        for (const ColumnName& col: columns){
            new_row.data.push_back(getColumnValue(row, col));
        }
        result_table.rows.push_back(std::move(new_row));
    }
    return result_table;
}


ResultTable buildResultTable(const std::vector<Group>& groups, const Query& query){
    ResultTable result_table;
    for (const ColumnName& col: groups[0].key_columns){
        result_table.column_names.push_back(columnNameToString(col));
    }

    if (query.aggregation){
        const Aggregation& aggregation = *query.aggregation;
        
        result_table.column_names.push_back(aggregationToString(aggregation));
        for (const Group& g: groups){
            ResultRow curRow;
            for (const ResultValue& k: g.key){
                curRow.data.push_back(k);
            }
            curRow.data.push_back(aggregate(g.rows, aggregation));
            result_table.rows.push_back(std::move(curRow));
        }
    }else {
        for (const Group& g: groups){
            ResultRow curRow;
            for (const ResultValue& val: g.key){
                curRow.data.push_back(val);
            }
            result_table.rows.push_back(std::move(curRow));
        }
    }

    return result_table;
}


ResultTable queryTable(const Table& table, const Query& query){
    // This is for filter
    std::vector<std::reference_wrapper<const Row>> filtered_rows = filterRows(table.rows, query.filter);

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

void insertRow(Table& table, Row row){
    table.rows.push_back(row);
}

} // namespace query_engine

