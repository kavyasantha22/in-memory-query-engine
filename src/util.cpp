#include "util.hpp"

namespace query_engine {

std::string orderExpressionToString(const OrderExpression& expression) {
    if (const auto* column = std::get_if<ColumnName>(&expression)) {
        return columnNameToString(*column);
    }else{
        const auto& aggregation = std::get<Aggregation>(expression);
        return aggregationToString(aggregation);
    }
}


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

} // namespace query_engine
